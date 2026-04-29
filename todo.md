Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
with the HIR static-member declaration lookup repair.
`find_struct_static_member_decl` now checks the structured owner-key declaration
map before accepting rendered-tag static-member declarations, while preserving
rendered fallback when no owner-key declaration is available.

Added focused HIR coverage for a template-instantiation owner key where stale
rendered spelling points at the wrong declaration, plus fallback coverage for
legacy rendered-only static-member declarations.

## Suggested Next

Continue Step 4 by auditing the remaining method/static-member owner-key parity
helpers for the same structured-primary boundary, starting with method mangled,
method link-name, and method return-type lookups.

## Watchouts

This packet only migrates static-member declaration lookup. Existing rendered
fallback is still active when `make_struct_member_lookup_key` cannot build a
complete owner key or when no owner-key declaration entry exists for the member.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
