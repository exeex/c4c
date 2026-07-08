Status: Active
Source Idea Path: ideas/open/609_rv64_global_data_consumer.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove consumer handoff and residual ownership

# Current Packet

## Just Finished

- Finished Step 5 (`Prove consumer handoff and residual ownership`) by
  refreshing the RV64 global-data allowlist evidence and recording the consumer
  handoff boundary.
- Backend proof passed with 346 backend tests and 0 failures; proof output is in
  `test_after.log`.
- Refreshed allowlist scan covered 9 rows from
  `build/agent_state/609_step1_global_consumer.allowlist`: 1 passed and 8
  failed. The passing row is `src/20030224-2.c`.
- Refreshed scan output is in
  `build/agent_state/609_step5_global_consumer_progress.log`; the movement and
  residual-owner summary is in
  `build/agent_state/609_step5_global_consumer_summary.md`.
- The source idea appears ready for supervisor/plan-owner close review from the
  consumer side: remaining rows are assigned to producer-authority,
  publication/link, or runtime/downstream ownership rather than another RV64
  global-data consumer implementation packet.

## Suggested Next

Return to the supervisor for close-review routing, broader validation choice,
and final commit handling.

## Watchouts

- Residual runtime/downstream row: `src/pr61517.c` now reaches run comparison
  and fails with `[RV64_BACKEND_RUNTIME_MISMATCH]` / `c4c_exit=Subprocess
  aborted`.
- Residual publication/link rows: `src/20010924-1.c`, `src/pr57877.c`,
  `src/pr57860.c`, and `src/20020118-1.c` reach object emission and fail at
  link with undefined prepared global-symbol references (`a4`, `g`, or `q`).
- Residual producer-authority rows: `src/20000703-1.c` still lacks supported
  prepared global memory facts, `src/pr82387.c` still lacks prepared direct
  global-symbol base-plus-offset addressing, and `src/20020213-1.c` still needs
  unsupported prepared global memory width support outside the current 1-, 2-,
  4-, and 8-byte RV64 consumer boundary.
- This packet did not edit source files, tests, expectations, unsupported
  markers, external allowlists, timeout/accounting files, `plan.md`, or
  `ideas/open/*`.

## Proof

- Delegated Step 5 proof was run:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Result: build passed; CTest ran 346 backend tests, 0 failed.
- Proof log path: `test_after.log`.
- Additional evidence command was run:
  `ALLOWLIST=build/agent_state/609_step1_global_consumer.allowlist STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/609_step5_global_consumer_progress.log 2>&1 || true`.
- Additional evidence result: 9 allowlisted rows scanned, 1 passed, 8 failed;
  summary path:
  `build/agent_state/609_step5_global_consumer_summary.md`.
