Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
with expression-side compound-literal aggregate-init member-symbol ingress in
`expr/object.cpp`. The `consume_from_list` aggregate member path now resolves
the owner from the available `TypeSpec` structured carrier before falling back
to rendered tags, and it prefers the resolved owner member symbol before the
legacy field-stored symbol.

Added focused HIR coverage proving a compound literal whose `TypeSpec` carries
a real `record_def` but a stale rendered tag lowers the aggregate `MemberExpr`
with the structured owner tag and selects the real owner member symbol instead
of the stale rendered symbol.

## Suggested Next

Continue Step 4 by auditing any remaining rendered-tag member-symbol ingress
outside the completed declaration and compound-literal aggregate-init paths.
Keep static member lookup, constructor overload routing, and previously
completed method/member-expression routes separate unless delegated.

## Watchouts

The compound-literal helper intentionally keeps the rendered-tag struct lookup
and old field-stored member-symbol fallback when structured owner resolution has
no usable owner, so legacy aggregate initialization remains supported. This
packet only touched `expr/object.cpp`; constructor overload routing, static
member lookup, and earlier expression/statement member routes were left
untouched.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
