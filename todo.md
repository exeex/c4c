Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
with the HIR struct-method lookup helper repair. `find_struct_method_mangled`,
`find_struct_method_link_name_id`, and `find_struct_method_return_type` now
check structured owner-key method maps before accepting rendered `tag::method`
maps, including the existing preferred/alternate constness order, while
preserving rendered fallback when no owner-key entry is available.

Added focused HIR coverage for template-instantiation owner keys where stale
rendered method maps point at wrong mangled names, link-name ids, and return
types, plus fallback coverage for legacy rendered-only method maps.

## Suggested Next

Continue Step 4 by auditing method lookup callers that still pass only rendered
owner tags, and route a follow-up packet for any remaining tests outside this
delegated subset that still encode rendered-primary method lookup expectations.

## Watchouts

Method lookup parity counters are still only recorded when a rendered
compatibility entry exists for the same constness key. Existing rendered
fallback remains active when `make_struct_method_lookup_key` cannot build a
complete owner key or when no owner-key method entry exists.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
