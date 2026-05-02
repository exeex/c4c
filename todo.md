# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the first residual
`src/frontend/hir/impl/templates/deduction.cpp` deletion-probe blocker away from
direct semantic `TypeSpec::tag` use.

Template deduction nominal comparison now stops at `record_def` or complete
TextId/namespace identity for nominal types instead of falling back to rendered
`TypeSpec::tag` spelling. Template type-parameter deduction and explicit
template-argument rebinding now derive legacy string map keys from structured
template-parameter metadata/TextId carriers when present, with only an
unambiguous single-binding fallback for structured carriers that cannot be
spelled through the active text table. Complete structured misses do not fall
back to rendered `TypeSpec::tag` spelling.

## Suggested Next

Return Step 4 to the next deletion-probe blocker in
`src/frontend/hir/impl/templates/global.cpp`, starting at
`ensure_template_global_instance` lines 54 and 60 where `TB_TYPEDEF`
substitution still probes `ts.tag` and `ctx->tpl_bindings.find(ts.tag)`. Keep
the parallel template materialization, member typedef, and struct instantiation
residuals split unless the supervisor routes those template families together.

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
  `stmt.cpp`, and `src/frontend/hir/impl/templates/deduction.cpp`. The first
  residual errors are now in `src/frontend/hir/impl/templates/global.cpp`;
  parallel build output also reports
  `src/frontend/hir/impl/templates/materialization.cpp`,
  `src/frontend/hir/impl/templates/member_typedef.cpp`, and
  `src/frontend/hir/impl/templates/struct_instantiation.cpp`.
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
- This packet added `/tmp/c4c_typespec_tag_deletion_probe_step4_deduction.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 after restoring the temporary deletion-probe edit. The
supervisor accepted the matching before/after guard and rolled `test_after.log`
forward to `test_before.log`. The build passed, and CTest passed 72 of 72
delegated tests.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_deduction.log 2>&1`, and restored
the temporary edit. The probe moved past
`src/frontend/hir/impl/templates/deduction.cpp`. The first residual errors are
direct `TypeSpec::tag` reads in
`src/frontend/hir/impl/templates/global.cpp:54` and `:60`, with same-build
residuals in `src/frontend/hir/impl/templates/materialization.cpp`,
`src/frontend/hir/impl/templates/member_typedef.cpp`, and
`src/frontend/hir/impl/templates/struct_instantiation.cpp`.

Result: command exited 1 as expected for the controlled deletion probe, and the
normal build proof above is green after reverting the temporary edit.
