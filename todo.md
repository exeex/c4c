Status: Active
Source Idea Path: ideas/open/154_bir_memory_access_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove BIR-Representable Memory Equivalence

# Current Packet

## Just Finished

Step 3 BIR-owned same-block load-local source identity query is complete, and
the runbook has been rewritten after
`review/154_memory_access_step3_route_review.md` to narrow Step 4/Step 5 around
the BIR-representable semantic equivalence boundary.

Implemented `BirSameBlockLoadLocalSourceRequest`,
`BirSameBlockLoadLocalSourceIdentity`, and
`find_bir_same_block_load_local_source_identity` in
`src/backend/mir/query.hpp/.cpp`. The query resolves a same-block root
producer by block/root value/before-instruction identity, requires a direct
`LoadLocalInst` producer, reuses `find_bir_memory_access_identity` for the
direct local-slot memory identity, and fail-closes on representable
intervening same-slot `StoreLocalInst` facts before the consuming instruction.
It exposes only BIR semantic facts: producer identity, load-local pointer,
result value identity, local slot/name identity, address space, volatile flag,
and semantic base kind.

Added test-only prepared/BIR comparisons against
`find_prepared_same_block_load_local_source_producer` in:
- `tests/backend/mir/backend_store_source_publication_plan_test.cpp` for the
  indexed load-local source positive path, before-producer fail-closed,
  root-type mismatch, same-slot intervening-store fail-closed, and the
  prepared-layout boundary where prepared can prove a non-overlapping store but
  BIR conservatively fails closed without owning range overlap facts.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` for the existing
  same-block scalar producer fixture, including direct load-local positive,
  before-producer fail-closed, root-type mismatch, and same-slot
  intervening-store fail-closed.

No consumers were switched to the BIR query in this packet.

## Suggested Next

Next executable packet: implement Step 4 proof for BIR-representable memory
equivalence.

The executor should add or tighten prepared/BIR comparison coverage for local,
global, pointer, string, volatile, same-block global-load, load-local
stored-value/source, store-source, and unavailable paths where the answer
depends only on BIR-owned semantic facts: operation identity, source identity,
result/stored names, address space, volatile flag, semantic base kind, and
same-block producer/source links.

The proof should also keep explicit boundary coverage for prepared-only
positives that depend on stack-layout range non-overlap. Those cases remain
prepared oracle/fallback behavior while BIR fails closed; they are not BIR
parity requirements and should not be used to justify importing frame offsets,
concrete sizes, alignment, or overlap math into BIR.

Do not switch consumers in this packet.

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
- `find_bir_same_block_load_local_source_identity` intentionally fail-closes on
  any representable same-slot store between the load and consumer. Prepared
  can still prove some non-overlapping same-slot stores safe through stack
  layout range math; that remains oracle-only evidence and was not copied into
  BIR.
- `find_prepared_same_block_load_local_stored_value_source` still depends on
  prepared stack-layout overlap/match facts for prior-store source recovery.
  Do not import frame offsets, concrete sizes, or range overlap math into BIR
  to chase exact prepared equivalence.
- Step 5 consumer migration is blocked for any load-local source path whose
  correctness depends on prepared stack-layout range overlap/non-overlap proof.
- The rewritten Step 4 proof target is semantic equivalence, not exact prepared
  parity for layout-derived positives.

## Proof

`cmake --build build --target backend_store_source_publication_plan_test backend_prepared_lookup_helper_test && ctest --test-dir build --output-on-failure -R '^(backend_store_source_publication_plan|backend_prepared_lookup_helper)$' > test_after.log 2>&1`

Passed. `test_after.log` contains the delegated proof log with both selected
tests passing.
