# `lowering/frame_lowering.hpp`

Primary role: declare the canonical frame-layout and prologue/epilogue seam.

Owned inputs:

- ABI stack-alignment and callee-save policy from `abi`
- function-local frame requirements and parameter-home publication needs

Owned outputs:

- declarations for frame sizing, prologue setup, epilogue teardown, and
  incoming-parameter home publication
- narrow frame-service types that prepared adapters may query instead of
  recreating frame policy locally

Allowed indirect queries:

- `abi/x86_target_abi.hpp`
- `core/x86_codegen_types.hpp`
- `core/x86_codegen_output.hpp`

Forbidden knowledge:

- prepared fast-path route selection
- module/data emission orchestration
- call-result publication and return-register semantics

Role classification:

- `lowering`
