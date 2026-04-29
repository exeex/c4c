Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
with constructor-initializer field constructor routing in `stmt.cpp`.
When a member initializer dispatches a field constructor, the lookup now derives
the constructor owner tag from structured field type identity (`record_def` or
template owner carriers) before falling back to the rendered field type tag.

Added focused HIR coverage proving a stale rendered field type tag cannot select
the wrong field constructor when structured `record_def` owner identity is
available.

## Suggested Next

Continue Step 4 by auditing declaration-side constructor dispatch in
`decl.cpp`, especially local declaration constructor paths that still key
through `decl_ts.tag`.

## Watchouts

The `stmt.cpp` field-constructor path intentionally keeps the rendered fallback
when no structured field owner can be resolved. Declaration-side constructor
paths were outside this packet's Do Not Touch boundary and remain candidates for
a separate packet.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
