Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
with the remaining assigned HIR method-lookup caller family in
`infer_generic_ctrl_type` and `expr/call.cpp`. The generic control-type
`operator_deref` and `operator_call` return-type probes, template-struct
`operator_call` lowering, and member-call lowering now resolve structured
owner identity from the relevant `TypeSpec` carrier before calling the method
lookup registries, while preserving rendered-tag fallback through the existing
owner-resolution helper.

Added focused HIR coverage for generic control-type deref/operator_call
return-type lookup and member-call lowering where stale rendered tags would
select the wrong method or return type unless `record_def` structured owner
identity is used first.

## Suggested Next

Continue Step 4 by auditing the remaining rendered-tag member/method lookup
ingress outside this packet, with care to keep static member routing separate
from method lookup routing unless the supervisor delegates that owner family.

## Watchouts

`resolve_struct_method_lookup_owner_tag` still delegates to the member-owner
resolver and then falls back to `TypeSpec::tag`, so legacy rendered-only method
lookup remains available. The `operator_call` empty-argument static `value`
shortcut in `try_lower_operator_call` remains a static-member path, not a
method lookup; it should stay out of method-owner packets unless static member
owner routing is explicitly delegated.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
