Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
by narrowing shared `Lowerer::resolve_member_lookup_owner_tag` fallback behavior
in `value_args.cpp`. The helper now records whether post-typedef owner identity
already has structured carriers (`record_def`, `tpl_struct_origin`, or
structured template args) and only returns rendered `TypeSpec::tag` fallback
when those carriers are absent.

Added focused HIR coverage for unresolved `record_def` and structured
template-arg carriers paired with stale rendered tags, plus a member-expression
caller fixture proving shared-helper callers do not select stale rendered owner
metadata after structured owner resolution fails.

## Suggested Next

Continue Step 4 with supervisor review of the shared-helper fail-closed slice,
then decide whether any remaining non-helper rendered-tag compatibility path
needs a separate bounded packet.

## Watchouts

The shared helper still preserves rendered-tag compatibility when no structured
owner carrier is present. This packet did not change the local declaration and
compound-literal helper copies that were already narrowed in earlier Step 4
slices.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
