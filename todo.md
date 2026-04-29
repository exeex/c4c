Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
with direct HIR member-symbol lookup ingress in `expr/object.cpp`,
`expr/scalar_control.cpp`, and `stmt/stmt.cpp`. Initializer-list temporary
member writes, implicit `this.member` lowering, and constructor member
initializers now resolve the member owner from the available `TypeSpec` or
current-struct carrier through structured owner identity before falling back to
rendered tags for `find_struct_member_symbol_id`.

Added focused HIR coverage for stale rendered owner tags in initializer-list
member materialization, implicit `this` field access, and constructor member
initializers. Each test proves the lowered `MemberExpr` keeps the structured
owner tag and selects the real owner member symbol instead of the stale rendered
symbol.

## Suggested Next

Continue Step 4 by auditing remaining rendered-tag member-symbol ingress in
files outside this packet, especially statement declaration paths, while keeping
static member lookup and constructor overload routing separate unless delegated.

## Watchouts

The new current-struct member-symbol paths construct a `TypeSpec` carrier from
the rendered struct tag plus the existing `struct_def_nodes_` entry when one is
available, so legacy rendered fallback remains active. Constructor overload
routing, static member lookup, and previously completed method lookup routing
were left untouched by this packet.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
