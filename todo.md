# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.5
Current Step Title: Replace The Dependent/Template Member-Typedef Bridge

## Just Finished

Step 2.4.4.4, now narrowed in `plan.md`, deleted the obsolete public
`register_struct_member_typedef_binding` rendered writer/helper and its private
key builder. `register_record_member_typedef_bindings` no longer creates
generic rendered `owner::member` typedef storage for ordinary non-template
record-body member typedefs; those now publish through the structured
record/member key only. The record-body writer retains an explicit, commented
dependent/template compatibility bridge because deleting that path regressed
template alias/member-typedef parse/runtime cases.

## Suggested Next

Execute Step 2.4.4.5 by replacing the remaining dependent/template
compatibility bridge with a structured metadata carrier for template
primary/specialization member typedef lookup, or record the exact missing
carrier as a blocker before any final mirror deletion.

## Watchouts

Deleting the rendered writer for all records regressed
`cpp_positive_sema_eastl_slice7_piecewise_ctor_parse_cpp`,
`cpp_positive_sema_step3_timeout_probe_baseline_parse_cpp`,
`cpp_positive_sema_tuple_element_alias_mix_parse_cpp`, and
`cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`.
The remaining semantic consumer is the dependent/template member-typedef path:
template records need a structured carrier available during body finalization
before the post-parse template parameter attachment runs.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Passed. `test_after.log` is the proof log for the final code state.
