# `debug/prepared_route_debug.hpp`

Primary role: declare route-summary and trace surfaces for prepared-path proof
and debugging.

Owned inputs:

- prepared route facts, prepared query context facts, and canonical admission
  decisions that are safe to expose for diagnostics

Owned outputs:

- declarations for route summaries, traces, and focused debug views that help
  explain prepared routing
- debug-only types that stay outside the lowering and prepared ownership paths

Allowed indirect queries:

- `prepared/prepared_query_context.hpp`
- `prepared/prepared_fast_path_dispatch.hpp`
- `core/x86_codegen_types.hpp`

Forbidden knowledge:

- machine-code emission ownership
- fast-path admission decisions beyond reporting them
- module-data emission or canonical lowering semantics

Role classification:

- `debug`
