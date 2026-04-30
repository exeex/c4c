Status: Active
Source Idea Path: ideas/open/136_hir_structured_record_template_lookup_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Scoped Static-Member Lookup Structured-Primary

# Current Packet

## Just Finished

Completed Plan Step 3 `Make Scoped Static-Member Lookup Structured-Primary`.

Added direct `HirStructMemberLookupKey` construction and lookup overloads for
static-member decl and const-value queries. Scoped `Owner::member` lowering now
builds a structured owner/member key from `TypeSpec.record_def`, resolved
template owner tags, and member `TextId` when available, then uses the rendered
tag/member path as an explicit compatibility fallback. Extended the focused HIR
test so stale rendered owner and member spelling loses to the structured
record/member identity.

## Suggested Next

Next coherent packet: continue with the supervisor-selected follow-up for
structured HIR record/template lookup authority cleanup.

## Watchouts

The scoped static-member fallback remains rendered-name compatibility only. The
structured route is direct for keyed lookups but still records rendered parity
when both authorities are present.

## Proof

Ran the supervisor-selected proof exactly:
`cmake --build build --target c4cll frontend_hir_tests frontend_hir_lookup_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests|frontend_parser_tests|cpp_hir_template_alias_deferred_nttp_static_member|cpp_hir_template_deferred_nttp_static_member_expr|cpp_hir_template_deferred_nttp_cast_static_member_expr|cpp_hir_template_value_arg_static_member_trait|cpp_positive_sema_operator_shift_static_member_call_parse_cpp|cpp_positive_sema_inherited_static_member_lookup_runtime_cpp|cpp_positive_sema_inherited_static_member_lookup_simple_runtime_cpp|cpp_positive_sema_unqualified_static_member_call_runtime_cpp|cpp_positive_sema_template_member_owner_resolution_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_positive_sema_template_struct_nested_cpp)$' >> test_after.log 2>&1`

Result: passed, 14/14 tests green. Proof log: `test_after.log`.
