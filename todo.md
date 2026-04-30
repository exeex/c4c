Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
by narrowing declaration owner fallback in `decl.cpp`. Local declaration owner
resolution now separates structured carrier presence from successful structured
resolution, so rendered `TypeSpec::tag` is used only when no structured carrier
is present. Failed structured declaration owner resolution now fails closed for
aggregate field/member-symbol lookup, default constructor lookup, and
destructor/member-dtor tracking instead of silently using stale rendered tags.

Added focused HIR coverage for an unresolved structured `record_def` paired with
a stale rendered tag that still has stale field/constructor/destructor entries;
the test proves declaration lowering does not consume those stale entries.

## Suggested Next

Continue Step 4 with supervisor review of the declaration-side fail-closed
slice and decide whether the remaining rendered-tag compatibility paths outside
local declaration struct ownership need another bounded packet.

## Watchouts

The narrowed helper is local to `lower_local_decl_stmt`; shared
`resolve_member_lookup_owner_tag` still preserves its legacy final rendered-tag
fallback for other callers. Union fallback remains rendered-tag based only when
no structured owner carrier is present.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
