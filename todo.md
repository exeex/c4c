Status: Active
Source Idea Path: ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Triage The 3 Residual Immediate-To-Stack Rows

# Current Packet

## Just Finished

Executed Step 8 for the three residual immediate-to-stack rows:
`src/920721-1.c`, `src/pr82192.c`, and `src/usmul.c`.

This was a proof-only packet. After the Step 7 immediate-store
generalization, all three rows now pass the RV64 gcc torture backend scan and
report zero generic move-bundle materialization failures.

Derived proof artifacts live under
`build/agent_state/551_step8_immediate_to_stack_residual.allowlist` and the
per-case logs under `build/rv64_gcc_c_torture_backend/`.

Fresh Step 8 counts:

- 3 rows scanned.
- 3 rows passed.
- 0 rows reported `fragment_status=generic_move_bundle_materialization_failed`.

No implementation files, source ideas, expectation files, unsupported markers,
allowlists outside `build/agent_state`, or runtime comparison code were
changed.

## Suggested Next

Delegate Step 9 final reconciliation for the 151-row coherent
move-bundle materialization lane.

## Watchouts

- Step 8 did not change code; it only proved the Step 7 immediate-store
  generalization covers the residual immediate-to-stack packet.
- Step 9 should reconcile the full 151-row coherent lane and distinguish
  remaining true materialization failures from later-route failures or
  prepared-authority reroutes.

## Proof

- Full delegated Step 8 proof output is preserved in `test_after.log`.
- `cmake --build --preset default` passed.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed:
  345/345.
- `ALLOWLIST=build/agent_state/551_step8_immediate_to_stack_residual.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh`
  reported `total=3 passed=3 failed=0`.
- The delegated Python log check reported
  `generic_move_bundle_failure_count=0`.
