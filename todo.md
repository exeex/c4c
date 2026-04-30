Status: Active
Source Idea Path: ideas/open/137_parser_known_function_name_compatibility_spelling_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Structured Lookup and Disambiguation Paths

# Current Packet

## Just Finished

Completed the first Plan Step 3 lookup/disambiguation packet. Unqualified
visible value/type-head classification now probes structured direct
known-function keys, structured current-record member keys, and structured
`VisibleNameResult` keys before falling back to rendered `head_name`,
`current_member_name`, or visible-name spelling. Rendered string lookup remains
an explicit compatibility fallback only when no structured known-function key
was available.

## Suggested Next

Step 3 next packet: inspect the remaining visible value/type and qualified
lookup paths for string projection bridges that still make type/value
classification decisions, then convert the next narrow site to structured key
authority before spelling fallback.

## Watchouts

- Keep source-idea intent stable; routine findings belong in this file.
- Do not let rendered spelling decide disambiguation when structured identity
  is available.
- Avoid testcase-shaped special cases.
- Do not remove `fn->name` rendered spelling yet; it is final AST/display data,
  not the known-name authority store.
- `current_record_member_name_key` intentionally uses existing parser-owned
  record/member `TextId`s and existing qualifier paths; it does not intern a new
  path just to make a rendered current-member spelling authoritative.

## Proof

Passed: `cmake --build build --target c4cll frontend_parser_tests >
test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R
'^(frontend_parser_tests|cpp_positive_sema_qualified_known_type_arg_fast_path_parse_cpp|cpp_positive_sema_qualified_type_start_probe_parse_cpp|cpp_positive_sema_qualified_type_resolution_dispatch_parse_cpp|cpp_positive_sema_qualified_type_spelling_shared_parse_cpp|cpp_positive_sema_qualified_type_start_shared_probe_parse_cpp|cpp_positive_sema_qualified_global_type_start_shared_probe_parse_cpp|cpp_positive_sema_qualified_template_call_expr_parse_cpp|cpp_positive_sema_template_known_type_arg_fast_path_parse_cpp|cpp_positive_sema_template_visible_typedef_type_arg_parse_cpp|cpp_positive_sema____generated_parser_disambiguation_matrix_parse_only_owner_global_qualified__decl_function_lvalue_ref__ctx_ambiguous_statement_context__parse_only_cpp|cpp_positive_sema____generated_parser_disambiguation_matrix_parse_only_owner_qualified__decl_function_lvalue_ref__ctx_ambiguous_statement_context__parse_only_cpp|cpp_positive_sema____generated_parser_disambiguation_matrix_parse_only_owner_simple__decl_function_lvalue_ref__ctx_ambiguous_statement_context__parse_only_cpp)$'
>> test_after.log 2>&1`. The proof rebuilt `c4cll` and
`frontend_parser_tests`, then passed 13/13 selected tests. Proof log:
`test_after.log`.
