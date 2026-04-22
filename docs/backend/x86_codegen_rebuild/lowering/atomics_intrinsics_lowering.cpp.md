# `lowering/atomics_intrinsics_lowering.cpp`

Primary role: implement atomic operations and explicit ISA intrinsic lowering.

Owned inputs:

- atomic operations, ordering, pointer/value operands, and intrinsic opcodes
- register/XMM facts and canonical operand-home services

Owned outputs:

- atomic load/store/RMW/cmpxchg/fence sequences
- explicit intrinsic instruction sequences such as SSE helpers and nontemporal
  stores

Allowed indirect queries:

- `lowering/memory_lowering.hpp`
- `abi/x86_target_abi.hpp`
- `core/x86_codegen_output.hpp`

Forbidden knowledge:

- prepared matcher or dispatcher ownership
- module/data emission
- generic route-debug reporting

Role classification:

- `lowering`
