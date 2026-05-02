# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated
`src/frontend/hir/impl/templates/struct_instantiation.cpp`
`Lowerer::append_instantiated_template_struct_bases` away from direct
`TypeSpec::tag` reads. Instantiated template base registration now derives the
base tag payload from structured record owner metadata, resolved base layouts,
`record_def`, or `tag_text_id` carriers, and uses that payload only for member
typedef compatibility resolution and final HIR base-tag storage.

## Suggested Next

Continue Step 4 with the next deletion-probe blocker in
`src/frontend/hir/impl/templates/templates.cpp`, starting with
`encode_template_type_arg_ref_hir` direct `TypeSpec::tag` reads around current
lines 128-157. Keep the parallel `type_resolution.cpp`, parser/core, parser
type-helper, and value-arg residuals split unless the supervisor routes them
together.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- Step 4 is a probe, not the final field removal. Do not commit a broken
  deletion build.
- Classify each failure as parser/HIR-owned, compatibility/display/final
  spelling, or downstream metadata gap instead of fixing broad clusters inside
  the probe packet.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Treat any `TypeSpec::tag` deletion build as temporary until Step 5.
- `range_for.cpp` is semantically cleared for the direct range/iterator method
  owner family. It still uses canonical HIR struct tags as registered method
  payloads after structured owner resolution, not `TypeSpec::tag` as semantic
  input.
- `stmt.cpp` is semantically cleared for the defaulted method/member destructor
  owner family. It still uses canonical HIR struct tags as destructor-map and
  constructor-map keys after structured owner resolution, not
  `TypeSpec::tag` as semantic input.
- The current deletion probe moves past `decl.cpp`, `range_for.cpp`,
  `stmt.cpp`, `src/frontend/hir/impl/templates/deduction.cpp`,
  `src/frontend/hir/impl/templates/global.cpp`,
  `src/frontend/hir/impl/templates/member_typedef.cpp`, and the targeted
  `apply_template_typedef_bindings` reads/writes in
  `src/frontend/hir/impl/templates/struct_instantiation.cpp`.
  `append_instantiated_template_struct_bases` is now semantically cleared for
  base tag/member typedef payloads. The deletion probe builds
  `struct_instantiation.cpp.o` successfully; first residual errors now start in
  `src/frontend/hir/impl/templates/templates.cpp`, with same-build residuals
  also reported in `src/frontend/hir/impl/templates/type_resolution.cpp`,
  parser/core, and parser type helpers.
- Non-canonical deletion probe artifacts for recent packets include
  `/tmp/c4c_typespec_tag_deletion_probe_step4_call_expr.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_object.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_object_next.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_object_new_expr.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_delete_expr.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_operator.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_decl.log`, and
  `/tmp/c4c_typespec_tag_deletion_probe_step4_decl_direct_agg.log`.
- Recent packets added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_decl_regression.log`.
- Recent packets added `/tmp/c4c_typespec_tag_deletion_probe_step4_stmt.log`.
- Recent packets added `/tmp/c4c_typespec_tag_deletion_probe_step4_deduction.log`.
- Recent packets added `/tmp/c4c_typespec_tag_deletion_probe_step4_global.log`.
- Recent packets added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_member_typedef.log`.
- Recent packets added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_materialization.log`.
- Recent packets added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_materialization_decode.log`.
- Recent packets added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_materialization_mangle.log`.
- Recent packets added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_struct_instantiation.log`.
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_struct_instantiation_bases.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 74 of 74 delegated
tests.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_struct_instantiation_bases.log 2>&1`,
and restored the temporary edit. The probe moved past the targeted
`append_instantiated_template_struct_bases` direct `TypeSpec::tag` reads; the
build reached `src/frontend/hir/impl/templates/struct_instantiation.cpp.o`
without errors. The first residual blocker is now
`src/frontend/hir/impl/templates/templates.cpp:128` and following direct
`TypeSpec::tag` use in `encode_template_type_arg_ref_hir`, with same-build
residuals also reported in
`src/frontend/hir/impl/templates/type_resolution.cpp`, parser/core, and parser
type helpers.

Result: command exited 1 as expected for the controlled deletion probe, and the
normal build proof above is green after reverting the temporary edit.
