# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory common query authority and establish the guard

## Just Finished

- Step 1 inventoried every common `mir/query.*` Route 1-8 spelling and
  classified it by semantic owner and migration family: Route 1 producer
  semantics (33 hits), Route 2 select-chain semantics (29), Route 3 memory
  semantics (71), Route 4 publication semantics (31), Route 5 edge/join
  semantics (133), and no Route 6-8 hits. The only route-shaped public payload
  is `Route5EdgeJoinSourceIndex`; all current families are BIR-owned source
  semantics rather than prepared placement or target materialization.
- Added registered contract `backend_common_mir_query_route_authority_guard`.
  It rejects direct route headers, exact-inventory drift, new public route
  records/indexes, route-analysis entry points, and generic `find_bir_*`
  wrappers that conceal the still-present route-backed implementation.
- Identified the directly affected registered proof surface: x86
  `backend_x86_shared_producer_query`; AArch64
  `backend_aarch64_prepared_memory_operand_records`,
  `backend_aarch64_current_block_join_routing`, and
  `backend_aarch64_instruction_dispatch`; RV64
  `backend_riscv_prepared_edge_publication` and
  `backend_riscv_object_emission`; plus shared
  `backend_prepared_lookup_helper`, `backend_store_source_publication_plan`,
  and `backend_prealloc_block_entry_publications`.

## Suggested Next

- Step 2: migrate the bounded Route 1 same-block producer and integer-constant
  family to the existing named BIR producer view, preserving the common query
  result identities and fail-closed behavior. Ratchet Route 1's guard count
  downward with the migration; do not broaden into target materializers.

## Watchouts

- `bir.hpp` remains the umbrella include, so the guard separately rejects any
  direct route header and inventories route payload/entry-point spellings.
- Route 5 is the only public-header breach and the largest family; leave its
  indexed edge/join migration for a later bounded packet.
- Keep target materializer migration in ideas 708-710 and stack-destination
  authority work in idea 707.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' | tee test_after.log`: 330/330 backend
  tests passed. Proof log: `test_after.log`.
