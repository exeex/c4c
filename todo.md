# Current Packet

Status: Active
Source Idea Path: ideas/open/546_rv64_instruction_fragment_current_classification.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct Current Instruction-Fragment Rows

## Just Finished

Step 1 (`Reconstruct Current Instruction-Fragment Rows`) rebuilt the row
artifact from the stable scan pointer plus available per-case logs:

- Verified `build/agent_state/rv64_gcc_torture_backend_current_log_path.txt`
  still points to
  `build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`.
- Stable scan log records `total=1467 passed=349 failed=1118`.
- Wrote
  `build/agent_state/unsupported_instruction_fragment_current_rows.tsv` with a
  header and `179` observed `unsupported_instruction_fragment` rows.
- Wrote drift notes to
  `build/agent_state/546_step1_instruction_fragment_reconstruction_drift.txt`.
- Reconstruction command used the stable full-scan fail rows as the case list
  and tested each referenced `build/rv64_gcc_c_torture_backend/<case-id>/case.log`
  for `unsupported_instruction_fragment`.

## Suggested Next

Step 2 should not classify the 179-row table as the intended 137-row current
scope without supervisor acceptance. Preferred next packet: refresh or recover
a matching full-scan summary/per-case-log snapshot, then classify semantic
families and first owners from the confirmed current row set.

## Watchouts

- Leave `review/557_step13_vector_local_memory_review.md` untouched.
- Do not reuse stale 190-row or 82-row instruction-fragment counts as current scope.
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` is stale from a
  one-case probe and contains only `src/20030209-1.c`; do not use it as the
  137-row scope.
- The stable 2026-07-02 scan log preserves pass/fail rows and log paths, but
  not diagnostic text. Current per-case logs are mutable: 335 case logs are
  newer than the stable scan log, and 62 of the 179 observed
  `unsupported_instruction_fragment` rows have newer case logs.
- Expected count is 137; reconstructed current-on-disk evidence is 179
  (`+42` drift). The artifact records observed rows instead of guessing a
  137-row subset.
- Do not implement RV64 lowering, edit expectations, or weaken unsupported markers in this classification packet.
- Screen primary-F128 rows into the quarantine lane before ordinary-C bucket ranking.

## Proof

- Evidence-only packet; no build proof required.
- Validation: `git diff --check -- todo.md`
