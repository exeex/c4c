Status: Active
Source Idea Path: ideas/open/136_hir_structured_record_template_lookup_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Make Out-of-Class Method Ownership Structured-Primary

# Current Packet

## Just Finished

Completed Plan Step 2 `Make Out-of-Class Method Ownership Structured-Primary`.

Added a structured out-of-class method lookup key builder in `hir_build.cpp`
that derives the owner from the function node's namespace qualifier segments
and the method from its unqualified TextId. `attach_out_of_class_struct_method_defs`
and `lower_non_method_functions_and_globals` now consult `struct_methods_by_owner_`
first and use `try_parse_qualified_struct_method_name` only as the compatibility
fallback. Added focused HIR tests covering stale/misleading rendered
`Owner::method` spelling for both method-body attachment and ordinary-function
lowering exclusion.

## Suggested Next

Next coherent packet: make the scoped static-member lookup boundary structured
primary, so `find_struct_static_member_decl` /
`find_struct_static_member_const_value` receive owner/member keys directly
when the caller has `TypeSpec.record_def` or a recoverable `HirRecordOwnerKey`.

## Watchouts

Keep HIR lookup changes structured-primary. Do not extend rendered-name
fallbacks or add testcase-shaped special cases as progress. The new
out-of-class helper intentionally requires structured qualifier metadata and an
indexed record owner before it bypasses the rendered compatibility path.

## Proof

Ran the supervisor-selected proof exactly:
`cmake --build build --target c4cll frontend_hir_tests frontend_hir_lookup_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests|frontend_parser_tests|cpp_hir_template_member_owner_resolution|cpp_hir_template_struct_inherited_method_binding|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_hir_template_member_owner_field_and_local|cpp_hir_template_member_owner_signature_local|cpp_positive_sema_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_out_of_class_member_owner_scope_parse_cpp|cpp_positive_sema_operator_this_out_of_class_runtime_cpp|cpp_positive_sema_using_namespace_struct_method_runtime_cpp|cpp_positive_sema_template_member_owner_resolution_cpp)$' >> test_after.log 2>&1`

Result: passed, 16/16 tests green. Proof log: `test_after.log`.
