# `lowering/return_lowering.cpp`

Primary role: implement return-register publication, epilogue coordination, and
final `ret` flow.

Owned inputs:

- optional return operands, function return types, and frame-size context
- ABI return-lane policy from `abi`

Owned outputs:

- integer, float, `i128`, and `f128` result publication
- final epilogue handoff into the canonical return sequence

Allowed indirect queries:

- `abi/x86_target_abi.hpp`
- `core/x86_codegen_output.hpp`
- `lowering/frame_lowering.hpp`
- `lowering/float_lowering.hpp` for `f128` support only

Forbidden knowledge:

- prepared route matching
- module-level orchestration
- call-argument placement or compare ownership

Role classification:

- `lowering`
