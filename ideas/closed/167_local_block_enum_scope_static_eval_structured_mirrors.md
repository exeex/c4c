# Local Block Enum Scope Static Eval Structured Mirrors

Status: Closed
Created: 2026-05-11
Closed: 2026-05-11

Parent Ideas:
- `ideas/closed/164_sema_type_utils_static_eval_structured_lookup.md`

Follow-up Ideas:
- `ideas/open/168_static_member_compile_time_local_enum_lifetime.md`

## Goal

Retire or narrow the remaining local/block enum-scope static-eval dependence on
mutable rendered enum mirrors by giving those scopes structured or TextId-aware
enum metadata where the surrounding sema/HIR context can provide it.

## Why This Idea Exists

Idea 164 converted the covered global/static-member `type_utils` static-eval
paths and fenced rendered lookup as a compatibility bridge. During closure,
local/block enum-scope mirrors were identified as adjacent residual work: they
can still depend on mutable `enum_consts_` save/restore behavior rather than a
structured enum-domain carrier.

This is separate from idea 164 because local/block enum scopes may require
different ownership and lifetime handling than global/static-member lookup.

## Completion Summary

Idea 167 completed the lowerer-focused local/block enum route. The HIR lowerer
now carries scoped enum `TextId` and local-key maps beside the rendered
`enum_consts_` compatibility mirror. Non-global enum collection populates those
scoped maps from `enum_name_text_ids`, and block / statement-expression exits
restore their scoped-map depth with `enum_consts_` so local/block enum lifetime
matches the existing mirror lifetime.

`make_lowerer_consteval_env` now passes scoped enum metadata into
`ConstEvalEnv` for constexpr-if, switch/range folding, ternary folding, local
const folding, template value arguments, and consteval-call argument
evaluation. Direct enum-expression lowering uses `ConstEvalEnv::lookup(n)` so
scoped TextId/key metadata is checked before rendered `enum_consts_`, and
complete scoped misses fail closed through the existing `ConstEvalEnv`
behavior.

Focused coverage now proves same-spelled local/block enum constants do not
collide in covered structured paths:

- HIR constexpr-if coverage verifies branch selection through lowerer static
  evaluation.
- HIR direct enum-expression coverage verifies the scoped inner enum value and
  the restored outer value after block exit.
- Metadata-level scoped enum coverage proves scoped metadata wins over stale
  rendered `enum_consts_` and complete scoped misses do not recover through
  rendered spelling.
- Existing positive sema enum-scope cases continue to cover local-over-global
  and no-leak-after-block behavior.

## Retained Compatibility

Rendered `enum_consts_` remains only as a no-metadata compatibility mirror for
enum references whose AST nodes lack complete TextId/key metadata. Removal
condition: all enum-reference producers carry usable TextId/key metadata.

Static-member initializer evaluation and compile-time engine state have a
separate local/block enum scoped-lifetime boundary. That work is intentionally
split into `ideas/open/168_static_member_compile_time_local_enum_lifetime.md`
instead of expanding this completed lowerer-focused route.

## Final Proof

Focused proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_hir_expr_scalar_control_helper|positive_sema_ok_enum_scope_local_over_global_c|positive_sema_ok_enum_scope_no_leak_after_block_c)$' > test_after.log
```

The final focused proof passed 5/5. An earlier focused proof before the final
metadata test covered 4/4 tests; because that earlier run used a different
focused scope, it is recorded only as historical context and not as a
matching-scope regression guard.

```text
final focused proof: passed=5 failed=0 total=5
earlier focused proof: passed=4 failed=0 total=4
```

The accepted full-suite baseline after the code slices was `test_baseline.log`
with 3135 passed and 0 failed.

## Original In Scope

- Inventory local and block enum paths that feed static-eval enum constants
  through rendered compatibility maps or mutable mirror state.
- Identify which local/block paths can carry structured enum-domain metadata,
  scoped `TextId` metadata, or neither.
- Convert metadata-capable local/block paths to structured lookup without
  fabricating incomplete enum identity.
- Keep no-metadata paths on an explicit compatibility bridge with a written
  owner, limitation, and removal condition.
- Add focused tests where same-spelled local/block enum constants must not
  collide during static evaluation when structured metadata is complete.

## Original Out Of Scope

- Reopening global/static-member lookup conversion completed by idea 164.
- Reworking the full consteval interpreter.
- Removing diagnostic or source spelling strings.
- Broad backend or ABI changes.

## Acceptance Criteria

- Local/block enum static-eval callers are inventoried and classified by
  structured metadata capability.
- Metadata-capable local/block paths use structured or scoped TextId-aware
  lookup and fail closed on complete structured misses.
- Any retained mutable rendered mirror or string map path is documented as
  compatibility with a removal condition.
- Focused tests prove local/block same-spelled enum constants do not collide in
  covered structured paths.

## Reviewer Reject Signals

- The change only renames or relocates the mutable `enum_consts_` mirror while
  preserving rendered spelling as ordinary local/block enum authority.
- A raw `TextId` is treated as sufficient for local/block enum lookup without
  enough enum/domain context to distinguish scopes.
- Tests prove only a named testcase while nearby local/block same-spelled enum
  cases remain unexamined.
- Unsupported expectations are downgraded or weakened to avoid local/block
  enum-scope behavior.
- The route reopens idea 164's global/static-member conversion instead of
  focusing on local/block ownership and lifetime.
