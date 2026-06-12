Status: Active
Source Idea Path: ideas/open/216_route7_comparison_oracle_row.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Route-Native Evidence for the Row

# Current Packet

## Just Finished

Step 2 added route-native evidence for the selected helper-oracle row in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

Changed files:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`: beside the existing `prepare::find_prepared_materialized_condition_producer(...)` success assertion for `%cond` at `block.insts.size()`, the row now queries `route7_find_materialized_condition(route7_index, block, %cond, block.insts.size())` and validates the same row through `route_index_validate_materialized_condition_reference(route_index_reference_facade(route7_index), block, %cond, block.insts.size())`.
- `todo.md`: recorded Step 2 completion and proof.

The prepared assertion was preserved. The new Route 7 materialized-condition evidence is checked against the prepared row for binary identity, instruction index, condition value name, and against the BIR identity for lhs/rhs producer provenance where available.

## Suggested Next

Step 3 should decide the next bounded row/gap for active idea 216, using the residual fallback matrix from Step 1: absent route, invalid/stale reference, duplicate/conflict, mismatch, unfused fallback, and prepared fallback coverage. Keep the next packet to one row or one gap family.

## Watchouts

- Step 2 intentionally did not broaden into fused-compare rows, branch-policy tests, wrappers, expected strings, Route 8 code, generic facades, or unrelated tests.
- Residual Step 3 gaps remain advisory for supervisor selection: absent route, invalid reference/stale owner, duplicate/conflict, mismatch, unfused fallback, and prepared fallback behavior.
- Expected-string rewrites, unsupported downgrades, weaker contracts, and testcase-shaped oracle handling remain blockers.

## Proof

Passed:

`cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering|backend_aarch64_machine_printer)$' --output-on-failure | tee test_after.log`

Proof log: `test_after.log`.
