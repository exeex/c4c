Status: Active
Source Idea Path: ideas/open/177_aarch64_selected_f64_global_readback_dispatch_debt.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove And Handoff

# Current Packet

## Just Finished

Step 4 (`Prove And Handoff`): reran the close-level monolithic dispatch proof
after the Step 3 repair. `backend_aarch64_instruction_dispatch` now passes the
selected f64 global readback call ABI move expectation.

Step 3 changed only
`src/backend/mir/aarch64/codegen/select_materialization.cpp`: the direct-global
select-chain call-argument helper now builds local prepared value-home lookups
when needed and materializes selected F32/F64 direct-global call arguments into
their prepared FPR home before the existing ABI move.

## Suggested Next

Plan-owner close decision: the active runbook proof is green. The supervisor
already accepted route-quality review in `review/route177_step3_review.md`;
close or deactivate the idea unless lifecycle review finds remaining source
intent.

## Watchouts

- No test expectations were weakened or edited.
- The repair stayed in `select_materialization.cpp`; `calls.cpp`, `plan.md`,
  and source ideas were not touched.
- `review/route177_step3_review.md` judged the expanded FP select-chain helper
  on-route and not testcase overfit. It remains watch debt, not a close
  blocker.

## Proof

Ran the close-level proof:

```bash
(cmake --build build --target backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^backend_aarch64_instruction_dispatch$' --output-on-failure) > test_after.log 2>&1
```

Result: passed. Proof log: `test_after.log`.
