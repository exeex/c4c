Status: Active
Source Idea Path: ideas/open/135_sema_structured_owner_static_member_lookup_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Make Namespace Owner Resolution Structured-Primary

# Current Packet

## Just Finished

Plan Step 2 `Make Namespace Owner Resolution Structured-Primary` completed for
`resolve_owner_in_namespace_context` and out-of-class method owner setup.

Namespace owner resolution now indexes record definitions by
`SemaStructuredNameKey`, derives an owner key from the method qualifier
`TextId` when the qualifier segment matches the parsed owner, and returns the
structured record match before considering the rendered-name path. The old
unqualified-name/suffix comparison path remains isolated as
`resolve_owner_by_rendered_name_fallback` and is used only when structured
owner identity is absent.

`validate_function` now seeds `current_method_struct_key_` from the resolved
owner record when available, so namespace owner context and `this` binding can
use the AST/TextId-derived record identity rather than reparsed owner spelling.

Added `test_sema_namespace_owner_resolution_prefers_structured_owner_key` to
`frontend_parser_tests`. The fixture gives an out-of-class method a misleading
rendered owner spelling (`wrong::Owner::self`) while its structured qualifier
`TextId` points at `real::Owner`; rendered-primary resolution would fail to
bind `this`, while structured-primary resolution validates.

## Suggested Next

Next coherent packet: continue with the next supervisor-selected owner cleanup
slice, likely the remaining HIR or downstream owner paths that still depend on
rendered owner strings after sema has selected a structured record.

## Watchouts

`method_owner_key_from_qualifier` deliberately refuses to build a key if the
last qualifier segment does not name the parsed owner. This preserves the
existing compatibility path for parsed operator/function shapes where qualifier
metadata is present but is not an owner carrier.

`enclosing_method_owner_struct` still returns rendered strings for legacy
callers; the structured record path is carried separately through
`enclosing_method_owner_record` and `current_method_struct_key_`.

## Proof

Ran the supervisor-selected proof exactly:

`cmake --build build --target c4cll frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_out_of_class_member_owner_scope_parse_cpp|cpp_positive_sema_namespace_cross_namespace_lookup_parse_cpp|cpp_positive_sema_namespace_cross_type_reference_runtime_cpp|cpp_positive_sema_namespace_nested_runtime_cpp|cpp_positive_sema_namespace_struct_collision_runtime_cpp|cpp_positive_sema_namespace_struct_runtime_cpp|cpp_positive_sema_namespace_struct_type_basic_cpp|cpp_positive_sema_namespace_template_struct_basic_cpp|cpp_positive_sema_template_member_owner_resolution_cpp|cpp_positive_sema_inherited_static_member_lookup_runtime_cpp|cpp_positive_sema_unqualified_static_member_call_runtime_cpp)$' >> test_after.log 2>&1`

Result: build succeeded and CTest reported 14/14 passing. Proof log:
`test_after.log`.
