# Prepared Destination Dump Contract Review

Status: Open
Type: Implementation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `ideas/closed/661_rv64_prepared_destination_publication.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
Owning Layer: prepared destination dump contract and value-id exposure
Queue Order: 71
Proof Surface: residual rows from the closed RV64 prepared destination
publication route:
- `backend_dump_riscv64_scalar_compare_frame_slot_destination`
- `backend_dump_riscv64_prepared_fused_compare_call_result_predicate`
- `backend_dump_riscv64_function_pointer_return_chain`

## Goal

Review and repair the prepared-BIR dump contract for destination publication
rows where Step 1 evidence shows current prepared facts exist but the dump
snippets are stale or value-id mismatched.

## Why This Exists

The RV64 prepared destination publication route refreshed the focused four-row
surface and found no direct prepared destination implementation boundary for
three rows. Scalar compare currently publishes the frame-slot home and storage
for `%t2` at `value_id=3` while the snippet expects `value_id=2`; fused compare
publishes the call-result destination at `destination_value_id=1` while the
snippet expects `0`; and function-pointer return-chain publishes `@sub` at
`value_id=6` while the snippet expects `5`. That is dump-contract or
expectation-review work unless later route/runtime proof shows a real consumer
lowering failure.

## In Scope

- Refresh focused prepared-BIR dump evidence for the three residual
  destination rows.
- Identify whether each row is stale snippet expectation, dump text emission
  over valid prepared facts, or missing prepared publication.
- Repair the general dump contract or prepared fact exposure only when focused
  evidence proves the owner.
- Pair any expectation update with current prepared-fact evidence and nearby
  route/runtime proof when those rows exist.

## Out Of Scope

- Stack-passed parameter-home publication and caller ABI stack-binding split
  work.
- Reopening ideas 647 or 655 stack-destination fan-in authority.
- RV64 pointer-local, byval, object-data static storage, object-emission,
  callee-saved GPR, packed-member, AArch64, prepared CLI, or LLVM torture
  work.
- Unsupported-marker changes, allowlist edits, timeout changes, runtime policy
  changes, baseline accounting changes, or claiming text-only expectation
  churn as compiler capability progress.

## Acceptance Criteria

- Focused evidence names whether each row is stale expectation, dump emission,
  or missing prepared publication.
- Any expectation update is justified by current prepared facts and paired
  with route/runtime proof where applicable, not claimed as lowering progress
  by itself.
- Any code repair exposes existing prepared facts faithfully without matching
  testcase names, value IDs, or final assembly shape.
- Backend regression proof shows no new backend failures in the supervisor's
  chosen subset.

## Reviewer Reject Signals

- Reject treating text-only expectation rewrites as prepared destination or
  RV64 lowering capability progress.
- Reject named-case matching for scalar compare, fused compare, or
  function-pointer return-chain rows.
- Reject value-id-only shortcuts that do not explain why the current prepared
  fact is authoritative.
- Reject unsupported-marker downgrades, allowlist edits, timeout changes,
  runtime policy changes, helper renames, or baseline accounting changes
  claimed as progress.
- Reject reopening the closed prepared destination publication route unless
  fresh focused proof shows a current prepared fact is missing or ambiguous.
- Reject leaving the same stale dump-contract mismatch behind a renamed helper
  or diagnostic.
