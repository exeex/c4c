Status: Active
Source Idea Path: ideas/open/183_phase_e_route5_edge_join_source_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Or Expose Route 5 View Boundary

# Current Packet

## Just Finished

Step 2 added an indexed Route 5 current-block join-source boundary for
`aarch64_codegen::build_current_block_join_prepared_query_routing(...)`.
`mir::BirCurrentBlockJoinSourceRequest` can now carry a
`Route5EdgeJoinSourceIndex`, and the MIR query uses
`route5_find_current_block_join_source(...)` keyed by successor block,
destination value, and source value when that index is supplied.

The selected AArch64 consumer now builds the local Route 5 index from the
current BIR function and passes it into the MIR semantic identity query before
the existing prepared fallback path. The prepared fallback still builds/reuses
`PreparedValueHomeLookups` and `PreparedEdgePublicationLookups`, then calls
`prepare::prepare_current_block_join_parallel_copy_source_facts(...)` whenever
the Route 5 identity is unavailable.

`backend_prepared_lookup_helper` now directly covers the indexed MIR boundary
for an available Route 5 identity and verifies an incomplete Route 5 index stays
unavailable, preserving fail-closed prepared fallback behavior.

## Suggested Next

Execute Step 3 by switching the selected reader to prefer the valid Route 5
semantic source records from the new indexed boundary, while keeping prepared
helpers as fallback/oracle surfaces for absent, incomplete, or invalid Route 5
data.

## Watchouts

- Keep Step 3 limited to the selected current-block join-source consumer.
- The Route 5 boundary provides semantic source identity only; do not move
  prepared move-bundle, source/destination home, scheduling, or final edge-copy
  policy into BIR.
- The indexed MIR query intentionally treats missing indexed publication data as
  unavailable so `build_current_block_join_prepared_query_routing(...)` can fall
  back to prepared facts.

## Proof

Ran the supervisor-selected proof:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_current_block_join_routing|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'; } > test_after.log 2>&1`

Result: passed; 3/3 focused tests passed. Proof log: `test_after.log`.
