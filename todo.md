# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated
`src/frontend/hir/impl/templates/templates.cpp`
`Lowerer::realize_template_struct` away from direct `TypeSpec::tag` reads/writes
around the former deletion-probe lines 896 and 941. Realization now relies on
the canonical primary/origin carrier and materialized template args for semantic
lookup, removes the unused rendered-tag capture, and writes the realized
instance final spelling only through the deletion-friendly legacy-tag assignment
helper. The realized `HirStructDef` also refreshes `tag_text_id` and
`namespace_context_id`, keeping rendered final spelling compatibility explicit
without reintroducing semantic lookup through rendered `TypeSpec::tag`. Added
focused CTest coverage in
`tests/frontend/cpp_hir_template_realize_struct_metadata_test.cpp` proving a
stale incoming rendered `TypeSpec::tag` does not block realization and the
realized `HirStructDef` refreshes structured metadata.

## Suggested Next

Continue Step 4 with the next deletion-probe blocker in
`src/frontend/hir/impl/templates/type_resolution.cpp`, starting with the first
residual direct `TypeSpec::tag` reads at current deletion-probe line 97. Keep
parser/core and parser type-helper residuals split unless the supervisor routes
them together.

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
  `src/frontend/hir/impl/templates/member_typedef.cpp`,
  the targeted `apply_template_typedef_bindings` reads/writes and
  `append_instantiated_template_struct_bases` in
  `src/frontend/hir/impl/templates/struct_instantiation.cpp`, and the targeted
  `encode_template_type_arg_ref_hir` direct reads, and the targeted
  `eval_struct_static_member_value_hir` direct reads and the targeted
  `hir_match_type_pattern`/`hir_specialization_match_score` direct reads in
  `src/frontend/hir/impl/templates/templates.cpp`, and the targeted
  `realize_template_struct` direct reads/writes. First residual errors now
  remain in `src/frontend/hir/impl/templates/type_resolution.cpp`, with
  same-build residuals also reported in parser/core, parser declarations, and
  parser type helpers.
- `hir_match_type_pattern` and `hir_specialization_match_score` are now
  semantically cleared for type template-parameter matching/scoring. The HIR
  binding map still uses the canonical parameter-name string as the binding
  key after resolving the parameter by structured metadata; broader binding-map
  key migration is outside this packet.
- `canonical_template_struct_primary` is now semantically cleared for primary
  lookup from template origin metadata. A complete `tpl_struct_origin_key`
  selects the owner-key primary or rejects stale rendered fallback on miss; the
  final `tag_text_id` spelling path remains compatibility-only for rendered
  family-root lookup when no complete structured origin key exists.
- `realize_template_struct` is now semantically cleared for struct-template
  realization. Its remaining legacy tag assignment is a final rendered spelling
  bridge guarded by `assign_typespec_legacy_tag_if_present`; when `TypeSpec::tag`
  is removed, the helper compiles away and the structured `tag_text_id` refresh
  remains. Focused regression coverage is
  `cpp_hir_template_realize_struct_structured_metadata`.
- The explicit tag-only fallback in `hir_type_template_param_index` is gated
  behind a no-structured-carrier check and a field-detection helper so the
  Step 4 deletion probe still classifies later residuals first.
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
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_templates_encode.log`.
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_templates_static_member.log`.
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_templates_pattern_match.log`.
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_templates_canonical_primary.log`.
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_templates_realize_struct.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 after adding the focused realization metadata CTest.
The build passed, and CTest passed 79 of 79 delegated tests, including new
`cpp_hir_template_realize_struct_structured_metadata` coverage for structured
metadata refresh from the realized `HirStructDef` despite stale incoming
rendered `TypeSpec::tag`. `test_after.log` is the canonical proof log.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_templates_realize_struct.log 2>&1`,
and restored the temporary edit. The probe moved past the targeted
`realize_template_struct` direct `TypeSpec::tag` reads/writes. The first
residual blocker is now
`src/frontend/hir/impl/templates/type_resolution.cpp:97`, with additional
same-file residuals in `type_resolution.cpp` and same-build residuals also
reported in parser/core, parser declarations, and parser type helpers.

Result: command exited 1 as expected for the controlled deletion probe, and the
normal build proof above is green after reverting the temporary edit.
