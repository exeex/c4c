Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
with remaining declaration-side aggregate owner lookups in `decl.cpp`.
Local aggregate initialization now resolves struct definitions, direct aggregate
assignment identity, base aggregate traversal, and array-element aggregate field
selection through structured declaration owner identity before falling back to
rendered tags where no structured owner is available.

Tightened focused HIR coverage so a stale rendered local declaration tag with a
different field shape cannot select the wrong aggregate owner when structured
`record_def` owner identity is available.

## Suggested Next

Continue Step 4 with supervisor review of the declaration-side owner lookup
slice and decide whether the remaining raw rendered-tag guards in `decl.cpp`
are intentional fallbacks or need a plan-owner follow-up.

## Watchouts

The packet repaired declaration-side aggregate paths in
`src/frontend/hir/impl/stmt/decl.cpp`, which is the compile-database path for
the requested `decl.cpp`. Union aggregate paths still preserve rendered-tag
fallback because the current structured owner helper is struct-specific.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
