# `lowering/comparison_lowering.cpp`

Primary role: implement canonical compare, branch, and select materialization.

Owned inputs:

- compare operands, compare opcodes, branch targets, and result destinations
- scalar/float type facts and canonical operand homes

Owned outputs:

- integer and floating compare sequences
- fused compare-and-branch lowering
- select materialization over canonical predicate policy

Allowed indirect queries:

- `lowering/memory_lowering.hpp`
- `core/x86_codegen_output.hpp`
- `abi/x86_target_abi.hpp`
- `lowering/float_lowering.hpp` for `f128` compare support only

Forbidden knowledge:

- prepared fast-path route ownership
- module/data emission
- debug summary generation

Role classification:

- `lowering`
