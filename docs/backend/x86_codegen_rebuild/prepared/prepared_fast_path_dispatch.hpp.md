# `prepared/prepared_fast_path_dispatch.hpp`

Primary role: declare thin prepared fast-path admission and fallback
decisions.

Owned inputs:

- prepared query context and prepared-route shape facts
- module/function-level requests from the module layer

Owned outputs:

- declarations for bounded fast-path eligibility checks and dispatch results
- explicit fallback contracts that return control to canonical lowering when a
  shape is unsupported or would reopen hidden policy

Allowed indirect queries:

- `prepared/prepared_query_context.hpp`
- `prepared/prepared_fast_path_operands.hpp`
- `lowering/comparison_lowering.hpp`
- `lowering/call_lowering.hpp`
- `lowering/memory_lowering.hpp`

Forbidden knowledge:

- direct ownership of stack-home formation, call-lane ordering, or predicate
  semantics
- module-data emission
- route-debug summary rendering

Role classification:

- `prepared`
