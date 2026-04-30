Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten AST Boundary Fields and Deferred Member Types

# Current Packet

## Just Finished

Plan Step 4 `Tighten AST Boundary Fields and Deferred Member Types` completed
the `d4638dbc` regression repair across the shared member-owner helper and the
local declaration/compound-literal aggregate helper copies. Generated anonymous
aggregate `record_def` carriers such as `_anon_N` now preserve rendered-tag
compatibility, while named/resolvable structured owner keys still fail closed
when the owner index cannot resolve them.

Added focused HIR coverage for generated anonymous aggregate `record_def`
compatibility where the rendered tag exists but the generated record identity
is not a semantic owner key; existing stale named/resolvable owner coverage
continues to prove fail-closed behavior for shared, local declaration, and
compound-literal paths.

## Suggested Next

Supervisor review and commit boundary selection for the completed Step 4
repair slice.

## Watchouts

No current blocker. The helper distinction intentionally treats complete,
non-generated `record_def` owner keys as semantic identity and generated
`_anon_` records as rendered-tag compatibility carriers.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|positive_sema_linux_stage2_repro_04_typeof_stmt_expr_combo_c|llvm_gcc_c_torture_src_20180131_1_c|llvm_gcc_c_torture_src_pr33631_c|cpp_hir_template_origin|cpp_hir_template_struct_registry_primary_only|cpp_hir_template_member_owner_resolution|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_member_owner_chain|cpp_hir_template_member_owner_decl_and_cast|cpp_hir_template_alias_member_owner|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_parse_record_member_typedef_using_dump' --output-on-failure >> test_after.log 2>&1`

Result: passed, 16/16 delegated tests green, including the three repaired C
regressions:
`positive_sema_linux_stage2_repro_04_typeof_stmt_expr_combo_c`,
`llvm_gcc_c_torture_src_20180131_1_c`, and
`llvm_gcc_c_torture_src_pr33631_c`. Proof log is `test_after.log`.
