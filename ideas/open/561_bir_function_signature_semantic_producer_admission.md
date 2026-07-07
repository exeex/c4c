# BIR Function Signature Semantic Producer Admission

Status: Open
Activation Priority: Deferred until the supervisor selects the function-signature lane.
Type: Producer implementation follow-up
Parent: `ideas/closed/545_bir_semantic_producer_admission_reconstruction.md`
Owning Layer: BIR semantic producer
Split From: `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`

## Goal

Repair BIR function-signature semantic producer admission for the current
function-signature rows in the smaller semantic admission lane.

## Why This Exists

The combined scalar/signature/control route was split because producer
inspection proved that the common `latest function failure` module note is
only a failure publication funnel. Function-signature failures are produced
before block and instruction lowering when `infer_function_return_info()` or
`lower_function_params()` fails. The concrete parameter producer is
`lower_function_params_with_layouts()` in
`src/backend/bir/lir_to_bir/call_abi.cpp`.

That boundary is independent from scalar-control-flow CFG/terminator/phi
lowering and scalar-binop instruction lowering.

## In Scope

- Add focused BIR tests for function-signature semantic fact publication or
  fail-closed admission.
- Repair function return-info or parameter-layout producer behavior at the BIR
  signature boundary.
- Prove representative RV64 function-signature rows, including
  `src/20050316-3.c` function `test1`, or current stronger substitutes.
- Compare nearby same-family rows where practical, including rows such as
  `src/20071029-1.c`, `src/ieee/pr72824-2.c`, `src/pr60960.c`,
  `src/pr70903.c`, `src/pr71626-1.c`, `src/pr71626-2.c`, `src/simd-6.c`, and
  `src/zero-struct-2.c`.

## Out Of Scope

- Scalar-control-flow CFG, terminator, or phi-lowering producer repair.
- Scalar-binop instruction lowering.
- Treating the failure as RV64 ABI or object-emission work before BIR
  signature publication is proven correct.
- Expectation rewrites, unsupported downgrades, allowlist changes, or weaker
  semantic admission checks.

## Acceptance Criteria

- Focused BIR coverage exists for function return-info and/or parameter-layout
  signature publication, matching the inspected failure boundary.
- Missing function-signature facts are published or rejected fail-closed at the
  real BIR producer boundary.
- Representative RV64 function-signature rows pass or advance beyond the
  original BIR semantic admission diagnostic because the BIR signature facts
  are correct.
- Remaining ABI or RV64 lowering failures, if any, are recorded as downstream
  owner boundaries instead of folded into this idea.

## Reviewer Reject Signals

- Reject routing signature failures directly to RV64 ABI or object emission
  without focused proof that BIR signature publication is already correct.
- Reject fixes that only change the outer `latest function failure` note,
  row classification, expectation files, unsupported markers, or allowlists.
- Reject named-case handling for only `src/20050316-3.c`, `test1`, or another
  representative instead of a semantic return-info or parameter-layout rule.
- Reject broad call/ABI rewrites that retain the same BIR function-signature
  admission failure.
- Reject claims of signature-family progress if nearby same-family rows remain
  unsupported or unexamined without a recorded downstream owner.
