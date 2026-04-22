# `lowering/memory_lowering.hpp`

Primary role: declare the canonical addressing, load/store, and stack-home
formation seam.

Owned inputs:

- pointer/value operands, type metadata, and frame-home information
- ABI size/alignment facts needed for address formation

Owned outputs:

- declarations for stack-slot resolution, direct/indirect/overaligned address
  formation, and typed load/store helpers
- narrow memory services that prepared adapters and other lowering families
  must query instead of rebuilding operand formation locally

Allowed indirect queries:

- `core/x86_codegen_types.hpp`
- `core/x86_codegen_output.hpp`
- `abi/x86_target_abi.hpp`
- `lowering/frame_lowering.hpp`

Forbidden knowledge:

- prepared dispatch decisions
- module/global data-section routing
- compare/branch ownership beyond exposing operands to comparison lowering

Role classification:

- `lowering`
