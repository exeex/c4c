Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
with the range-for HIR method-lookup caller path. `lower_range_for_stmt` now
resolves structured owner identity from the range and iterator `TypeSpec`
carriers before calling `find_struct_method_mangled` or
`find_struct_method_return_type`, while retaining rendered-tag fallback through
the shared owner-resolution helper when structured identity is absent.

Added focused HIR coverage for the range-for method-owner path where a stale
rendered range tag would select the wrong `begin` method unless `record_def`
structured owner identity is used first.

## Suggested Next

Continue Step 4 by auditing the operator-call family in
`src/frontend/hir/impl/expr/operator.cpp`, especially `try_lower_operator_call`,
`lower_member_expr` operator-arrow chaining, and `maybe_bool_convert`, so those
method lookups also prefer structured `TypeSpec` owner identity before rendered
tags where available.

## Watchouts

`resolve_struct_method_lookup_owner_tag` currently reuses the member-owner
resolver because both method and member lookup need the same structured record
owner selection; it still returns `TypeSpec::tag` as the explicit rendered
fallback. Other callers in `operator.cpp`, `expr/call.cpp`, and `expr/object.cpp`
still pass rendered tags directly, but only `operator.cpp` is in this packet's
owned file set.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
