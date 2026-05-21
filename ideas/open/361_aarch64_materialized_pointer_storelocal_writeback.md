# AArch64 Materialized Pointer StoreLocal Writeback

Status: Parked
Created: 2026-05-21
Split From: ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md

## Parked Outcome

Parked: 2026-05-21

The materialized pointer-addressed `StoreLocal` writeback owner was repaired by
commit `ee027c36a Write back materialized pointer stores`. The accepted slice
added focused backend coverage for explicit materialized pointer stores and the
`00181` stack-homed pointer shape, and generated `Move` assembly now contains
real stores through reloaded computed pointers before `PrintAll`.

Focused proof after repair was 6/7: the backend contracts, `00170`, and
`00189` pass. `00181` advanced beyond the old unchanged subsequent-state
runtime mismatch and now fails as a 5 second timeout. The visible residual is
adjacent pointer-derived load/address scaling, with suspicious generated
address scaling such as `mov x9, #4; mul x9, x9, x9` around pointer-derived
loads, not missing store writeback.

Close was rejected by the plan-owner close gate because
`c4c-regression-guard` reported no strict pass-count increase for the focused
before/after logs: both runs remained 6/7 with `00181` failing. The residual is
split to
`ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md`.

## Goal

Repair the AArch64 lowering residual where stores through materialized pointer
addresses do not write back to the addressed memory, leaving `00181`'s later
Hanoi moves unchanged after the starting-state select-store repair.

## Why This Exists

Idea 360 repaired the starting-state stale preserve bug for expanded
global-array select-store lowering. After narrowing the accepted slice, `00181`
now prints the correct initial towers:

- `A: 1 2 3 4`
- `B: 0 0 0 0`
- `C: 0 0 0 0`

The remaining failure is separate: subsequent moves still print the same
starting values because pointer-addressed stores represented through
materialized addresses are not being committed to the selected destination
memory. A broad `StoreLocal` fallback for this shape was reviewed as split
worthy and removed from idea 360 because it was shared address-store
capability, not the starting-state owner, and lacked focused backend coverage.

## In Scope

- Localize the first pointer-addressed store in `00181` that should mutate a
  tower after the correct starting-state print.
- Compare semantic BIR, prepared BIR, and generated AArch64 for the producing
  address value, the `StoreLocal` or equivalent store node, and the later load
  or print consumer.
- Repair the general AArch64 lowering rule for materialized pointer-addressed
  stores so the store writes through the pointed-to memory instead of
  disappearing or only updating a local value home.
- Add focused backend coverage for the materialized pointer-addressed
  `StoreLocal` writeback shape independent of `00181`.
- Prove `00181` advances past the unchanged-later-moves residual while
  preserving the idea 360 starting-state repair and nearby passing
  representatives.

## Out Of Scope

- Reopening the idea 360 direct `LoadGlobal` current-memory select-store
  repair except to keep it stable.
- Reopening stack-preserved pointer formal post-call overwrite handling from
  idea 359.
- Reopening scalar formal post-call reloads from idea 358.
- Reopening pointer-formal callee-saved home publication from idea 357.
- Reopening address-valued call-argument publication from idea 355 unless the
  first bad fact proves this store residual shares the same owner and a new
  lifecycle split is made.
- Semantic-BIR dynamic pointer-derived string loads for `00173`.
- Frontend or semantic admission failure for `00005`.
- ABI-wide composite/byval/HFA/f128 work, variadic floating work, dynamic
  stack work, unrelated scalar compare/select residuals, expectation changes,
  unsupported classifications, runner behavior, timeout policy,
  CTest-registration, or proof-log policy.

## Acceptance Criteria

- The first bad fact is localized to a concrete pointer-addressed store
  boundary, including the source store operation, materialized address carrier,
  expected target memory, emitted store behavior, and later consumer.
- Focused backend coverage fails before the repair and passes after it for a
  materialized pointer-addressed `StoreLocal` or equivalent writeback shape.
- `00181` advances beyond the repeated unchanged starting-state output without
  expectation, runner, timeout, or filename-specific changes.
- The idea 360 starting-state output remains correct.
- `00170`, `00189`, and focused backend contracts for the repaired memory
  shapes remain passing.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00181`, `Hanoi`, a tower name, one source line, one stack
  offset, one ABI register, or one emitted instruction neighborhood instead of
  repairing a general pointer-addressed store writeback rule;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, classification notes, emitted-text
  rewrites, or pass-count movement while materialized pointer-addressed stores
  still fail to update the addressed memory;
- absorbs unrelated address-valued call argument, dynamic string load,
  frontend admission, ABI composite, variadic/floating, or dynamic-stack work
  without fresh first-bad-fact evidence and a lifecycle split;
- regresses the idea 360 starting-state output or the already repaired ideas
  357, 358, and 359 stability paths.
