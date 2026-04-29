Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
with direct struct constructor-call routing in `object.cpp`.
`try_lower_direct_struct_constructor_call` now builds a constructor owner
`TypeSpec` from the callee AST, including `record_def` and template-argument
payloads, resolves it through structured owner lookup, and uses the resolved HIR
struct tag for constructor and aggregate-temporary routing before falling back
to rendered callee spelling.

Added focused HIR coverage proving a stale rendered constructor spelling cannot
select the wrong constructor when structured `record_def` owner identity is
available.

## Suggested Next

Continue Step 4 by auditing any remaining rendered-tag ingress in constructor
paths outside direct call lowering, especially declaration/statement-side
constructor dispatch that still keys through `decl_ts.tag` or field type tags.

## Watchouts

Direct constructor-call lowering intentionally keeps the legacy rendered
fallback only when no structured owner can be resolved. The delegated Do Not
Touch list left declaration and statement constructor paths unchanged, so those
remain candidates for separate packets.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
