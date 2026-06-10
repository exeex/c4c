Status: Active
Source Idea Path: ideas/open/177_aarch64_selected_f64_global_readback_dispatch_debt.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement The Readback To Call-Move Repair

# Current Packet

## Just Finished

Step 3 (`Implement The Readback To Call-Move Repair`): implemented the local
readback repair in
`src/backend/mir/aarch64/codegen/select_materialization.cpp`.
`materialize_direct_global_select_chain_call_argument(...)` now builds a local
`PreparedValueHomeLookups` fallback from `context.function.value_locations`
when `context.function.value_home_lookups` is null and passes that lookup into
`find_prepared_value_home_for_bir_value(...)`.

The helper also now materializes selected FP direct-global call arguments into
their prepared FPR home before the existing ABI move. It emits the same-block
FP select chain locally, reads leaf FP values back from prepared FPR homes when
generic FP reconstruction cannot, records the prepared FPR home as emitted,
and leaves the existing `fmov d0, d20` ABI move to consume that home.

## Suggested Next

Step 4 should inspect the Step 3 diff for route quality and decide whether the
local FP select-chain readback should remain in
`select_materialization.cpp` or be split into a smaller shared helper before
supervisor acceptance.

## Watchouts

- No test expectations were weakened or edited.
- The repair stayed in `select_materialization.cpp`; `calls.cpp`, `plan.md`,
  and source ideas were not touched.
- The code now recomputes this direct-global selected FP call argument into its
  prepared FPR home immediately before the ABI move, rather than trusting stale
  scalar-state availability.
- The FP leaf fallback currently handles prepared FPR register homes, which is
  sufficient for the selected f64 direct-global call-argument fixture that
  stores `%lhs`, `%rhs`, and `%selected` in FPR homes.

## Proof

Ran the delegated proof:

```bash
(cmake --build build --target backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^backend_aarch64_instruction_dispatch$' --output-on-failure) > test_after.log 2>&1
```

Result: passed. Proof log: `test_after.log`.
