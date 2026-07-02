Status: Active
Source Idea Path: ideas/open/545_bir_semantic_producer_admission_reconstruction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Producer Families

# Current Packet

## Just Finished

Step 2 from `plan.md` classified the verified current BIR admission rows by
first owner and producer topic. The durable classification artifact is
`docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`.

Current exact semantic row classification: `373` rows accounted for as
`264` BIR local-memory facts, `55` BIR call metadata rows, `34` BIR
runtime/intrinsic memory rows, and `20` BIR scalar/signature/control rows.
The related `44` bootstrap/global data-shape handoff rows remain separate and
are not counted as exact semantic producer rows.

## Suggested Next

Execute Step 3 from `plan.md`: generate follow-up routing for coherent
high-frequency current producer families, keeping local-memory, call metadata,
runtime/intrinsic, and scalar/signature/control lanes separate unless row
evidence proves a shared producer boundary.

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
- Classification rejected non-BIR routes for the `373` exact semantic rows:
  prepared contract gaps `0`, RV64/MIR object lowering `0`, runtime mismatch
  `0`, test infrastructure `0`, F128 quarantine as first owner `0`, and
  evidence gaps `0`.
- Aggregate facts and publication gaps have `0` first-owner rows in the exact
  semantic set; do not create those claims from downstream inference.
- The `44` bootstrap/global data-shape handoff rows are BIR handoff-related
  but not exact `semantic lir_to_bir` rows.
- Representative inspected logs include
  `build/rv64_gcc_c_torture_backend/src_pr82388.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20000717-4.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20011008-3.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20000412-2.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_stdarg-4.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20000703-1.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20050604-1.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20041218-1.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20000314-3.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20050316-3.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_complex-1.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_960513-1.c/case.log`, and
  `build/rv64_gcc_c_torture_backend/src_strlen-2.c/case.log`.

## Proof

Evidence-only packet. No build or compile proof was required, and no
`test_after.log` update was required by the delegated proof contract.

Extraction commands recorded in
`docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
classified the Step 1 artifact table, inspected representative per-case logs,
and confirmed first-topic evidence from current logs under
`build/rv64_gcc_c_torture_backend/<case-id>/case.log`.
