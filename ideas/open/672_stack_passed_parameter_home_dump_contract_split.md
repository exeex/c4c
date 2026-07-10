# Stack-Passed Parameter Home Dump Contract Split

Status: Open
Type: Implementation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `ideas/closed/661_rv64_prepared_destination_publication.md`
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
Owning Layer: stack-passed parameter-home publication and dump contract
Queue Order: 72
Proof Surface: residual row from the closed RV64 prepared destination
publication route:
- `backend_dump_riscv64_stack_passed_parameter_home_publication`

## Goal

Classify and repair the stack-passed parameter-home dump row by separating
callee parameter-home publication from caller ABI stack-binding dump contract
evidence.

## Why This Exists

The prepared destination publication probe found that callee parameter homes
for stack-passed parameters are present under current IDs and slots
(`%p.fdB` value_id 10 slot#10 offset 40, `%p.C` value_id 12 slot#12 offset 48,
and `%p.fdC` value_id 13 slot#11 offset 44), but the first missing snippet is
for caller ABI stack-binding placement:
`abi_binding destination_kind=call_argument_abi destination_storage=stack_slot abi_index=8 stack_offset=0`.
Current prepared output publishes ABI stack bindings for indices 9, 11, and 12,
while index 8 is in register `a7` and index 10 is in register `fa1`. That
evidence is a parameter-home/dump-contract split, not stack-destination fan-in
authority from ideas 647 or 655.

## In Scope

- Refresh focused prepared-BIR dump evidence for
  `backend_dump_riscv64_stack_passed_parameter_home_publication`.
- Identify whether the first owner is callee parameter-home publication,
  caller ABI stack-binding dump exposure, or stale snippet expectation.
- Repair one general publication or dump-contract rule only after the caller
  versus callee owner is proven.
- Preserve ideas 647 and 655 as nearby authority boundaries without absorbing
  their stack-destination fan-in scope.

## Out Of Scope

- Scalar compare, fused compare, and function-pointer return-chain dump
  contract review.
- Ordered final-state, mutual-exclusion, explicit-merge, or rejection
  authority for stack-destination fan-in from ideas 647 and 655.
- RV64 byval, pointer-local, object-data static storage, object-emission,
  callee-saved GPR, packed-member, AArch64, prepared CLI, or LLVM torture
  work.
- Unsupported-marker changes, allowlist edits, timeout changes, runtime policy
  changes, baseline accounting changes, or expectation updates without current
  ABI/home evidence.

## Acceptance Criteria

- Focused evidence names whether the residual row is stale expectation, caller
  ABI stack-binding dump exposure, or missing callee parameter-home
  publication.
- Any repair explains the caller/callee boundary and preserves fail-closed
  diagnostics for missing or ambiguous home facts.
- Any expectation update is justified by current ABI and parameter-home facts,
  not by final assembly or testcase identity.
- Backend regression proof shows no new backend failures in the supervisor's
  chosen subset.

## Reviewer Reject Signals

- Reject route changes that silently reopen ideas 647 or 655 stack-destination
  fan-in authority.
- Reject named-case matching for
  `backend_dump_riscv64_stack_passed_parameter_home_publication`.
- Reject final-assembly, source-order, source-filename, or fixed-value-id
  shortcuts for deciding parameter-home authority.
- Reject unsupported-marker downgrades, allowlist edits, timeout changes,
  runtime policy changes, helper renames, or baseline accounting changes
  claimed as progress.
- Reject leaving the caller/callee ABI-home split ambiguous behind a renamed
  diagnostic.
