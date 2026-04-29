Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` continued
with the HIR static-member const lookup repair. `find_struct_static_member_const_value`
now checks the structured owner-key value map before accepting rendered-tag
static-member values, while preserving rendered fallback when no owner key
value is available.

Added focused HIR coverage for a template-instantiation owner key where stale
rendered spelling carries the wrong value, plus fallback coverage for legacy
rendered-only static-member values.

## Suggested Next

Continue Step 4 with the declaration side of static-member lookup: audit
`find_struct_static_member_decl` and its `try_eval_template_static_member_const`
callers for a structured-first decl path that does not break the current
parity-only rendered-decl compatibility test.

## Watchouts

Only the static constexpr value helper is behaviorally structured-primary in
this packet. Static-member declaration lookup still exposes rendered-map
behavior with owner-key parity checks, so the next packet should either migrate
that deliberately or update the existing lookup parity test in the same slice.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 13/13 delegated tests green; proof log is `test_after.log`.
