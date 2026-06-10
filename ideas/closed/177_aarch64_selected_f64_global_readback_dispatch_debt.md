# 177 AArch64 selected f64 global readback dispatch debt

## Goal

Repair the ambient `backend_aarch64_instruction_dispatch` failure where the
selected f64 global readback no longer feeds the expected call ABI move.

## Why This Exists

Route 5 current-block join-source validation proved its narrow helper and
isolated consumer subset, but the broader monolithic AArch64 dispatch test
still fails before reaching that Route 5 consumer. The failure message is:
`expected selected f64 global readback to feed call ABI move`.

This is close-level dispatch debt, not Route 5 join-source migration work.

## In Scope

- The selected f64 global readback path in AArch64 instruction dispatch.
- Call ABI move selection for the failing readback case.
- The narrow reproducing expectation inside
  `backend_aarch64_instruction_dispatch`.
- Any semantic source or ABI handoff needed to make the selected f64 readback
  feed the call move correctly.

## Out Of Scope

- Route 5 current-block join-source identity or prepared helper contraction.
- Weakening, deleting, or downgrading the monolithic dispatch expectation.
- Broad call lowering rewrites unrelated to the selected f64 readback failure.
- Moving ABI/layout policy into Route 5 or generic BIR records.

## Acceptance Criteria

- `backend_aarch64_instruction_dispatch` passes the selected f64 global
  readback call ABI move expectation.
- The fix preserves existing call ABI/layout ownership.
- Any added narrower proof still keeps the monolithic dispatch check as the
  close-level acceptance signal.

## Proof Route

Run the focused build for `backend_aarch64_instruction_dispatch_test`, then run:

```bash
ctest --test-dir build -R '^backend_aarch64_instruction_dispatch$' --output-on-failure
```

Escalate to broader backend validation if public call, source-selection, or
AArch64 dispatch interfaces change.

## Reviewer Reject Signals

- The failing expectation is weakened, removed, or marked unsupported without
  explicit user approval.
- The patch special-cases the exact test name or emitted assembly snippet
  instead of repairing source selection or call ABI move routing.
- Route 5 join-source records are used to carry call ABI/layout policy.
- The same f64 global readback still bypasses the call ABI move under a renamed
  helper or facade.

## Completion Notes

Closed after Step 4 proof/handoff. Commit `af3891f3e` repaired the selected
F32/F64 direct-global call-argument path in AArch64 select materialization so
it materializes through the prepared FPR home before the existing ABI move.
The route-quality review in `review/route177_step3_review.md` judged the
implementation on-route and not testcase overfit.

Close proof used the focused build plus monolithic dispatch CTest:

```bash
(cmake --build build --target backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^backend_aarch64_instruction_dispatch$' --output-on-failure) > test_after.log 2>&1
```

`test_after.log` passed 1/1. The close-time regression guard compared the
matching focused `test_before.log` and `test_after.log` scope and passed with
no new failures.
