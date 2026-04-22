# `lowering/return_lowering.hpp`

Primary role: declare the canonical return-value publication seam.

Owned inputs:

- function return types, optional return operands, and frame-size context
- ABI return-lane policy from `abi`

Owned outputs:

- declarations for return-register publication, epilogue handoff, and special
  return-family helpers such as `i128` and `f128`
- narrow return services that other files call instead of open-coding return
  register policy

Allowed indirect queries:

- `abi/x86_target_abi.hpp`
- `core/x86_codegen_output.hpp`
- `core/x86_codegen_types.hpp`
- `lowering/frame_lowering.hpp` for epilogue coordination

Forbidden knowledge:

- prepared-route matcher state
- module-level route selection or data emission
- compare/branch or call-argument ownership

Role classification:

- `lowering`
