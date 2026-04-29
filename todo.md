Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
with the operator HIR method-lookup caller family. `try_lower_operator_call`,
the `operator->` chaining loop in `lower_member_expr`, and
`maybe_bool_convert` now resolve structured owner identity from the relevant
`TypeSpec` carrier before calling `find_struct_method_mangled` or
`find_struct_method_return_type`, while retaining rendered-tag fallback through
the shared owner-resolution helper when structured identity is absent.

Added focused HIR coverage for the direct operator-call path where a stale
rendered object tag would select the wrong operator method and return type
unless `record_def` structured owner identity is used first, plus caller-level
coverage that the rendered fallback still works without structured owner
identity.

## Suggested Next

Continue Step 4 by auditing the remaining HIR method-lookup ingress outside
`operator.cpp`, starting with `infer_generic_ctrl_type` in `hir_types.cpp` and
`lower_member_call_expr`/call-object handling in `expr/call.cpp`, so those
callers also prefer structured `TypeSpec` owner identity before rendered tags
where available.

## Watchouts

`resolve_struct_method_lookup_owner_tag` currently reuses the member-owner
resolver because both method and member lookup need the same structured record
owner selection; it still returns `TypeSpec::tag` as the explicit rendered
fallback. The `operator_call` empty-argument static `value` shortcut in
`try_lower_operator_call` remains a static-member path, not a method lookup; it
still uses the rendered tag and should be considered separately if static
member owner routing becomes the next packet. Other callers in `hir_types.cpp`,
`expr/call.cpp`, and `expr/object.cpp` still pass rendered tags directly, but
only the `operator.cpp` method-lookup family was in this packet's slice.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
