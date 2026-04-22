# `lowering/float_lowering.cpp`

Primary role: implement scalar floating arithmetic and the legacy `f128`/x87
support path.

Owned inputs:

- float operands, float opcodes, unary float requests, and `f128` address/home
  state
- canonical memory services for stack-based float access

Owned outputs:

- scalar floating arithmetic sequences
- floating unary helpers that naturally belong with the float family
- `f128` load/store/publication helpers that remain one visible capability

Allowed indirect queries:

- `lowering/memory_lowering.hpp`
- `abi/x86_target_abi.hpp`
- `core/x86_codegen_output.hpp`

Forbidden knowledge:

- prepared route dispatch
- module-level symbol or data emission
- integer-scalar cast orchestration that belongs in `scalar_lowering`

Role classification:

- `lowering`
