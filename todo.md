# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the deletion-probe
blocker in `src/frontend/hir/impl/stmt/range_for.cpp`. Range-for lowering now
resolves range and iterator method owners from structured record owner keys,
`tag_text_id`/namespace metadata, and structured template realization before
method lookup.

When a range or iterator type has structured owner identity, the local
range-for lookup no longer falls back to rendered `TypeSpec::tag` spelling
after the structured owner miss. The remaining rendered/text path is limited to
no-structured-identity compatibility and diagnostics.

## Suggested Next

Return Step 4 to the next deletion-probe blocker in
`src/frontend/hir/impl/stmt/stmt.cpp`, starting with defaulted method/member
destructor TypeSpec owner construction and field destructor recursion. Preserve
structured owner semantics and split template-deduction/global TypeSpec
template-parameter binding residuals into later packets unless the supervisor
routes them together.

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
- The current deletion probe moves past `range_for.cpp`. The first residual
  errors are now in `src/frontend/hir/impl/stmt/stmt.cpp`; parallel build output
  also reports `src/frontend/hir/impl/templates/deduction.cpp` and
  `src/frontend/hir/impl/templates/global.cpp`.
- Non-canonical deletion probe artifacts for recent packets include
  `/tmp/c4c_typespec_tag_deletion_probe_step4_call_expr.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_object.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_object_next.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_object_new_expr.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_delete_expr.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_operator.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_decl.log`, and
  `/tmp/c4c_typespec_tag_deletion_probe_step4_decl_direct_agg.log`.
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_range_for.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 after restoring the temporary deletion-probe edit, and
`test_after.log` was preserved as the canonical executor proof log. The build
passed, and CTest passed 72 of 72 delegated tests.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_range_for.log 2>&1`, and restored
the temporary edit. The probe moved past
`src/frontend/hir/impl/stmt/range_for.cpp`. The first residual errors are direct
`TypeSpec::tag` reads in `src/frontend/hir/impl/stmt/stmt.cpp`, with same-build
residuals in `src/frontend/hir/impl/templates/deduction.cpp` and
`src/frontend/hir/impl/templates/global.cpp`.

Result: command exited 1 as expected for the controlled deletion probe, and the
normal build proof above is green after reverting the temporary edit.
