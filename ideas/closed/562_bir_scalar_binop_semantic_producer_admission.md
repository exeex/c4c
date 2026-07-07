# BIR Scalar Binop Semantic Producer Admission

Status: Closed
Activation Priority: Deferred until the supervisor selects the scalar-binop lane.
Type: Producer implementation follow-up
Parent: `ideas/closed/545_bir_semantic_producer_admission_reconstruction.md`
Owning Layer: BIR semantic producer
Split From: `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`

## Goal

Repair BIR scalar-binop semantic producer admission for the current
scalar-binop row without claiming broad scalar progress from a single
representative.

## Why This Exists

The combined scalar/signature/control route was split because producer
inspection proved that the common `latest function failure` module note is
only a failure publication funnel. The scalar-binop failure is produced inside
the instruction-lowering boundary:
`lower_scalar_or_local_memory_inst()` in
`src/backend/bir/lir_to_bir/memory/coordinator.cpp` calls scalar helpers in
`src/backend/bir/lir_to_bir/scalar.cpp`, including
`lower_scalar_binary_opcode()` and `lower_scalar_binop_operands()`, before
emitting `bir::BinaryInst`.

That boundary is independent from scalar-control-flow CFG/terminator/phi
lowering and function-signature lowering.

## In Scope

- Add focused BIR tests for scalar-binop semantic fact publication or
  fail-closed admission.
- Repair scalar binary opcode or operand lowering at the real instruction
  producer boundary.
- Prove the representative RV64 scalar-binop row `src/960513-1.c` function
  `f`, or a current stronger substitute if the row inventory changes.
- Identify any nearby same-feature scalar-binop rows before claiming family
  progress.

## Out Of Scope

- Scalar-control-flow CFG, terminator, or phi-lowering producer repair.
- Function-signature return-info or parameter-layout producer repair.
- Broad scalar rewrites not justified by scalar-binop producer evidence.
- RV64 lowering, expectation rewrites, unsupported downgrades, allowlist
  changes, or weaker semantic admission checks.

## Acceptance Criteria

- Focused BIR coverage exists for the scalar-binop instruction producer
  boundary.
- Missing scalar-binop facts are published or rejected fail-closed at the real
  BIR instruction-lowering boundary.
- `src/960513-1.c` function `f`, or a stronger substitute, passes or advances
  beyond the original BIR semantic admission diagnostic because scalar-binop
  BIR facts are correct.
- The result is reported as scalar-binop progress only; broader scalar claims
  require additional same-feature evidence.

## Reviewer Reject Signals

- Reject named-case handling for only `src/960513-1.c`, `f`, or a particular
  operand shape instead of a semantic scalar-binop opcode or operand rule.
- Reject claiming broad scalar-lane progress from this single row without
  nearby same-feature evidence.
- Reject fixes that only change the outer `latest function failure` note,
  row classification, expectation files, unsupported markers, or allowlists.
- Reject patches that move the failure into RV64 lowering without proving the
  scalar-binop BIR instruction facts are correct.
- Reject broad scalar rewrites that retain the exact same binop admission
  failure behind new helper names.

## Closure Notes

Closed after focused F128 named scalar arithmetic coverage and implementation
admitted `fneg`, `fmul`, and `fsub` through the general scalar-binop opcode and
operand path. The representative `src/960513-1.c` function `f` advances beyond
the original BIR `scalar-binop semantic family` boundary and its generated
`llvm_gcc_c_torture_src_960513_1_c` test passes.

Remaining scalar-cast, scalar/local-memory, alloca local-memory, RV64 object,
and vector arithmetic stops are separate owner families. The vector arithmetic
residual exposed by `src/simd-6.c` is tracked separately in
`ideas/open/585_bir_vector_binop_semantic_producer_admission.md`.
