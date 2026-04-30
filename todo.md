# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.5A
Current Step Title: Add Alias-Template Member-Typedef Carrier

## Just Finished

Lifecycle review split Step 2.4.4.5 because the bridge deletion found a
parser/Sema-owned missing carrier rather than an outside metadata boundary.
The failed execution attempt deleted the remaining dependent/template
member-typedef compatibility bridge in
`register_record_member_typedef_bindings`, but the exact missing structured
carrier is still at alias-template registration: `ParserAliasTemplateInfo`
stores only `TypeSpec aliased_type` for
`typename Owner<Args>::member`, and that `TypeSpec` preserves the dependent
owner/member through rendered/deferred fields (`tag`/`tpl_struct_origin`,
template-arg refs, and `deferred_member_type_name`) instead of a structured
owner `QualifiedNameKey`, concrete argument keys/refs, and member `TextId`.
Deleting the bridge leaves alias instantiation unable to resolve
primary/specialization member typedefs for `Owner<Args>::member` without
falling back to rendered `Owner::member`. The active next route is now
Step 2.4.4.5A, which adds structured alias-template member-typedef metadata;
bridge deletion moves to Step 2.4.4.5B after that carrier lands.

## Suggested Next

Execute Step 2.4.4.5A: add a structured alias member-typedef carrier to
`ParserAliasTemplateInfo` or an equivalent parser/Sema alias-template metadata
surface for `typename Owner<Args>::member`: owner `QualifiedNameKey`,
substituted template argument refs/keys, and member `TextId`. Populate it
while parsing alias templates, then make alias instantiation resolve the
selected primary/specialization member typedef from that carrier. Do not retry
bridge deletion until this carrier is proven.

## Watchouts

Deleting the dependent/template bridge still regresses
`cpp_positive_sema_eastl_slice7_piecewise_ctor_parse_cpp`,
`cpp_positive_sema_step3_timeout_probe_baseline_parse_cpp`,
`cpp_positive_sema_tuple_element_alias_mix_parse_cpp`, and
`cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`.
An exploratory attempt to let the alias resolver consume the existing deferred
`TypeSpec` owner/member fields did not change that failure set, confirming the
blocker is the missing structured alias carrier rather than a single skipped
reader branch. Do not replace this with a helper that renders, splits, or
reparses `Owner::member`.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Exploratory bridge-deletion proof failed with the four regressions listed in
Watchouts; source changes were reverted because Step 2.4.4.5 is blocked on the
missing structured alias carrier. No acceptance proof is claimed for this
blocked todo-only handoff.
