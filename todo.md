Status: Active
Source Idea Path: ideas/open/545_bir_semantic_producer_admission_reconstruction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct Current BIR Admission Rows

# Current Packet

## Just Finished

Step 1 from `plan.md` reconstructed the current BIR admission candidate row
set from the 2026-07-02 RV64 gcc_torture backend scan artifacts. The durable
evidence artifact is
`docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md`.

Current result: `373` exact `semantic lir_to_bir` rows, all with enough
visible per-case log evidence to identify a first BIR admission topic, plus
`44` related bootstrap `lir_to_bir` handoff rows that are not counted as exact
semantic producer rows.

## Suggested Next

Execute Step 2 from `plan.md`: classify the verified `373` exact semantic
rows by first owner and BIR producer topic, keeping the `44` bootstrap
handoff rows in a related/non-semantic lane unless classification proves they
share a producer boundary.

## Watchouts

- Do not use stale historical counts as current row evidence.
- The mutable files
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` and
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt` are truncated in
  this checkout; Step 1 reconstructed from the timestamped current pointer log
  and current per-case logs instead.
- Do not implement RV64/MIR workarounds or infer facts missing from the BIR
  producer.
- Do not weaken semantic admission checks, expectations, unsupported markers,
  allowlists, or runtime comparison behavior.
- Keep F128-primary rows in the quarantine lane, not ordinary-C producer
  cleanup.
- Representative inspected logs include
  `build/rv64_gcc_c_torture_backend/src_pr82388.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_stdarg-4.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20000412-2.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_complex-1.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20000314-3.c/case.log`, and
  `build/rv64_gcc_c_torture_backend/src_strlen-2.c/case.log`.

## Proof

Evidence-only packet. No build or compile proof was required, and no
`test_after.log` update was required by the delegated proof contract.

Extraction commands recorded in
`docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md` inspected
`build/agent_state/rv64_gcc_torture_backend_current_log_path.txt`,
`build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`,
the current mutable summary/failed-list artifacts, and current per-case logs
under `build/rv64_gcc_c_torture_backend/<case-id>/case.log`.
