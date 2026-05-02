# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the first residual
`src/frontend/hir/impl/stmt/stmt.cpp` deletion-probe blocker away from direct
semantic `TypeSpec::tag` use.

Defaulted method/member destructor owner construction now populates `this`
`TypeSpec` values from HIR `record_def`, owner-key, `tag_text_id`, and namespace
metadata via `populate_struct_owner_typespec`; nested field destructor recursion
now resolves the field record owner through structured metadata only. Complete
structured misses do not fall back to rendered `TypeSpec::tag` spelling. Any
remaining `tag` write in this family is deletion-safe final spelling payload via
`set_typespec_final_spelling_tag_if_present`.

## Suggested Next

Return Step 4 to the next deletion-probe blocker in
`src/frontend/hir/impl/templates/deduction.cpp`, starting with
`typespec_rendered_tags_match_for_deduction` and template-parameter binding
lookups that still read `TypeSpec::tag`. Keep the template global and template
materialization residuals split unless the supervisor routes those template
families together.

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
- The current deletion probe moves past `decl.cpp`, `range_for.cpp`, and
  `stmt.cpp`. The first residual errors are now in
  `src/frontend/hir/impl/templates/deduction.cpp`; parallel build output also
  reports `src/frontend/hir/impl/templates/global.cpp` and
  `src/frontend/hir/impl/templates/materialization.cpp`.
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
- This packet added `/tmp/c4c_typespec_tag_deletion_probe_step4_stmt.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|cpp_hir_.*|llvm_gcc_c_torture_src_20090113_1_c)$"' > test_after.log 2>&1`

Result: command exited 0 after restoring the temporary deletion-probe edit, and
`test_after.log` was preserved as the canonical executor proof log. The build
passed, and CTest passed 73 of 73 delegated tests, including
`llvm_gcc_c_torture_src_20090113_1_c`.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_stmt.log 2>&1`, and restored the
temporary edit. The probe moved past
`src/frontend/hir/impl/stmt/decl.cpp`,
`src/frontend/hir/impl/stmt/range_for.cpp`, and
`src/frontend/hir/impl/stmt/stmt.cpp`. The first residual errors are direct
`TypeSpec::tag` reads in `src/frontend/hir/impl/templates/deduction.cpp`, with
same-build residuals in `src/frontend/hir/impl/templates/global.cpp` and
`src/frontend/hir/impl/templates/materialization.cpp`.

Result: command exited 1 as expected for the controlled deletion probe, and the
normal build proof above is green after reverting the temporary edit.
