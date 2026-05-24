# AArch64 Scalar ALU Fallback Operand Phase Extraction

## Intent

Extract one scalar `alu.cpp` helper group for fallback operand selection into a
small phase-local implementation boundary without changing scalar ALU lowering
behavior or widening `alu.hpp` exposure.

This optional follow-up exists only if the extraction can stay focused on one
helper group and remain behavior-preserving.

## Target Files

- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp` only if the existing public surface
  must stay aligned.
- One new or existing phase-local implementation unit only if it reduces
  fallback operand responsibility mixing without increasing header exposure.

Do not use this idea for full scalar ALU decomposition.

## Refactor Type

`Phase extraction`

Use exactly this slice type. Do not combine it with helper absorption, header
contraction, naming repair, or control-publication redesign.

## Durable Owner

Scalar ALU fallback operand-selection phase logic.

The durable owner is the target-local scalar phase that decides how fallback
operands are selected from prepared homes and same-block producer facts. It is
not generic operand lowering and not final instruction emission.

## Scope

- Select only the fallback operand-selection helper group.
- Extract implementation details into a phase-local boundary if doing so makes
  ownership clearer.
- Keep `alu.hpp` as narrow as it is today or narrower; do not expose private
  fallback concepts broadly.
- Preserve prepared-home lookup, fallback operand selection, and same-block
  producer behavior.
- Leave control-publication materialization for a separate idea unless the
  chosen helper group cannot be separated without changing behavior.

## Behavior-Preservation Proof

Expected proof for a future implementation run:

- build proof for the AArch64 backend target
- focused scalar tests covering:
  - scalar ALU lowering
  - fallback operand selection
  - same-block producer operands
  - prepared-home lookup behavior relevant to fallback operands

If control-publication behavior is touched, this idea is too broad unless the
future packet explicitly narrows the proof and scope first.

## Reject Signals

Reject the implementation route if it requires any of the following:

- expectation downgrades or unsupported-test conversions
- testcase-shaped shortcuts or named-case matching
- hidden semantic changes to scalar ALU lowering, fallback operand selection,
  prepared-home lookup, or same-block producer behavior
- target-specific instruction or register logic moved into generic layers
- full `alu.cpp` decomposition or broad scalar pipeline redesign
- broad renames without durable concept proof
- reducing file count while increasing responsibility mixing
- widening `alu.hpp` with private phase helpers

## Acceptance

- One fallback operand-selection helper group has a clearer phase-local home.
- Scalar ALU lowering behavior and generated output are unchanged.
- The slice is small enough for one focused future run.
