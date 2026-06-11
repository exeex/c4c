Status: Active
Source Idea Path: ideas/open/188_phase_e_route7_comparison_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Parity, Fallback, and Invalid-Reference Cases

# Current Packet

## Just Finished

Step 2 and Step 3 completed for the selected consumer:
`aarch64::codegen::lower_materialized_compare_condition_branch(...)` now tries
validated Route 7 materialized-condition provenance first by building
`bir::route7_build_comparison_condition_index(*context.bir_block)`, querying
`bir::route7_find_materialized_condition(...)`, and accepting the result only
when `bir::route_index_validate_materialized_condition_reference(...)` validates
the same binary record.

Fallback behavior is preserved: invalid or unavailable Route 7 provenance falls
back to `bir::find_materialized_condition_producer_identity(...)`; if that still
does not produce a binary comparison, the existing emitted-condition branch path
can handle the materialized bool.

Tests added in `backend_aarch64_branch_control_lowering_test.cpp` cover selected
consumer Route 7/BIR identity parity and duplicate materialized-condition
provenance falling back to the emitted-condition branch path.

## Suggested Next

Supervisor should review and commit this completed Route 7 selected-consumer
migration slice, then choose the next comparison consumer or lifecycle action.

## Watchouts

- This slice intentionally did not touch return-chain lookup, unrelated
  comparison consumers, branch spelling/condition-code policy, or ALU
  machine-record formation.
- The new Route 7 adapter only exports the fields this selected consumer needs:
  binary pointer, instruction index, and condition name.

## Proof

Ran delegated proof:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper)$') > test_after.log 2>&1`

Result: passed, 3/3 tests green. Proof log: `test_after.log`.
