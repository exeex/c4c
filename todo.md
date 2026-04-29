Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
with declaration-side local destructor tracking in `decl.cpp`. Local destructor
stack registration now derives the tracked destructor/member-dtor owner from
structured declaration type identity (`record_def` or template owner carriers)
before falling back to the rendered declaration `tag`.

Added focused HIR coverage proving a stale rendered local declaration tag cannot
select the wrong explicit destructor tracking or suppress member-dtor tracking
when structured `record_def` owner identity is available.

## Suggested Next

Continue Step 4 by auditing remaining declaration-side struct owner lookups in
`decl.cpp` for any paths that still prefer rendered `tag` before structured
owner identity.

## Watchouts

Declaration constructor routing intentionally keeps the rendered fallback when
no structured declaration owner can be resolved. Local destructor tracking now
uses the same declaration owner helper; the rendered fallback remains
intentional when no structured owner can be resolved.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
