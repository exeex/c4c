# `prepared/prepared_query_context.hpp`

Primary role: declare the narrow prepared-route query surface that adapters use
to ask canonical owners for facts.

Owned inputs:

- prepared module/function metadata needed to choose among bounded fast paths
- stable handles into canonical frame, call, memory, comparison, and output
  services

Owned outputs:

- a query-shaped context type that exposes prepared names, control-flow facts,
  and value-home lookups without granting ownership of lowering policy
- declarations for the minimum adapter-facing queries needed by prepared fast
  paths

Allowed indirect queries:

- `core/x86_codegen_types.hpp`
- `lowering/frame_lowering.hpp`
- `lowering/call_lowering.hpp`
- `lowering/memory_lowering.hpp`
- `lowering/comparison_lowering.hpp`

Forbidden knowledge:

- no direct assembly rendering policy for calls, returns, addresses, or
  compares
- no module-data emission ownership
- no debug rendering helpers beyond exposing facts for the debug layer

Role classification:

- `prepared`
