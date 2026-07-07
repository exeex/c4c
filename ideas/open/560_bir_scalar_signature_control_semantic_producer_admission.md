# BIR Scalar Control Flow Semantic Producer Admission

Status: Open
Activation Priority: Active after split from the combined scalar/signature/control lane.
Type: Producer implementation follow-up
Parent: `ideas/closed/545_bir_semantic_producer_admission_reconstruction.md`
Owning Layer: BIR semantic producer
Split From: combined scalar-control-flow, function-signature, and scalar-binop lane.
Related Ideas:
- `ideas/open/561_bir_function_signature_semantic_producer_admission.md`
- `ideas/open/562_bir_scalar_binop_semantic_producer_admission.md`

## Goal

Repair BIR scalar-control-flow semantic producer admission for the current
scalar-control-flow rows in the smaller semantic admission lane.

## Why This Exists

The first active inspection packet proved that the prior combined
scalar/signature/control route did not share one repairable BIR producer
boundary. The visible `latest function failure` module note is only a failure
publication funnel.

Scalar-control-flow failures are produced by CFG, terminator, and phi-lowering
paths, including `BirFunctionLowerer::lower()`,
`collect_phi_lowering_plans()`, `lower_block_phi_insts()`,
`initialize_aggregate_phi_state()`, `apply_pending_aggregate_phi_copies()`,
and `lower_block_terminator()` in `src/backend/bir/lir_to_bir/module.cpp`.
That boundary is independent from function-signature lowering and scalar-binop
instruction lowering.

## In Scope

- Add focused BIR tests for scalar-control-flow semantic fact publication or
  fail-closed admission.
- Repair the real CFG, terminator, or phi-lowering producer boundary that
  emits or rejects scalar-control-flow facts.
- Prove representative RV64 scalar-control-flow rows, including
  `src/20000314-3.c` function `attr_eq`, or current stronger substitutes.
- Compare nearby same-family rows where practical so proof is not
  testcase-shaped.

## Out Of Scope

- Function-signature lowering; it is tracked separately by
  `ideas/open/561_bir_function_signature_semantic_producer_admission.md`.
- Scalar-binop instruction lowering; it is tracked separately by
  `ideas/open/562_bir_scalar_binop_semantic_producer_admission.md`.
- RV64 ABI/object emission, local-memory, call metadata, runtime/intrinsic,
  bootstrap/global data-shape, or unrelated BIR producer work.
- Expectation rewrites, unsupported downgrades, allowlist changes, or weaker
  semantic admission checks.

## Acceptance Criteria

- Focused BIR coverage exists for the scalar-control-flow producer boundary.
- The missing scalar-control-flow semantic facts are published or rejected
  fail-closed at the real BIR producer boundary.
- Representative RV64 scalar-control-flow rows pass or advance beyond the
  original BIR semantic admission diagnostic because the BIR facts are correct.
- Remaining downstream failures, if any, are assigned to their own owner
  boundary instead of expanding this idea.

## Reviewer Reject Signals

- Reject fixes that only change the outer `latest function failure` note,
  row classification, expectation files, unsupported markers, or allowlists
  while retaining the same scalar-control-flow producer failure.
- Reject named-case handling for only `src/20000314-3.c`, `attr_eq`, or another
  representative instead of a semantic CFG, terminator, or phi-lowering rule.
- Reject patches that claim scalar-control-flow progress by modifying
  function-signature lowering, scalar-binop instruction lowering, RV64 ABI, or
  object emission without proving the scalar-control-flow BIR facts are
  correct.
- Reject broad backend rewrites that do not add focused scalar-control-flow BIR
  evidence.
- Reject a route that leaves nearby scalar-control-flow rows unexamined while
  claiming only one representative as family progress.
