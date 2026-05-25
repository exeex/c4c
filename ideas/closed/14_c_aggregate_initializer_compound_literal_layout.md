# C Aggregate Initializer Compound Literal Layout

## Goal

Repair the aggregate initializer and compound literal layout failure represented
by `c_testsuite_aarch64_backend_src_00216_c`.

## Why This Exists

The prior-preservation baseline drift runbook left `00216` red after the
source-selection family was repaired. The failure was classified as an
aggregate initializer and compound literal layout family, separate from
AArch64 call-boundary source selection.

## In Scope

- Reproduce `c_testsuite_aarch64_backend_src_00216_c`.
- Trace aggregate initializer and compound literal lowering through frontend,
  HIR/LIR, BIR, and AArch64 backend ownership as needed.
- Repair the semantic layout or initialization rule that causes the runtime
  failure.
- Add focused coverage for the repaired initializer/compound-literal shape.

## Out Of Scope

- Reopening prior-preservation fallback behavior.
- ABI-wide aggregate calling convention rewrites unless the failure proves they
  are the direct cause.
- Weakening c_testsuite contracts.
- Folding `00181` or `00204` into this route without evidence.

## Acceptance Criteria

- `c_testsuite_aarch64_backend_src_00216_c` passes.
- Focused tests prove the aggregate initializer or compound literal layout rule
  that was missing.
- The fix preserves existing aggregate initialization behavior outside the
  failing shape.
- No named-case shortcuts or unsupported expectation downgrades are used.

## Reviewer Reject Signals

- A patch matches `00216` or one exact initializer spelling instead of using
  structured aggregate layout facts.
- A patch changes only expected output, unsupported status, or failure
  classification.
- A patch moves the failure behind a helper rename without fixing aggregate
  layout semantics.
- A patch broadly rewrites unrelated ABI or call-boundary code to make one
  compound-literal case pass.

## Closure Note

Closed after Step 4 validation recorded the corrected proof command:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '(^backend_aarch64_|^c_testsuite_aarch64_backend_src_00216_c$)') > test_after.log 2>&1`

The recorded result was a successful build with all 28 selected
`backend_aarch64_` tests passing and
`c_testsuite_aarch64_backend_src_00216_c` passing. The focused coverage added
for the repaired rule is the local aggregate frame-slot call argument
materialization case in
`backend_aarch64_instruction_dispatch`. The relevant implementation and
validation commits are `c6a4b8d66` and `1b70e7f37`.
