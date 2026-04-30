Status: Active
Source Idea Path: ideas/open/135_sema_structured_owner_static_member_lookup_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Make Static-Member Lookup Structured-Primary

# Current Packet

## Just Finished

Plan Step 4 `Make Static-Member Lookup Structured-Primary` completed for
`lookup_struct_static_member_type`.

`lookup_struct_static_member_type(tag, member, reference)` now returns the
`struct_static_member_types_by_key_` / `struct_base_keys_by_key_` result when
`reference->unqualified_text_id` is present and `structured_record_key_for_tag`
resolves a non-ambiguous owner key. It still computes the rendered
`tag/member` result for comparison in that structured-primary path, but the
rendered result is returned only when structured member or owner identity is
absent.

Added `test_sema_static_member_type_lookup_prefers_structured_member_key` to
`frontend_parser_tests`. The fixture gives a static-member reference stale
rendered member spelling (`Owner::stale`) while carrying the structured
member `TextId` for `actual`; stale rendered authority would infer `float` and
fail an `int*` return check, while structured-primary lookup validates.

## Suggested Next

Next coherent packet: route the remaining Sema owner helpers toward structured
owner identity, starting with `resolve_owner_in_namespace_context` or
`enclosing_method_owner_struct`, so `current_method_struct_key_` and owner
resolution do not depend on reparsed rendered names.

## Watchouts

This packet intentionally changes structured misses to return `nullopt` when
both owner key and member `TextId` are available; that is the point of making
structured identity authoritative. Compatibility fallback remains for missing
`reference`, missing member `TextId`, or ambiguous/missing owner key.

`resolve_owner_in_namespace_context` and `enclosing_method_owner_struct` still
return rendered owner strings and should not be expanded with more rendered
parsing in follow-up work.

## Proof

Ran the supervisor-selected proof exactly:

`cmake --build build --target c4cll frontend_parser_tests frontend_hir_tests frontend_hir_lookup_tests > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests|frontend_hir_lookup_tests|cpp_positive_sema_operator_shift_static_member_call_parse_cpp|cpp_positive_sema_inherited_static_member_lookup_runtime_cpp|cpp_positive_sema_inherited_static_member_lookup_simple_runtime_cpp|cpp_positive_sema_unqualified_static_member_call_runtime_cpp|cpp_positive_sema_template_member_owner_resolution_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_hir_template_alias_deferred_nttp_static_member|cpp_hir_template_deferred_nttp_static_member_expr|cpp_hir_template_deferred_nttp_cast_static_member_expr|cpp_hir_template_value_arg_static_member_trait)$' >> test_after.log 2>&1`

Result: build succeeded and CTest reported 14/14 passing. Proof log:
`test_after.log`.
