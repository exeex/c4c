# `lowering/comparison_lowering.hpp`

Primary role: declare the shared compare, branch, and select seam.

Owned inputs:

- compare operands, compare opcodes, target blocks, and result destinations
- scalar and float type facts relevant to predicate lowering

Owned outputs:

- declarations for integer, floating, and `f128` compare materialization
- fused compare-and-branch and select services that prepared fast paths may
  reuse without owning predicate semantics themselves

Allowed indirect queries:

- `core/x86_codegen_output.hpp`
- `core/x86_codegen_types.hpp`
- `lowering/memory_lowering.hpp` for operand/home access only
- `abi/x86_target_abi.hpp` when predicate lowering needs target facts

Forbidden knowledge:

- prepared route selection
- module-level orchestration
- standalone debug or proof-surface formatting

Role classification:

- `lowering`
