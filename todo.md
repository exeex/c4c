# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.5C
Current Step Title: Add Dependent/Template Record Member-Typedef Carrier

## Just Finished

The Step 2.4.4.6 deletion attempt confirmed that the remaining parser
member-typedef mirror is
`register_record_member_typedef_bindings()` in
`src/frontend/parser/impl/types/struct.cpp`: for template/dependent record
contexts it still publishes `source_tag::member` through
`register_typedef_binding()`. A direct deletion attempt was reverted because
the delegated parser/Sema subset timed out in three covered positive parse
cases, so this writer is still live semantic compatibility rather than a
deletable non-semantic cache.

## Suggested Next

Execute new Step 2.4.4.5C before retrying Step 2.4.4.6. The next packet should
add or thread a structured parser/Sema carrier for template/dependent record
member typedef availability currently provided by
`register_record_member_typedef_bindings()`, targeting the route exercised by
`cpp_positive_sema_eastl_slice7_piecewise_ctor_parse_cpp`,
`cpp_positive_sema_step3_timeout_probe_baseline_parse_cpp`, and
`cpp_positive_sema_tuple_element_alias_mix_parse_cpp`, then re-attempt removal
of the `source_tag::member` publication only after that route has structured
metadata.

## Watchouts

- Do not replace the blocker with another rendered-key reconstruction or local
  `owner::member` string split. The missing carrier is for template/dependent
  record member typedef availability currently provided by the rendered
  typedef binding.
- The structured concrete-instantiation carrier
  `template_instantiation_member_typedefs_by_key` and alias-template
  `ParserAliasTemplateInfo::member_typedef` are not sufficient for the three
  timed-out parse fixtures.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Failed after the deletion attempt: 925/928 tests passed; the three failures
were 30s parse timeouts in
`cpp_positive_sema_eastl_slice7_piecewise_ctor_parse_cpp`,
`cpp_positive_sema_step3_timeout_probe_baseline_parse_cpp`, and
`cpp_positive_sema_tuple_element_alias_mix_parse_cpp`.
Canonical proof log: `test_after.log`.
