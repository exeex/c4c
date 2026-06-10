Status: Active
Source Idea Path: ideas/open/154_bir_memory_access_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add BIR-Owned Memory Access Queries

# Current Packet

## Just Finished

Step 3 BIR-owned same-block global-load access identity query is complete.

Implemented `BirSameBlockGlobalLoadAccessRequest`,
`BirSameBlockGlobalLoadAccessIdentity`, and
`find_bir_same_block_global_load_access_identity` in
`src/backend/mir/query.hpp/.cpp`. The query resolves a same-block scalar
producer by block/root value/before-instruction identity, requires a direct
`LoadGlobalInst` producer, and then reuses `find_bir_memory_access_identity`
for the direct memory identity. It exposes only semantic BIR facts:
producer identity, load-global pointer, result value identity, global symbol
identity, address space, volatile flag, and semantic base kind.

Added test-only prepared/BIR comparisons against
`find_prepared_same_block_global_load_access` in:
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
  for a direct global-symbol load positive, before-producer fail-closed,
  root-type mismatch, and string-load fail-closed path.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` for the existing
  same-block scalar producer fixture, including direct global-load positive,
  before-producer fail-closed, non-global root fail-closed, and mismatched root
  type rejection.

No consumers were switched to the BIR query in this packet.

## Suggested Next

Next coherent packet: add BIR-owned same-block load-local source identity
queries for the semantic source relationship currently exposed through
prepared same-block load-local source helpers, comparing against prepared
oracles in tests while keeping consumers on prepared queries.

## Watchouts

- Keep `review/phase_a_steps_1_4_route_review.md` untouched.
- Keep prepared memory/access queries available as the oracle until broader
  BIR equivalence is proven.
- Do not switch `dispatch_value_materialization.cpp`,
  `fp_value_materialization.cpp`, `alu.cpp`, `calls.cpp`, `globals.cpp`, or
  memory-retargeting consumers before the supervisor delegates that consumer
  migration.
- The same-block global-load access query intentionally requires semantic
  `GlobalSymbol` memory identity, but still does not import target address
  formation, offset legality, layout, relocation/GOT/TLS policy, or register
  homes.
- `find_prepared_same_block_load_local_stored_value_source` and
  `find_prepared_same_block_load_local_source_producer` currently depend on
  stack-layout range matching/overlap for correctness. A BIR query can expose
  the semantic source answer after equivalence, but should not own frame
  offsets, concrete sizes, or overlap math as BIR data.

## Proof

`cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_prepared_lookup_helper_test && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_prepared_lookup_helper)$' > test_after.log 2>&1`

Passed. `test_after.log` contains the delegated proof log with both selected
tests passing.
