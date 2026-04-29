Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
with generated defaulted-method and member-destructor-call member-symbol ingress
in `stmt.cpp`. Defaulted copy/move generated member expressions and generated
member destructor-call expressions now build an owner `TypeSpec` from the
current struct and available `struct_def_nodes_` `record_def`, resolve the
owner through `resolve_member_lookup_owner_tag`, and prefer
`find_struct_member_symbol_id` for the structured owner before falling back to
the legacy field-stored member symbol.

Added focused HIR coverage proving stale rendered owner tags cannot select the
wrong member symbol for a defaulted copy generated member access or a generated
member-destructor access when structured `record_def` owner identity is
available.

## Suggested Next

Continue Step 4 by auditing any remaining rendered-tag member-symbol ingress
outside the completed declaration, compound-literal aggregate-init, defaulted
copy/move, and member-destructor generated-body paths. Keep static member
lookup, constructor overload routing, and previously completed
method/member-expression routes separate unless delegated.

## Watchouts

Both touched `stmt.cpp` sites intentionally keep the rendered `struct_tag` and
old field-stored member-symbol fallback when structured owner resolution has no
usable owner, so legacy generated bodies remain supported. This packet did not
touch static member lookup, constructor overload routing, or other expression
member routes.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
