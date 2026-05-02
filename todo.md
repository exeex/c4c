# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the first
`src/frontend/hir/hir_types.cpp` deletion-probe blocker in
`resolve_typedef_to_struct`. Typedef-to-record resolution now applies
structured `record_def` and TextId owner metadata before any compatibility
bridge, restores final spelling through a deletion-safe payload helper, and
keeps rendered tag lookup only as a no-complete-metadata fallback.

## Suggested Next

Continue Step 4 by taking the next first deletion-probe blocker in
`src/frontend/hir/hir_types.cpp`. The current probe first reports direct
`TypeSpec::tag` use in `resolve_struct_method_lookup_owner_tag` around
`hir_types.cpp:608`, followed by the struct-method parity helper and
layout/member compatibility surfaces.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- `hir_functions.cpp` no longer directly reads `TypeSpec::tag` in the targeted
  helper/member-typedef cluster.
- Qualified dependent member typedefs with a unique template owner/member are
  now resolved structurally from parser TextIds and current template bindings;
  keep `member_typedef_text_ids` as the member carrier and do not replace that
  with rendered owner-name lookup.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- The callable zero-sized-return path should keep rendered `TypeSpec::tag`
  compatibility only when no complete structured owner metadata exists.
- The ref-overload grouping path no longer reads rendered `TypeSpec::tag`.
  It uses `record_def` and TextId-backed type-name identity before the bounded
  no-structured-metadata compatibility bridge.
- The member-symbol lookup path now treats complete owner/member metadata as
  authoritative; stale rendered owner symbols are not a fallback after a
  complete miss, but `HirStructDef` fields remain a structured hit source when
  the owner lookup mirror is absent.
- The generic `NK_MEMBER` inference path now delegates owner lookup to the
  layout resolver; do not reintroduce direct `struct_defs[TypeSpec::tag]`
  lookup when complete `record_def` or TextId owner metadata is present.
- The typedef-to-struct resolver now treats complete `record_def` and TextId
  owner metadata as authoritative; rendered `TypeSpec::tag` is retained there
  only through deletion-safe final-spelling/no-complete-metadata compatibility
  helpers.
- `hir_lowering_core.cpp` no longer has direct `TypeSpec::tag` reads in generic
  record compatibility or local layout TypeSpec lookup.
- The base layout path still uses `HirStructDef::base_tags` as final spelling
  and as no-complete-metadata compatibility input; it no longer writes
  `TypeSpec::tag`.
- The former `hir_functions.cpp` lines 35-52 deletion-probe cluster is cleared
  of direct rendered tag reads.
- `hir_types.cpp` layout/member-base helpers now classify retained rendered
  lookup as no-complete-metadata compatibility. Base traversal should continue
  to prefer complete `base_tag_text_ids` owner metadata before rendered
  `base_tags`.
- The `hir_build.cpp` retained ref-overload deletion-probe failure is cleared.
- `hir_types.cpp` still contains lookup, parity, compatibility, and
  final-spelling payload surfaces outside the cleared layout/member-base helper
  family. Split those before editing so ABI/link-visible names, diagnostics,
  dumps, and base-tag storage are not accidentally collapsed into owner-key
  semantics.
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
- The `hir_lowering_core.cpp` and `hir_build.cpp` deletion-probe clusters are
  now clear of direct `TypeSpec::tag` reads. The first residual probe blocker
  moved past `resolve_typedef_to_struct` to
  `src/frontend/hir/hir_types.cpp:608`, followed by later `hir_types.cpp`
  surfaces and then `hir/impl/expr/call.cpp`.
- The rejected `ft.tag` layout repair route was replaced with a structured
  AST-node-to-HIR-owner carrier. Do not reintroduce rendered field type tag
  lookup for layout ownership.
- Non-canonical deletion probe artifacts for this packet:
  `/tmp/c4c_typespec_tag_deletion_probe_step4_hir_types.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was preserved as the canonical
executor proof log. The build passed, and CTest passed 73 of 73 delegated
tests: `frontend_hir_lookup_tests` and all `cpp_hir_.*` tests.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_hir_types.log 2>&1`, and restored
the temporary edit. The probe no longer reports the
`resolve_typedef_to_struct` construction/fallback cluster; the first residual
error is `resolve_struct_method_lookup_owner_tag` in
`src/frontend/hir/hir_types.cpp`, with later parallel errors in
`src/frontend/hir/impl/expr/call.cpp`.

Result: command exited 1 as expected for the controlled deletion probe, and the
normal build proof above is green after reverting the temporary edit.
