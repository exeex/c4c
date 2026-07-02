Status: Active
Source Idea Path: ideas/open/545_bir_semantic_producer_admission_reconstruction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Generate Follow-Up Routing

# Current Packet

## Just Finished

Step 3 from `plan.md` generated follow-up routing candidates from the
classified current BIR admission rows without editing lifecycle source ideas.
The durable routing artifact is
`docs/rv64_gcc_torture_post_contract/bir_semantic_admission_followups.md`.

Follow-up candidates recorded:

- BIR local-memory semantic producer admission: `264` exact semantic rows;
  strong separate new idea candidate.
- BIR call metadata semantic producer admission: `55` exact semantic rows;
  strong separate new idea candidate.
- BIR runtime/intrinsic memory producer admission: `34` exact semantic rows;
  separate new idea candidate, lower priority than local-memory and call
  metadata.
- BIR scalar/signature/control semantic producer admission: `20` exact
  semantic rows; small follow-up idea candidate or explicit defer until the
  higher-frequency lanes are routed.
- Bootstrap/global data-shape handoff support: `44` related BIR handoff rows;
  separate related new idea candidate, not part of the `373` exact semantic
  producer row set.

Rejected/deferred routes from the exact semantic set remain unsupported by
current first-owner evidence: aggregate-first facts `0`, publication/prepared
or RV64/MIR handoff `0`, malformed semantic inputs `0`, prepared contract gaps
`0`, RV64/MIR object lowering `0`, runtime mismatch `0`, test infrastructure
`0`, F128 quarantine `0`, and evidence gaps `0`.

## Suggested Next

Execute Step 4 from `plan.md`: prove the reconstruction outcome and decide
whether the source idea is satisfied by the evidence artifacts or needs
plan-owner lifecycle follow-up. Exact lifecycle action needed next: supervisor
should pass
`docs/rv64_gcc_torture_post_contract/bir_semantic_admission_followups.md` to
the plan-owner as input for creating or activating separate ideas for the
local-memory, call metadata, runtime/intrinsic, scalar/signature/control, and
bootstrap/global data-shape lanes, with scalar/signature/control allowed to be
explicitly deferred.

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
- Routing rejected non-BIR routes for the `373` exact semantic rows: prepared
  contract gaps `0`, RV64/MIR object lowering `0`, runtime mismatch `0`, test
  infrastructure `0`, F128 quarantine as first owner `0`, and evidence gaps
  `0`.
- Aggregate facts and publication gaps have `0` first-owner rows in the exact
  semantic set; do not create those claims from downstream inference.
- The `44` bootstrap rows are related to BIR admission and global/data shape
  support, but they are intentionally excluded from the `373` exact semantic
  producer count and should remain a separate lifecycle lane.
- Do not merge local-memory, call metadata, runtime/intrinsic,
  scalar/signature/control, or bootstrap/global data-shape lanes unless fresh
  evidence proves a shared producer boundary.
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

Source artifacts used:

- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- `build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`
- `build/rv64_gcc_c_torture_backend/<case-id>/case.log`

Extraction and classification commands recorded in
`docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
classified the Step 1 artifact table, inspected representative per-case logs,
and confirmed first-topic evidence from current logs under
`build/rv64_gcc_c_torture_backend/<case-id>/case.log`. Step 3 was a routing
documentation packet and did not run compile validation.
