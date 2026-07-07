# RV64 Va Start Stack-Backed Destination

Status: Open
Type: Focused RV64 variadic helper repair
Parent: `ideas/open/550_rv64_scalar_fpr_residual_salvage.md`
Owning Layer: RV64 variadic helper lowering

## Goal

Repair or narrowly diagnose RV64 `va_start` helper lowering when the prepared
destination `va_list` address lives in stack slots instead of a prepared GPR
home.

## Why This Exists

Step 3 of the scalar/FPR residual salvage lane classified `src/va-arg-21.c`
under a separate variadic helper owner, not under scalar compare publication or
floating casts. The relevant prepared helper operands record
`dst_va_list_addr` values in stack slots. Evidence is saved in:

- `build/agent_state/550_rv64_scalar_fpr_residual_salvage/step2/summary.md`
- `build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/owners.tsv`
- `build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/summary.md`

The row includes f128 declarations through libc headers, but Step 2 records
that the case-local prepared f128 carrier/helper sections are empty. Its live
owner is variadic helper destination-address lowering.

## In Scope

- RV64 `va_start` helper lowering for prepared destination `va_list` addresses
  backed by stack slots.
- Address materialization or a narrower fail-closed diagnostic for helper
  operands whose destination homes are not prepared GPRs.
- Focused helper tests that construct the semantic stack-backed destination
  shape without matching `src/va-arg-21.c` by name.
- Representative route proof for `src/va-arg-21.c` or a narrower owner if full
  support is not reached.

## Out Of Scope

- Scalar compare publication and ordinary floating-cast lowering.
- F128, long-double, or external soft-float helper implementation.
- Broad call ABI or varargs rewrites outside the `va_start` destination-address
  ownership boundary.
- Reclassifying the row as scalar/FPR work merely because libc declarations
  mention f128 prototypes.

## Acceptance Criteria

- Focused coverage proves RV64 `va_start` helper handling for stack-backed
  destination `va_list` addresses, or reports a narrower diagnostic at that
  owner.
- The representative `src/va-arg-21.c` route advances past the old
  `unsupported_variadic_helper_lowering` reason or records a narrower
  destination-address materialization failure.
- The proof does not rely on scalar compare or floating-cast changes.
- Backend validation for the touched RV64 helper bucket passes.

## Reviewer Reject Signals

- Reject treating libc f128 declarations as evidence that this is an F128 lane
  when the prepared case-local f128 sections remain empty.
- Reject named-case checks for `va-arg-21.c` instead of semantic helper operand
  handling.
- Reject broad call ABI or varargs rewrites that leave stack-backed
  `dst_va_list_addr` operands unsupported behind a renamed helper.
- Reject expectation rewrites, unsupported-marker changes, or diagnostic-only
  relabeling claimed as helper capability progress.
- Reject mixing this work with scalar compare publication or floating-cast
  implementation in one slice.
