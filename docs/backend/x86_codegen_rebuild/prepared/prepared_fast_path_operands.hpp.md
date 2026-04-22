# `prepared/prepared_fast_path_operands.hpp`

Primary role: declare prepared-only operand helpers that translate prepared
facts into canonical lowering queries.

Owned inputs:

- prepared value-home, naming, and control-flow facts
- canonical frame and memory services exposed through the prepared query
  context

Owned outputs:

- declarations for operand-query helpers that are narrow enough to stay as
  adapters instead of becoming a second addressing subsystem
- prepared-side utility types for bounded operand lookup results

Allowed indirect queries:

- `prepared/prepared_query_context.hpp`
- `lowering/frame_lowering.hpp`
- `lowering/memory_lowering.hpp`
- `core/x86_codegen_types.hpp`

Forbidden knowledge:

- no local frame layout construction
- no local call-lane or return-value policy
- no direct branch or compare ownership

Role classification:

- `prepared`
