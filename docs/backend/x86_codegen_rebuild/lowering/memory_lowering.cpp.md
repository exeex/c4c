# `lowering/memory_lowering.cpp`

Primary role: implement canonical addressing, stack-slot resolution, and typed
load/store helpers.

Owned inputs:

- pointer/value operands, frame-home information, and type metadata
- alignment and size facts from `abi`

Owned outputs:

- direct, indirect, and overaligned address formation
- typed load/store sequences
- reusable operand-home queries for other lowering families

Allowed indirect queries:

- `lowering/frame_lowering.hpp`
- `abi/x86_target_abi.hpp`
- `core/x86_codegen_output.hpp`

Forbidden knowledge:

- prepared route dispatch
- module/data emission
- compare/branch semantics except where memory operands feed those owners

Role classification:

- `lowering`
