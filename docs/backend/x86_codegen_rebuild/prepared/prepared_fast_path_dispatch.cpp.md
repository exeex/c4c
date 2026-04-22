# `prepared/prepared_fast_path_dispatch.cpp`

Primary role: implement bounded prepared fast-path admission and delegation.

Owned inputs:

- prepared query context, prepared route-shape facts, and module-level dispatch
  requests
- canonical lowering services that can satisfy the shape once admitted

Owned outputs:

- admission decisions for guarded prepared shapes
- explicit fallback paths into canonical lowering when the fast path does not
  remain thin
- adapter-level sequencing that never claims semantic ownership of the lowered
  operation

Allowed indirect queries:

- `prepared/prepared_query_context.hpp`
- `prepared/prepared_fast_path_operands.hpp`
- `lowering/comparison_lowering.hpp`
- `lowering/call_lowering.hpp`
- `lowering/memory_lowering.hpp`
- `core/x86_codegen_output.hpp`

Forbidden knowledge:

- local reconstruction of stack layout or slot addressing
- local compare/branch lowering policy
- module-data emission or symbol rendering

Role classification:

- `prepared`
