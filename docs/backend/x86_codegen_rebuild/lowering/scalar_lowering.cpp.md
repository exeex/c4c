# `lowering/scalar_lowering.cpp`

Primary role: implement scalar arithmetic, shifts, casts, and integer-unary
operations.

Owned inputs:

- scalar operands, integer opcodes, cast type pairs, and width/sign metadata
- register/home information from `core` and `memory_lowering`

Owned outputs:

- ALU instruction sequences
- cast and width-conversion sequences
- shared scalar helper routines for register/immediate shaping

Allowed indirect queries:

- `lowering/memory_lowering.hpp`
- `abi/x86_target_abi.hpp`
- `core/x86_codegen_output.hpp`

Forbidden knowledge:

- prepared route selection
- module orchestration
- long-double/x87 ownership that belongs in `float_lowering`

Role classification:

- `lowering`
