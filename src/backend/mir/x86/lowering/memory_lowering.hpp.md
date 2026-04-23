# `lowering/memory_lowering.hpp` contract

## Legacy Evidence

The legacy memory helper surface exposed too much: operand rendering, carrier
state, pointer-family reasoning, and even mini-lowering orchestration.

## Design Contract

This header now owns only narrow x86 memory-operand helpers.

Owned inputs:

- scalar type kinds
- byte offsets for stack-relative operands

Owned outputs:

- size tokens for scalar memory accesses
- stack memory operand text

Must not own:

- semantic lowering policy
- prepared value-home archaeology
- ad hoc register reuse rules
