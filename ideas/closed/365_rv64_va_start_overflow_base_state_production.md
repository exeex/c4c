# RV64 `va_start` Overflow Base-State Production

Status: Closed
Type: Target ABI follow-up
Parent: `ideas/closed/364_rv64_va_start_helper_lowering.md`

## Goal

Produce explicit prepared overflow-area initial base state for RV64 variadic
entries so prepared `va_start` helper lowering can initialize the `va_list`
overflow field without guessing target runtime state.

## Why This Exists

Idea 364 completed the object-emission helper-lowering milestone for prepared
`va_start` facts when the overflow-area base state is explicit. Step 4
representative validation still leaves `src/920908-1.c` failing at the more
precise producer/runtime-state boundary:

`unsupported_variadic_helper_lowering: RV64 va_start helper requires prepared overflow-area initial base state`

The remaining work is not another object-emission fallback. It must define and
materialize the prepared entry-state facts that tell RV64 `va_start` where the
overflow argument area begins.

## In Scope

- Define the RV64 prepared-module contract for overflow-area initial base state
  at variadic function entry.
- Identify the producer that can publish the overflow-area base slot and
  signed stack offset into prepared variadic helper facts.
- Materialize helper-free RV64 variadic entry state only when the runtime
  contract is explicit and supportable.
- Update focused backend or prepared-contract tests to prove the producer emits
  `overflow_area.base_slot_id` and `base_stack_offset_bytes` for supported
  RV64 cases.
- Rerun `src/920908-1.c` as a representative signal and document whether it
  advances past the missing-base-state diagnostic.

## Out of Scope

- Reworking the idea-364 RV64 object-emission `va_start` store path except for
  narrow consumer adjustments required by a finalized producer contract.
- External variadic call lowering such as
  `PreparedCallWrapperKind::DirectExternVariadic`.
- General RV64 parameter-home expansion, tracked separately in
  `ideas/open/363_rv64_object_parameter_home_coverage.md`.
- Scalar `va_arg`, `va_copy`, aggregate `va_arg`, or broad variadic ABI
  rewrites not needed to publish the overflow-area initial base state.
- Testcase-name matching, source-pattern shortcuts, unsupported expectation
  downgrades, allowlist filtering, or scan-only classification changes.

## Acceptance Criteria

- Supported RV64 variadic entries publish explicit overflow-area initial base
  state in prepared facts, including the base slot and stack offset needed by
  prepared `va_start` lowering.
- Focused tests show the producer/runtime-state contract is present before
  object emission consumes it.
- Helper-free RV64 variadic entry remains rejected unless the explicit runtime
  contract has been materialized.
- `src/920908-1.c` is rerun and its outcome is documented; progress must be a
  real producer/runtime-state advance, not a renamed diagnostic.
- Existing focused backend coverage for RV64 object emission, prepared frame
  stack call contracts, prepared printer output, and variadic-entry diagnostics
  remains green.

## Reviewer Reject Signals

- Reject named-case-only handling for `src/920908-1.c` or matching its exact
  source shape instead of publishing general prepared RV64 variadic entry
  state.
- Reject object-emission guesses that synthesize an overflow-area base without
  an explicit prepared producer/runtime contract.
- Reject helper renames, expectation rewrites, unsupported downgrades, or
  classification-only changes claimed as overflow-base-state production
  progress.
- Reject broad RV64 parameter-home or external variadic-call rewrites that do
  not directly produce the `va_start` overflow-area initial base state.
- Reject retaining the old missing-base-state failure behind a new abstraction
  name or diagnostic spelling.

## Starting Signal

Representative validation from idea 364:

- focused backend tests passed
- `src/920908-1.c` failed at
  `unsupported_variadic_helper_lowering: RV64 va_start helper requires prepared overflow-area initial base state`
- case log:
  `build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log`

## Closure Note

Closed after Step 3 validation/classification. RV64 prepared variadic entry
state now publishes the explicit overflow-area initial base state for
supportable helper-bearing entries, including the base slot and prepared stack
offset consumed by `va_start` lowering.

Focused backend prepared-contract, prepared-printer, and RV64 object-emission
tests passed. The representative `src/920908-1.c` case was rerun and advanced
past the original missing overflow-area base-state diagnostic. Its remaining
failure is the later destination-address boundary:

`unsupported_variadic_helper_lowering: RV64 va_start helper requires destination va_list address in a prepared GPR home`

That residual is outside this focused overflow-base-state producer milestone
and is tracked separately in
`ideas/open/366_rv64_va_start_destination_va_list_gpr_home.md`.
