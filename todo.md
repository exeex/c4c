Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
with scoped static member lookup in `scalar_control.cpp`. `Owner::member`
lowering now builds an owner `TypeSpec` from AST template arguments and
available `struct_def_nodes_` `record_def`, resolves that through
`resolve_member_lookup_owner_tag`, and uses the structured owner tag for static
member decl/value lookup before falling back to rendered owner spelling.

Added focused HIR coverage proving a stale rendered scoped owner cannot select
the wrong static member const value when structured `record_def` owner identity
is available.

## Suggested Next

Continue Step 4 by auditing constructor overload routing or any remaining
rendered-tag member-symbol ingress outside the completed declaration,
compound-literal aggregate-init, generated-body, and scoped static member
lookup paths.

## Watchouts

Scoped static member lookup intentionally keeps the legacy rendered fallback
when no structured owner can be resolved. Template trait fast-path evaluation
still uses the parsed template primary spelling for the value computation, but
its resulting type lookup now consults the structured owner tag when available.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
