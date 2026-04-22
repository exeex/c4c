# `lowering/call_lowering.hpp`

Primary role: declare the canonical x86 call-lowering seam.

Owned inputs:

- operand lists, call targets, and return-type metadata
- ABI argument and result classification policy from `abi`
- frame and output services from `core`

Owned outputs:

- declarations for stack-space sizing, register-vs-stack argument placement,
  call issuance, and post-call result publication
- narrow call-service entrypoints that prepared adapters must consume when a
  fast path still issues a real call

Allowed indirect queries:

- `abi/x86_target_abi.hpp`
- `core/x86_codegen_types.hpp`
- `core/x86_codegen_output.hpp`
- `lowering/frame_lowering.hpp` for frame-home interaction only

Forbidden knowledge:

- prepared shape matching and dispatcher ownership
- module/global/string-data emission
- debug-trace summary ownership

Role classification:

- `lowering`
