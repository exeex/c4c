# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the first residual
`src/frontend/hir/impl/templates/global.cpp` deletion-probe blocker away from
direct semantic `TypeSpec::tag` use.

Template global instantiation now substitutes forwarded enclosing template type
arguments through `FunctionCtx::tpl_bindings_by_text` using
`template_param_text_id`/`tag_text_id` carriers while preserving outer
pointer/ref/cv modifiers. Deferred member typedef resolution in the same path
uses structured `record_def` owner keys or template-origin materialization when
present. Complete structured misses do not fall back to rendered
`TypeSpec::tag` spelling.

## Suggested Next

Return Step 4 to the next deletion-probe blocker in
`src/frontend/hir/impl/templates/member_typedef.cpp`, starting at
`resolve_deferred_member_typedef_type` line 119 where the final owner fallback
still probes `owner_ts.tag`. Keep the parallel template materialization and
struct instantiation residuals split unless the supervisor routes those
template families together.

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
  `stmt.cpp`, `src/frontend/hir/impl/templates/deduction.cpp`, and
  `src/frontend/hir/impl/templates/global.cpp`. The first residual errors are
  now in `src/frontend/hir/impl/templates/member_typedef.cpp`; parallel build
  output also reports
  `src/frontend/hir/impl/templates/materialization.cpp`,
  `src/frontend/hir/impl/templates/struct_instantiation.cpp`, and
  `src/frontend/hir/impl/templates/templates.cpp`.
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
- This packet added `/tmp/c4c_typespec_tag_deletion_probe_step4_global.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 after restoring the temporary deletion-probe edit. The
build passed, and CTest passed 72 of 72 delegated tests.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_global.log 2>&1`, and restored the
temporary edit. The probe moved past
`src/frontend/hir/impl/templates/global.cpp`. The first residual errors are
direct `TypeSpec::tag` reads in
`src/frontend/hir/impl/templates/member_typedef.cpp:119` and `:125`, with
same-build residuals in
`src/frontend/hir/impl/templates/materialization.cpp`,
`src/frontend/hir/impl/templates/struct_instantiation.cpp`, and
`src/frontend/hir/impl/templates/templates.cpp`.

Result: command exited 1 as expected for the controlled deletion probe, and the
normal build proof above is green after reverting the temporary edit.
