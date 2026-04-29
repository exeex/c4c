Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` started
with a focused deferred-member typedef authority repair. HIR now lets
`TypeSpec::record_def` resolve a pending `deferred_member_type_name` before
falling back to parser-rendered owner spelling in `TypeSpec::tag`.

Added focused HIR coverage where the owner `tag` is deliberately stale but the
structured `record_def` points at the real record carrying the member typedef.
That test would fail under the old rendered-tag-first path.

## Suggested Next

Continue Step 4 with the adjacent member-owner lookup path in HIR: prefer
structured owner keys or direct record carriers before `template_origin_name`
or rendered owner tags in `resolve_member_lookup_owner_tag` and template static
member helpers.

## Watchouts

This slice intentionally handles direct `record_def` member typedefs only. The
existing rendered-tag fallback remains for cases with no structured record
carrier, inherited typedef traversal through already-lowered module base tags,
and template-origin materialization paths.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
