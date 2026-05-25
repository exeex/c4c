# AArch64 c_testsuite 00181 Recursion Global Array Runtime

## Goal

Repair the runtime failure in
`c_testsuite_aarch64_backend_src_00181_c`.

## Why This Exists

The prior-preservation baseline drift runbook left `00181` red after the
source-selection family was repaired. The failure was classified as a separate
Tower of Hanoi recursion/global-array runtime family, not as another
call-boundary prior-preservation case.

## In Scope

- Reproduce `c_testsuite_aarch64_backend_src_00181_c` on AArch64 backend.
- Identify whether the bad behavior comes from recursion, global array
  addressing, pointer/index arithmetic, or call/return value handling.
- Repair the semantic lowering rule that causes the runtime mismatch.
- Add or update focused backend coverage for the repaired rule.

## Out Of Scope

- Reopening the prior-preservation selected fallback.
- Changing unrelated c_testsuite expectations.
- Broad call-plan or AArch64 calls cleanup.
- Treating `00216` or `00204` as part of this family without new evidence.

## Acceptance Criteria

- `c_testsuite_aarch64_backend_src_00181_c` passes.
- Focused proof isolates the repaired recursion/global-array behavior.
- Nearby same-feature tests are examined or covered enough to reject a
  named-test-only fix.
- No unsupported markings or weaker c_testsuite contracts are introduced.

## Reviewer Reject Signals

- A patch special-cases `00181`, Tower of Hanoi symbols, or one exact global
  array layout instead of repairing the lowering rule.
- A patch claims progress only by changing expectations, diagnostics, or test
  classification.
- A patch routes through the old broad prior-preservation fallback to hide the
  runtime mismatch.
- A patch leaves the same recursion/global-array failure mode in nearby cases
  while only the named testcase passes.
