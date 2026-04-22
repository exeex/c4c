# `lowering/call_lowering.cpp`

Primary role: implement canonical call setup, call issuance, cleanup, and
post-call result publication.

Owned inputs:

- call targets, operands, ABI argument classes, and return-type metadata
- frame-home and output services

Owned outputs:

- stack-space sizing and alignment for calls
- register-vs-stack argument moves
- direct/indirect call emission
- post-call result moves into canonical destinations

Allowed indirect queries:

- `abi/x86_target_abi.hpp`
- `core/x86_codegen_output.hpp`
- `lowering/frame_lowering.hpp`

Forbidden knowledge:

- prepared route admission logic or shape recognition
- module entry selection
- data-section rendering

Role classification:

- `lowering`
