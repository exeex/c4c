# `lowering/scalar_lowering.hpp`

Primary role: declare the canonical scalar arithmetic, cast, width-conversion,
and integer-unary seam.

Owned inputs:

- integer/scalar operands, unary and binary opcodes, and type-width facts
- register assignments and output services from `core`

Owned outputs:

- declarations for scalar arithmetic, shift, cast, and width-conversion
  lowering services
- narrow scalar helpers that collect the current `alu.cpp`, `cast_ops.cpp`,
  and adjacent scalar-only responsibilities without growing a matcher bucket

Allowed indirect queries:

- `core/x86_codegen_types.hpp`
- `core/x86_codegen_output.hpp`
- `abi/x86_target_abi.hpp`
- `lowering/memory_lowering.hpp` when scalar work needs canonical homes

Forbidden knowledge:

- prepared fast-path selection
- module-level routing or data emission
- x87-only long-double ownership, which belongs in `float_lowering`

Role classification:

- `lowering`
