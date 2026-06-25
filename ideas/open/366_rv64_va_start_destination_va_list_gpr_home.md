# RV64 `va_start` Destination `va_list` GPR Home

Status: Open
Type: Target ABI follow-up
Parent: `ideas/closed/365_rv64_va_start_overflow_base_state_production.md`

## Goal

Materialize or prove the prepared destination `va_list` address state that RV64
`va_start` helper lowering requires when storing initialized `va_list` fields.

## Why This Exists

Idea 365 completed the overflow-area base-state producer milestone. The
representative `src/920908-1.c` case no longer fails with:

`RV64 va_start helper requires prepared overflow-area initial base state`

It now reaches a later RV64 helper/lowering boundary:

`unsupported_variadic_helper_lowering: RV64 va_start helper requires destination va_list address in a prepared GPR home`

That residual is not an overflow-base production problem. It requires a
separate producer/contract decision for how the prepared pipeline represents
the address of the destination `va_list` object in a GPR home before RV64
object emission can perform the helper stores.

## In Scope

- Identify the prepared fact or producer boundary that should make the
  destination `va_list` address available to RV64 `va_start` helper lowering.
- Define when the destination address is considered resident in, or
  materializable from, a prepared GPR home.
- Preserve precise unsupported diagnostics when the prepared address contract
  is not available.
- Add focused backend or prepared-contract tests proving both supported address
  materialization and the unsupported boundary.
- Rerun `src/920908-1.c` as a representative signal and document whether it
  advances past the destination-address prepared-GPR-home diagnostic.

## Out of Scope

- Reworking overflow-area base-state production from idea 365.
- Reopening the idea-364 RV64 object-emission helper-store path except for
  narrow consumer adjustments required by the finalized destination-address
  contract.
- General RV64 parameter-home expansion, tracked separately in
  `ideas/open/363_rv64_object_parameter_home_coverage.md`.
- External variadic call lowering such as
  `PreparedCallWrapperKind::DirectExternVariadic`.
- Scalar `va_arg`, `va_copy`, aggregate `va_arg`, or broad variadic ABI
  rewrites not needed to prepare the destination `va_list` address.
- Testcase-name matching, source-pattern shortcuts, unsupported expectation
  downgrades, allowlist filtering, or scan-only classification changes.

## Acceptance Criteria

- Supported RV64 `va_start` helper entries have explicit prepared destination
  `va_list` address state before object emission consumes it.
- Focused tests distinguish supported destination-address materialization from
  entries that must remain precisely rejected.
- Existing focused coverage for RV64 object emission, prepared frame stack call
  contracts, prepared printer output, and variadic-entry diagnostics remains
  green.
- `src/920908-1.c` is rerun and its outcome is documented; progress must be a
  real prepared destination-address advance, not a renamed diagnostic.

## Reviewer Reject Signals

- Reject named-case-only handling for `src/920908-1.c` or matching its exact
  source shape instead of publishing a general prepared destination-address
  contract.
- Reject object-emission guesses that synthesize the destination `va_list`
  address without an explicit prepared producer/runtime contract.
- Reject helper renames, expectation rewrites, unsupported downgrades, or
  classification-only changes claimed as destination-address progress.
- Reject broad RV64 parameter-home, external variadic-call, or `va_arg` rewrites
  that do not directly prepare the destination `va_list` address needed by
  `va_start` helper stores.
- Reject retaining the exact prepared-GPR-home failure behind a new abstraction
  name or diagnostic spelling.

## Starting Signal

Closure validation from idea 365:

- focused backend prepared-contract, prepared-printer, and RV64 object-emission
  tests passed
- `src/920908-1.c` advanced past the old missing overflow-area base-state
  diagnostic
- the current residual is:
  `unsupported_variadic_helper_lowering: RV64 va_start helper requires destination va_list address in a prepared GPR home`
