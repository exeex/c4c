Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
with the HIR member-owner lookup repair. `resolve_member_lookup_owner_tag` now
tries structured `record_def` owner keys/direct record carriers and concrete
`tpl_struct_origin` realization before accepting parser-rendered owner tags.

Added focused HIR coverage where stale rendered owner spelling would have won
under the old path: one direct `record_def` owner case and one template-origin
realization case.

## Suggested Next

Continue Step 4 with the adjacent template static-member helper lookups in HIR:
audit `try_eval_template_static_member_const` and instantiated static-member
const lookup paths for remaining tag-first owner resolution, preserving rendered
fallback only when no structured carrier is available.

## Watchouts

The rendered-tag fallback remains intentionally in place for types that carry no
`record_def`, no concrete template-origin payload, and no recoverable current
struct family. The new template-origin path only claims authority when
realization changes or fills in the HIR struct tag, so unresolved/generic
payloads can still fall through to the compatibility bridge.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
