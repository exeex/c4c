Status: Active
Source Idea Path: ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Reconcile The Residual 20-Row Lane

# Current Packet

## Just Finished

Executed Step 9, "Reconcile The Residual 20-Row Lane", as a proof/accounting
packet only. No implementation files, source ideas, `plan.md`, expectation
files, unsupported markers, allowlists outside `build/agent_state`, or runtime
comparison code were changed.

Derived artifacts:

- `build/agent_state/551_step9_residual_20.allowlist`
- `build/agent_state/551_step9_residual_20/row_status.tsv`
- `docs/rv64_gcc_torture_post_contract/move_bundle_materialization_residual_20_reconciliation.md`

Fresh Step 9 counts from the 20-row allowlist:

- 20 rows scanned.
- 5 rows passed.
- 6 rows rerouted to prepared move-bundle classifier/authority through
  `ambiguous_non_parallel_multi_source_stack_destination`.
- 6 rows still print
  `fragment_status=generic_move_bundle_materialization_failed`, but their
  row-level evidence points at prepared-authority gaps rather than an
  implementable RV64 materialization rule:
  `src/20000717-3.c`, `src/20100316-1.c`, `src/920908-2.c`,
  `src/loop-2d.c`, `src/strcmp-1.c`, and `src/strncmp-1.c`.
- 2 rows advanced to `RV64_BACKEND_RUNTIME_MISMATCH`.
- 1 row advanced to `unsupported_terminator_fragment`.
- 0 rows remain in
  `same_generic_move_bundle_materialization_failure_without_reroute`.
- 0 evidence-gap rows were found in this residual subset.

## Suggested Next

Supervisor should send this packet through reviewer or plan-owner judgment for
closure because six rows still expose the old generic fragment status even
though the evidence points outside RV64 materialization. If continuing instead
of closing, the next narrow packet is prepared-authority/evidence work to turn
those six rows into explicit non-RV64 diagnostics.

## Watchouts

- The focused scan reports 15 failures, but none are a clear implementable
  RV64 move-bundle materialization rule inside this source idea.
- The six generic-fragment rows should not be fixed by inferring destination
  homes, pointer-base stack slots, or missing source size authority in RV64.
- Later-route failures are `src/pr48197.c` with
  `unsupported_terminator_fragment` and `src/20020510-1.c` plus
  `src/pr89195.c` with `RV64_BACKEND_RUNTIME_MISMATCH`.

## Proof

- Full delegated Step 9 proof output is preserved in `test_after.log`.
- `cmake --build --preset default` passed.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed:
  345/345.
- `ALLOWLIST=build/agent_state/551_step9_residual_20.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh`
  reported `total=20 passed=5 failed=15`.
- `git diff --check` passed for the tracked changes.
