Status: Active
Source Idea Path: ideas/open/687_structured_layout_bridge_isolation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Structured Layout Bridge Ownership

# Current Packet

## Just Finished

Completed `plan.md` Step 1 inventory for the structured type/layout bridge.

Findings:
- `TypeDeclMap` is adapter-private compatibility state. It is built in
  `types.cpp` from legacy raw LIR type declarations and is still consumed by
  aggregate/layout fallback paths; it is not public BIR, prepared/prealloc,
  target, MIR, initializer, memory/provenance, or call ABI ownership.
- Legacy type text parsing lives in `types.cpp` through
  `compute_aggregate_type_layout`, `resolve_type_decl_body`, scalar storage
  parsing, array parsing, and inline struct parsing. It produces stable layout
  facts but remains an import-local compatibility parser.
- Typed operand parsing lives in `types.cpp` as `parse_typed_operand` and is
  used broadly by memory/addressing/GEP/call-adjacent adapter code. It is
  adapter-private LIR spelling parsing, not a downstream policy surface.
- Structured layout fallback lives in `types.cpp` through
  `BackendStructuredLayoutTable`, `BackendStructuredLayoutEntry`,
  `build_backend_structured_layout_table`, parity notes, and
  `lookup_backend_aggregate_type_layout_result`. It prefers structured layout
  when available and falls back to legacy text only for no-id/rendered-text
  bridge callers.
- Aggregate lookup in `aggregate.cpp` still has rendered-text bridge points:
  `selected_aggregate_type_layout`, `lower_byval_aggregate_layout`,
  `collect_sorted_leaf_slots`, `append_local_aggregate_scalar_slots`,
  `declare_local_aggregate_slots`, and local aggregate copy validation. The
  existing `selected_aggregate_type_ref_layout` path is the narrower
  StructNameId-aware lookup.
- `memory_helpers.hpp` is already a narrow pure layout/projection declaration
  surface for scalar layout facts, aggregate byte-offset projection,
  child-index projection, and reinterpret checks. It should not absorb
  stateful lowerer policy.

Selected first contraction:
- Next packet should stay in `src/backend/bir/lir_to_bir/aggregate.cpp` and
  `todo.md`, with `types.cpp` only if a declaration adjustment is needed.
  Narrow `materialize_aggregate_param_aliases`/local aggregate param setup so
  structured byval parameter slots created from `collect_aggregate_params`
  reuse the already-computed `AggregateParamInfo::layout` for validation or
  slot construction instead of re-entering the rendered-text fallback through
  `declare_local_aggregate_slots(info.type_text, ...)`.
- This is structured-layout bridge work because it reduces one adapter-private
  rendered-type fallback for byval aggregate parameter materialization while
  preserving the produced BIR local slots and load/store copies. It should not
  touch public BIR type/model authority, prepared/prealloc, target transport,
  MIR, initializer, memory/provenance policy, call ABI placement, tests,
  expectations, unsupported markers, or allowlists.

## Suggested Next

Implement the selected contraction in `aggregate.cpp`: add the minimal helper
or overload needed for aggregate parameter alias materialization to consume the
known `AggregateParamInfo::layout` without broadening `memory_helpers.hpp` or
changing downstream behavior, then run the delegated backend proof.

## Watchouts

Do not rename the fallback without reducing a real rendered-text lookup site.
The typed-operand parser has broad memory/addressing consumers, so moving it is
not the first contraction. Avoid changes outside the structured-layout bridge,
especially target aggregate transport, byval ABI placement, prepared storage,
MIR consumers, initializer lowering, memory/provenance policy, tests,
expectations, unsupported markers, and allowlists.

## Proof

Inventory-only packet; changed `todo.md` only. No compile proof was required,
and no `test_after.log` was produced.
