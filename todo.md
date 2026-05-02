# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the
`src/frontend/hir/impl/templates/materialization.cpp`
`Lowerer::build_template_mangled_name` `append_type_suffix`
struct/union/enum/typedef final-spelling payload family away from direct
`TypeSpec::tag` reads.

The local mangling helper now prefers structured `record_def` names and
TextId/namespace metadata for deletion-safe suffix payloads before falling back
through the existing legacy display-spelling shim. This keeps mangling/final
spelling string-shaped without making rendered text semantic lookup authority.

## Suggested Next

Continue Step 4 with the first deletion-probe blocker now outside the owned
file: `src/frontend/hir/impl/templates/struct_instantiation.cpp`
`apply_template_typedef_bindings` direct `TypeSpec::tag` reads/writes around
current lines 9 and 11. Keep `templates.cpp` and `type_resolution.cpp` split
unless the supervisor routes them together.

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
  `src/frontend/hir/impl/templates/global.cpp`, and
  `src/frontend/hir/impl/templates/member_typedef.cpp`. The first same-file
  residual errors are now the materialization mangling payload reads in
  `src/frontend/hir/impl/templates/materialization.cpp`; parallel build output
  also reports
  `src/frontend/hir/impl/templates/struct_instantiation.cpp`,
  `src/frontend/hir/impl/templates/templates.cpp`, and
  `src/frontend/hir/impl/templates/type_resolution.cpp`.
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
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_materialization_mangle.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 after restoring the temporary deletion-probe edit. The
build passed, and CTest passed 72 of 72 delegated tests.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_materialization_mangle.log 2>&1`,
and restored the temporary edit. The probe moved past the targeted
`Lowerer::build_template_mangled_name` `append_type_suffix` struct/union/enum/
typedef reads in `src/frontend/hir/impl/templates/materialization.cpp`.
`materialization.cpp` compiled under the deletion probe. The first residual
blocker is now
`src/frontend/hir/impl/templates/struct_instantiation.cpp:9` and `:11`
direct `TypeSpec::tag` use in `apply_template_typedef_bindings`, with
same-build residuals also reported in
`src/frontend/hir/impl/templates/templates.cpp` and
`src/frontend/hir/impl/templates/type_resolution.cpp`.

Result: command exited 1 as expected for the controlled deletion probe, and the
normal build proof above is green after reverting the temporary edit.
