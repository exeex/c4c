# `lowering/frame_lowering.cpp`

Primary role: implement frame sizing, prologue/epilogue emission, and incoming
parameter home publication.

Owned inputs:

- function frame requirements, parameter lists, and ABI frame policy
- output/state services from `core`

Owned outputs:

- concrete stack-size computation
- callee-save setup and teardown
- parameter home publication and frame-home lookup services reused by other
  lowering files

Allowed indirect queries:

- `abi/x86_target_abi.hpp`
- `core/x86_codegen_output.hpp`
- `core/x86_codegen_types.hpp`

Forbidden knowledge:

- prepared fast-path matcher ownership
- module/data emission decisions
- direct compare or call-result semantics beyond frame coordination

Role classification:

- `lowering`
