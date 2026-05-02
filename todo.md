# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the bounded
frontend/HIR generic member-inference cluster in
`src/frontend/hir/hir_types.cpp`.

`Lowerer::infer_generic_ctrl_type` now resolves `NK_MEMBER` field types through
`find_struct_def_for_layout_type(base_ts)` instead of looking up
`module_->struct_defs` by rendered `TypeSpec::tag`. Complete `record_def` and
TextId-backed owner metadata are therefore authoritative for member field-type
inference; complete structured misses do not infer from stale rendered owner
tags, while the existing layout resolver still preserves rendered-tag
compatibility when no complete structured owner metadata exists. Focused
`frontend_hir_tests` coverage now proves record_def hit, TextId hit, and TextId
miss rejection against stale rendered owner spelling.

## Suggested Next

Continue Step 4 with the next bounded frontend/HIR deletion-probe cluster from
the remaining inventory. The callable helper blocker in `hir_functions.cpp` and
the standalone layout helper cluster in `hir_lowering_core.cpp` are cleared, and
the bounded ref-overload grouping path in `hir_build.cpp` plus the
member-symbol lookup path and generic member-inference path in
`hir_types.cpp` are now migrated. Remaining probe inventory still includes
parser-owned semantic producers/consumers and first-failure HIR clusters in
`src/frontend/hir/impl/compile_time/compile_time_engine.hpp`,
`src/frontend/hir/hir_types.cpp`, `src/frontend/hir/hir_ir.hpp`,
`src/frontend/hir/impl/expr/builtin.cpp`, and
other `src/frontend/hir/hir_build.cpp` rendered-name compatibility paths.

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
- `hir_lowering_core.cpp` still intentionally contains rendered-tag fallback
  reads for no-metadata compatibility; do not treat those as semantic authority
  unless the structured owner-key probe lacks complete metadata.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was preserved as the canonical
executor proof log. The build passed; CTest ran 73 HIR tests and all passed.
