Status: Active
Source Idea Path: ideas/open/137_parser_known_function_name_compatibility_spelling_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Quarantine or Remove String Compatibility Overloads

# Current Packet

## Just Finished

Completed Plan Step 4 known-function string compatibility quarantine.
Production parser callers now use `has_known_fn_name_compatibility_fallback`
and `register_known_fn_name_compatibility_fallback` for rendered-spelling
known-function fallback paths. The original `std::string` overloads remain as
public compatibility shims only, delegating to the named compatibility helpers.
Fallback behavior remains for TextId-less rendered qualified names in
`register_known_fn_name_in_context`, declaration recovery for scoped function
names, out-of-class operator/constructor registration when structured keys
cannot be built, and unqualified visible value/type-head classification only
when no structured known-function key was available.

## Suggested Next

Supervisor should review whether Step 4 is complete enough to close or whether
another narrow packet should migrate public/test-only string callers to the
named compatibility helpers while preserving source compatibility.

## Watchouts

- Keep source-idea intent stable; routine findings belong in this file.
- Do not let rendered spelling decide disambiguation when structured identity
  is available.
- Avoid testcase-shaped special cases.
- Do not remove `fn->name` rendered spelling yet; it is final AST/display data,
  not the known-name authority store.
- Remaining compatibility fallback storage is intentional for TextId-less
  rendered bridges and public string API compatibility; new parser-owned
  semantic paths should continue to use `QualifiedNameKey`/`TextId` identity.

## Proof

Passed: `cmake --build build --target c4cll frontend_parser_tests >
test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R
'^(frontend_parser_tests|cpp_positive_sema_qualified_known_type_arg_fast_path_parse_cpp|cpp_positive_sema_qualified_type_start_probe_parse_cpp|cpp_positive_sema_qualified_type_resolution_dispatch_parse_cpp|cpp_positive_sema_qualified_template_call_expr_parse_cpp|cpp_positive_sema_template_known_type_arg_fast_path_parse_cpp|cpp_positive_sema_namespace_function_call_runtime_cpp|cpp_positive_sema_operator_conversion_out_of_class_parse_cpp|cpp_positive_sema_cpp20_out_of_class_trailing_requires_runtime_cpp)$'
>> test_after.log 2>&1`. The proof rebuilt `c4cll` and
`frontend_parser_tests`, then passed 9/9 selected tests. Proof log:
`test_after.log`.
