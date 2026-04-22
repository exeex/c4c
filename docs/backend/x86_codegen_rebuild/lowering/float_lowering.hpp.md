# `lowering/float_lowering.hpp`

Primary role: declare the canonical floating-point seam, including the legacy
`f128`/x87 path.

Owned inputs:

- floating operands, float opcodes, and float type metadata
- canonical memory/home services for `f128` address access

Owned outputs:

- declarations for scalar floating arithmetic and unary helpers
- declarations for `f128` load/store/return support so long-double policy has
  one visible owner

Allowed indirect queries:

- `core/x86_codegen_output.hpp`
- `core/x86_codegen_types.hpp`
- `lowering/memory_lowering.hpp`
- `abi/x86_target_abi.hpp`

Forbidden knowledge:

- prepared route matching
- module/data emission orchestration
- integer-scalar cast ownership that belongs in `scalar_lowering`

Role classification:

- `lowering`
