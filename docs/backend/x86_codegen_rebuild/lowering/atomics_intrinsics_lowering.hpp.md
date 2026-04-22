# `lowering/atomics_intrinsics_lowering.hpp`

Primary role: declare the low-level target-specific seam for atomics and
explicit x86 intrinsics.

Owned inputs:

- atomic operation kinds, ordering, operands, and intrinsic opcodes
- target-specific register/XMM facts from `abi`

Owned outputs:

- declarations for atomic load/store/RMW/cmpxchg/fence services
- declarations for explicit ISA intrinsic lowering helpers that remain inside
  the canonical lowering stack

Allowed indirect queries:

- `abi/x86_target_abi.hpp`
- `core/x86_codegen_output.hpp`
- `core/x86_codegen_types.hpp`
- `lowering/memory_lowering.hpp` for canonical operand homes

Forbidden knowledge:

- prepared dispatcher ownership
- module-level data or symbol emission
- generic route-debug reporting

Role classification:

- `lowering`
