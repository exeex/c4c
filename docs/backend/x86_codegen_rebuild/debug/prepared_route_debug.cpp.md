# `debug/prepared_route_debug.cpp`

Primary role: implement prepared-route summaries and traces as observational
proof surfaces.

Owned inputs:

- prepared route facts, prepared query results, and dispatcher outcomes
- focused user/debug selections such as function or block filters

Owned outputs:

- human-readable route summaries and trace strings
- focused debug facts that explain why a prepared fast path matched or fell
  back without owning the lowering itself

Allowed indirect queries:

- `prepared/prepared_query_context.hpp`
- `prepared/prepared_fast_path_dispatch.hpp`
- `core/x86_codegen_types.hpp`

Forbidden knowledge:

- emitting final assembly
- mutating canonical lowering policy
- becoming a hidden utility layer for non-debug callers

Role classification:

- `debug`
