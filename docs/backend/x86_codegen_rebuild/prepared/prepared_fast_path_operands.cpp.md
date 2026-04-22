# `prepared/prepared_fast_path_operands.cpp`

Primary role: implement prepared operand-query helpers as thin consumers of
canonical frame and memory seams.

Owned inputs:

- prepared value-home, naming, and control-flow metadata
- canonical frame and memory services reachable through the prepared query
  context

Owned outputs:

- prepared operand lookup helpers that convert prepared facts into canonical
  homes, registers, and memory operands
- bounded adapter logic that supports fast paths without rebuilding the old
  `prepared_local_slot_render.cpp` subsystem

Allowed indirect queries:

- `prepared/prepared_query_context.hpp`
- `lowering/frame_lowering.hpp`
- `lowering/memory_lowering.hpp`
- `core/x86_codegen_output.hpp`

Forbidden knowledge:

- independent frame-layout synthesis
- independent call-lane or return register policy
- direct dispatch ownership or route-debug rendering

Role classification:

- `prepared`
