# Code Review — TextId Cross-Table Canonicalization

Reviewed: 2026-04-30
Branch: main
Range: `8c1e008c7..ef267bd0c` (4 commits)
Result: **44 → 20 ctest failures**, no regressions.

## Background

Ideas 139/140/141 progressively split TypeSpec record identity into multiple
TextId-keyed maps (`struct_def_owner_index`, `ConstEvalEnv::struct_defs`,
LIR `struct_names`, etc.). Producers may intern TextIds in different
`TextTable`s — parser/lexer token table vs. HIR `link_name_texts` —
without a canonicalization point. The 44-failure baseline at `8c1e008c7`
was the visible cost of that asymmetry.

## Commits Under Review

### 1. `119a6ef04` — Recover consteval record layout via owner-index rendered tag

First, narrow fix in `lookup_record_layout`. Replaced the strict
`has_record_layout_metadata` guard with an O(n) scan of
`struct_def_owner_index`: when the structured key misses, accept any
owner entry whose rendered tag and namespace agree with the TypeSpec.
A bare `struct_defs[compat_tag]` fallback is still refused so stale
spelling cannot silently substitute identity (idea 141 guard).

**Verdict**: Correct as a minimal fix, but ad-hoc — a per-call-site scan
that does not generalize. Superseded by commit 2.

### 2. `e99baef4b` — Canonicalize consteval record lookup via `link_name_texts->find`

Same 6 tests pass, ad-hoc scan replaced with O(1) canonicalization:

- Added `link_name_texts` (`const TextTable*`) to `ConstEvalEnv`.
- Wired it from `make_engine_consteval_env` and `Lowerer` env builders.
- `lookup_record_layout` now:
  1. Direct owner-key lookup (fast).
  2. Re-canonicalize via `link_name_texts->find(rendered_tag)` and retry.
  3. Bare rendered fallback only when `struct_def_owner_index` is null.

**Verdict**: Clean. The three-step ladder makes the strict-guard semantics
explicit (path 3 is gated). `TextTable::find` is `const`-friendly (no
mutation) — appropriate for read-only lookup.

**Risk surface**: New env field unwired in any other site that builds a
ConstEvalEnv would silently fall back to path 3 and lose the canonical
recovery. Searched: every `env.struct_def_owner_index = ...` site (3 in
`engine.cpp`, 1 in `hir_types.cpp`) was updated. None missed.

### 3. `fbbfe78b7` — Prefer rendered C-string for codegen aggregate compatibility tag

`typespec_aggregate_compatibility_tag(mod, ts)` previously asked
`mod.link_name_texts->lookup(ts.tag_text_id)` first. When `ts.tag_text_id`
came from a different table, the lookup returned whatever string happened
to share that ID — e.g. variable name `"s"` in place of struct tag `"S"`
in `c_testsuite_src_00019`'s `s.p->p->...` chain. The rendered C-string
fallback never fired.

Swapped: ask `typespec_aggregate_final_spelling(ts)` first (uses `ts.tag`
and `record_def->name`, both cross-table-safe); only consult
`link_name_texts->lookup(tag_text_id)` for typespecs that carry no
rendered tag of their own.

**Verdict**: One-line semantic swap, big payoff (-14 failures). The
remaining `link_name_texts->lookup(tag_text_id)` path still has the
mismatch issue but only triggers for typespecs without `ts.tag` and
without `record_def->name` — narrow and probably internally produced.

**Risk surface**: A producer that DOES intern its TypeSpec into
`link_name_texts` and then has a stale `ts.tag` would now read the stale
string in preference to the canonical TextId. No such producer was
identified, but worth a sweep if a regression surfaces.

### 4. `ef267bd0c` — Unify aggregate owner_key lookup with rendered-tag agreement

Three duplicated helpers were building the same owner key from
`ts.tag_text_id`:
- `typespec_aggregate_owner_key(const TypeSpec&)` (codegen/shared)
- `indexed_gep_owner_key_from_type` (lvalue.cpp)
- `const_init_aggregate_owner_key_from_type` (const_init_emitter.cpp)

The pr44164 nested-struct global init revealed a subtler failure mode:
the direct lookup hit a *different* record because TextId 7 was both
`"Y"` in the parser table and `"YY"` in `link_name_texts`. The IR shipped
`%struct.X { %struct.Y { %struct.Z { ... } } }` — silently dropping the
YY layer.

Added a single Module-aware overload:
```
typespec_aggregate_owner_key(const TypeSpec& ts, const Module& mod)
```
which:
1. Tries the direct key.
2. **Verifies the indexed rendered tag string equals ts's own rendered
   C-string.** If they disagree, the direct hit is a cross-table TextId
   collision — not canonical.
3. Re-canonicalizes via `link_name_texts->find(rendered_tag)`.

Retired the two duplicate local helpers; routed every aggregate owner-key
call site through the unified helper.

**Verdict**: This is the helper the pattern was asking for. The
"rendered tag agreement" check (step 2) is the real generalization — it
catches collisions that look-then-fall-back patterns would miss. The
fact that pr44164 failed for several commits before being detected is
itself a warning that any direct-TextId lookup not paired with a
rendered-tag agreement check is suspect.

**Risk surface**:
- The agreement check requires `ts.tag` (the rendered C-string) to be
  set. If a producer leaves `ts.tag` empty but sets `ts.tag_text_id`
  from a foreign table, the agreement check skips and the direct hit
  remains. This case surfaced as the existing `typespec_aggregate_owner_key`
  no-mod fallback. The mod-aware overload still falls back to that
  behavior when the rendered tag is empty — acceptable but a known
  weakness.
- `typespec_aggregate_owner_key(ts, mod)` is now O(small) on mismatched
  hits (one extra `find_struct_def_tag_by_owner` + one
  `link_name_texts->find`). Both maps are hash-indexed; effective O(1).
- `is_named_aggregate_value(ts)` was deliberately left on the no-mod
  variant: it's a "shape" predicate that should not require a Module
  reference. If it ever needs cross-table verification, switch then.

## Test Coverage

Existing fixtures already exercised the failure modes — no new tests
written. Fixtures that flipped from failing to passing:

| Bug shape | Fixed tests |
|---|---|
| `sizeof: unresolved record layout` (consteval, idea 140) | 6 |
| `field 'X' not found in struct/union 'Y'` (StmtEmitter, ts.tag.*compat) | 14 |
| Nested struct global init layer drop (TextId collision) | 4 |
| Total | 24 |

Guard tests that were previously protecting strict behavior all still
pass:
- `cpp_hir_sema_consteval_type_utils_structured_metadata` — verifies
  rendered fallback does NOT save us when owner_index legitimately has
  no entry.
- `test_consteval_sizeof_prefers_owner_metadata_over_stale_rendered_tag`
  — verifies structured wins over stale rendered when both present.

## Outstanding

- `cpp_hir_parser_type_base_producer_structured_metadata` still fails:
  this is a parser-layer guard test for idea 141, not affected by this
  HIR/codegen-layer canonicalization work.
- 20 remaining failures: 8 parser disambiguation matrix
  (`c_style_cast_target` group), 4 LIR `StructNameId` mirror
  (`20040709_1/2/3` + `pr71083`), and the rest unclassified Group G.
- The "rendered tag agreement" pattern likely applies elsewhere in the
  codebase; recommend searching for direct `find_struct_def_tag_by_owner`
  calls and confirming each one either (a) builds the key from a TextId
  guaranteed to be in `link_name_texts`, or (b) verifies the rendered
  agreement.

## Recommendation

**Accept all four commits.** The series replaces three duplicated
helpers with one shared, cross-table-safe lookup. The diff is mostly
deletions plus a single, well-documented helper. Strict failure-mode
guards are preserved; previously-passing tests continue to pass; 24 of
the 44 baseline failures are now green.
