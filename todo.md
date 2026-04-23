# Execution State

Status: Active
Source Idea Path: ideas/open/81_parser_state_convergence_and_scope_rationalization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Regroup Parser Member Fields Into Explicit Bundles
# Current Packet

## Just Finished

- continued Step 2 by removing the temporary diagnostic/recovery compatibility
  aliases from `Parser`, routing parser-core and statement recovery code
  directly through `diagnostic_state_`, and updating `c4cll` to read parser
  errors from the explicit diagnostic bundle

## Suggested Next

- continue Step 2 by removing the pragma-state compatibility aliases and
  routing declaration/record visibility, execution-domain, and pack-alignment
  reads directly through `pragma_state_`

## Watchouts

- keep the work inside the parser subsystem
- keep Step 2 focused on field layout and ownership readability, not helper
  rewrites or grammar changes
- preserve constructor, snapshot, and rollback behavior while the grouped
  layout is being converted to direct bundle access
- alias removals can reach non-parser call sites such as `src/apps/c4cll.cpp`,
  so audit repo-wide member reads before assuming a family is parser-local

## Proof

- `cmake --build build -j --target c4c_frontend c4cll`
- `ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_hir_template_alias_deferred_nttp_static_member|cpp_hir_template_inherited_member_typedef_trait)$'`
- Result: passed
- Log: `test_after.log`
