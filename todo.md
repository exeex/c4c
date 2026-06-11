Status: Active
Source Idea Path: ideas/open/183_phase_e_route5_edge_join_source_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Route/Prepared Equivalence Coverage

# Current Packet

## Just Finished

Step 4 tightened `backend_aarch64_current_block_join_routing` around the
selected current-block join-source reader. The fixture now checks explicit
per-instruction route/prepared routing bits for normal predecessor coverage
with and without attached prepared policy, a missing predecessor-block shape,
no-source fallback, load-local stack/memory-source routing, and absent-route
prepared fallback. The expectations stay strict and do not weaken existing
oracles.

## Suggested Next

Supervisor should decide whether Step 4 is sufficient for closure or whether a
review/plan-owner handoff is needed for the next lifecycle action.

## Watchouts

- The Route 5 boundary provides semantic source identity only; do not move
  prepared move-bundle, source/destination home, scheduling, or final edge-copy
  policy into BIR.
- The indexed MIR query intentionally treats missing indexed publication data as
  unavailable so `build_current_block_join_prepared_query_routing(...)` can fall
  back to prepared facts.
- The current-block Route 5 identity is semantic source identity; the prepared
  fallback still owns value-home and move-policy details.
- The missing predecessor-block fixture documents current selected-reader
  behavior: the current-block source identity remains available because this
  reader reasons from the join block source expression, not from predecessor
  edge-copy policy.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_current_block_join_routing|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'; } > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
