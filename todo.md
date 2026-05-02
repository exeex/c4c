# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared/classified the
`src/frontend/hir/hir_lowering_core.cpp` residual deletion-probe cluster:

- `generic_type_compatible` now treats complete structured record owner keys as
  authoritative, rejects a one-sided complete-key mismatch, and retains
  rendered `TypeSpec::tag` comparison only as an explicit no-complete-metadata
  compatibility bridge.
- `LayoutQueries::find_struct_def_for_layout_type` keeps its rendered
  `TypeSpec::tag` lookup explicitly classified as a legacy TypeSpec producer
  bridge after complete owner-key metadata is absent.
- base-layout sizing/alignment no longer synthesizes `base_ts.tag`; it resolves
  base records directly through `base_tag_text_ids` plus the containing
  namespace owner key, with rendered `base_tags` lookup retained only for base
  metadata that lacks a complete structured owner key.

## Suggested Next

Continue Step 4 with one bounded frontend/HIR packet for the next fresh
deletion-probe cluster outside `hir_lowering_core.cpp`, likely
`src/frontend/hir/hir_functions.cpp` callable compatibility scaffolding or
`src/frontend/hir/hir_build.cpp` ref-overload no-owner-key fallback, per the
supervisor's subset choice.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- The callable zero-sized-return path should keep rendered `TypeSpec::tag`
  compatibility only when no complete structured owner metadata exists.
- The ref-overload grouping path now has the same no-complete-metadata rendered
  fallback boundary; do not reintroduce tag comparison after complete owner-key
  mismatch.
- The member-symbol lookup path now treats complete owner/member metadata as
  authoritative; stale rendered owner symbols are not a fallback after a
  complete miss, but `HirStructDef` fields remain a structured hit source when
  the owner lookup mirror is absent.
- The generic `NK_MEMBER` inference path now delegates owner lookup to the
  layout resolver; do not reintroduce direct `struct_defs[TypeSpec::tag]`
  lookup when complete `record_def` or TextId owner metadata is present.
- The typedef-to-struct resolver now treats complete `record_def` and TextId
  owner metadata as authoritative; keep rendered `TypeSpec::tag` fallback only
  for no-complete-metadata compatibility.
- `hir_lowering_core.cpp` still has two intentional rendered `TypeSpec::tag`
  reads: generic record compatibility and local layout TypeSpec lookup. Both
  are classified no-complete-metadata compatibility bridges, not semantic
  owner keys.
- The base layout path still uses `HirStructDef::base_tags` as final spelling
  and as no-complete-metadata compatibility input; it no longer writes
  `TypeSpec::tag`.
- The latest deletion probe still sees residual compile blockers in other
  previously touched clusters. Treat them as fresh inventory, not as proof that
  the older migrations should be reverted.
- `hir_functions.cpp` is currently an unintegrated draft file in the build; its
  `compatibility_type_tag()` helper may be compatibility scaffolding, but it
  still blocks physical deletion and needs either structured replacement or
  explicit classification.
- `hir_build.cpp:559` is a no-owner-key rendered fallback in the ref-overload
  grouping path. Do not remove the complete-owner mismatch guard while clearing
  the compile blocker.
- `hir_types.cpp` still contains both semantic-looking lookup reads and
  compatibility/final-spelling payload writes. Split those before editing so
  ABI/link-visible names, diagnostics, dumps, and base-tag storage are not
  accidentally collapsed into owner-key semantics.
- `hir/impl/expr/call.cpp` remains a later first-wave HIR cluster after the
  top-level HIR files compile.
- Step 4 is a probe, not the final field removal. Do not commit a broken
  deletion build.
- Do not create downstream follow-up ideas until a probe reaches LIR/BIR/backend
  failures after frontend/HIR compile blockers are cleared.
- Classify each failure as parser/HIR-owned, compatibility/display/final
  spelling, or downstream metadata gap instead of fixing broad clusters inside
  the probe packet.
- Do not create new follow-up ideas for parser/HIR work that still belongs in
  this runbook.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Keep downstream LIR/BIR/backend carrier gaps as separate follow-up ideas
  instead of broadening this runbook.
- Treat any `TypeSpec::tag` deletion build as temporary until Step 5.
- `hir_lowering_core.cpp` retained rendered-tag fallback reads only for
  no-complete-metadata compatibility; do not treat those as semantic authority.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was preserved as the canonical
executor proof log. The build passed; CTest ran 73 HIR tests and all passed.
