Status: Active
Source Idea Path: ideas/open/137_parser_known_function_name_compatibility_spelling_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Structured Registration Paths

# Current Packet

## Just Finished

Completed the first Plan Step 2 packet. The common
`register_decl_known_fn_name` declaration path now asks
`register_known_fn_name_in_context()` to derive and register a structured
`QualifiedNameKey` before any rendered `scoped_decl_name` fallback. The context
registration helper now reports success and tries the structured/context key
before its rendered qualified-name compatibility fallback. `fn->name` and
`scoped_decl_name` remain final spelling/display data.

## Suggested Next

Step 2 next packet: convert the out-of-class operator declaration registration
path away from rendered `qualified_op_name` authority while preserving final
display spelling.

## Watchouts

- Keep source-idea intent stable; routine findings belong in this file.
- Do not let rendered spelling decide disambiguation when structured identity
  is available.
- Avoid testcase-shaped special cases.
- Out-of-class operator and constructor registration are still follow-up
  packets; they need owner-name structure preserved instead of reparsing
  `qualified_op_name`/`qualified_ctor_name`.
- Do not remove `fn->name` rendered spelling yet; it is final AST/display data,
  not the known-name authority store.

## Proof

Passed: `cmake --build build --target c4cll frontend_parser_tests >
test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R
'^(frontend_parser_tests|cpp_positive_sema_namespace_function_call_runtime_cpp|cpp_positive_sema_namespaced_function_param_shadow_frontend_cpp|cpp_positive_sema_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_out_of_class_member_owner_scope_parse_cpp|cpp_positive_sema_namespace_global_qualified_self_parse_cpp|cpp_positive_sema_qualified_type_start_probe_parse_cpp|cpp_positive_sema_qualified_type_resolution_dispatch_parse_cpp|cpp_positive_sema_qualified_type_spelling_shared_parse_cpp|cpp_positive_sema_qualified_template_call_expr_parse_cpp|cpp_positive_sema_qualified_template_call_template_arg_parse_cpp|cpp_positive_sema_forward_declared_self_type_local_qualified_parse_cpp)$'
>> test_after.log 2>&1`. The proof rebuilt `c4cll` and
`frontend_parser_tests`, then passed 13/13 selected tests. Proof log:
`test_after.log`.
