# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries ran a controlled deletion
probe by temporarily removing `const char* tag` from `TypeSpec` in
`src/frontend/parser/ast.hpp`, running `cmake --build --preset default`, and
then restoring the field before proof.

The deletion build failed as expected and the temporary `ast.hpp` edit was
fully reverted. Fresh first-failure clusters from
`/tmp/c4c_typespec_tag_deletion_probe.log`:

- `src/frontend/hir/hir_lowering_core.cpp`: generic type compatibility still
  falls back to rendered tags at lines 357-358; layout lookup still falls back
  through `LayoutQueries::find_struct_def_for_layout_type` at lines 403-404;
  base layout synthesis still writes `base_ts.tag` at line 544.
- `src/frontend/hir/hir_functions.cpp`: the staged callable helper file still
  has `compatibility_type_tag()` returning `ts.tag` at line 26.
- `src/frontend/hir/hir_build.cpp`: the ref-overload grouping path still has a
  rendered no-owner-key fallback at line 559.
- `src/frontend/hir/hir_types.cpp`: remaining compile blockers include typedef
  application writing `ts.tag` at line 126, the no-complete-metadata typedef
  fallback at lines 163-164, member lookup fallback/parity comparison at lines
  524 and 579-580, layout/member-base helpers at lines 1022-1038, member symbol
  fallback at lines 1725-1726, base class storage at lines 2025-2035, and
  generic/template inference or call-owner compatibility paths at lines
  2490-2722.
- `src/frontend/hir/impl/expr/call.cpp`: first-wave call lowering blockers
  remain at lines 79, 110, 228, 350, 405, 549, and 661-662.
- `src/frontend/hir/impl/compile_time/engine.cpp`,
  `src/frontend/hir/impl/compile_time/inline_expand.cpp`, and
  `src/frontend/hir/impl/expr/builtin.cpp` started in the same probe wave but
  did not emit first-failure diagnostics before Ninja stopped.

## Suggested Next

Continue Step 4 with one bounded frontend/HIR packet for the fresh
`src/frontend/hir/hir_lowering_core.cpp` residual cluster: migrate or classify
`generic_type_compatible`, `LayoutQueries::find_struct_def_for_layout_type`,
and base layout `TypeSpec` synthesis without replacing `TypeSpec::tag` with
another rendered-string semantic key.

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
- The latest deletion probe still sees residual compile blockers in previously
  touched clusters. Treat them as fresh inventory, not as proof that the older
  migrations should be reverted.
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
- `hir_lowering_core.cpp` still intentionally contains rendered-tag fallback
  reads for no-metadata compatibility; do not treat those as semantic authority
  unless the structured owner-key probe lacks complete metadata.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was preserved as the canonical
executor proof log. The build passed; CTest ran 73 HIR tests and all passed.
