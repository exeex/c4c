Status: Active
Source Idea Path: ideas/open/183_phase_e_route5_edge_join_source_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prefer Route 5 In The Selected Consumer

# Current Packet

## Just Finished

Step 3 required no code change because Step 2 already completed the selected
consumer preference behavior. `build_current_block_join_prepared_query_routing`
builds a local `Route5EdgeJoinSourceIndex`, calls
`mir::find_bir_current_block_join_source_identity(...)` with that indexed
boundary, and returns Route 5-derived routing immediately when the identity is
`Available`. Only absent, incomplete, or invalid Route 5 data reaches the
prepared fallback path that builds/reuses `PreparedValueHomeLookups` and
`PreparedEdgePublicationLookups`, then calls
`prepare::prepare_current_block_join_parallel_copy_source_facts(...)`.

The existing focused routing coverage already demonstrates the selected reader
can route from Route 5 without attached prepared policy, and the prepared helper
coverage demonstrates incomplete indexed Route 5 data remains unavailable so
the selected reader can preserve prepared fallback.

## Suggested Next

Execute Step 4 by adding or tightening route/prepared equivalence coverage for
the selected current-block join-source reader, including normal predecessor,
missing-predecessor, no-source, memory-source, and absent-route cases without
weakening existing oracle expectations.

## Watchouts

- The Route 5 boundary provides semantic source identity only; do not move
  prepared move-bundle, source/destination home, scheduling, or final edge-copy
  policy into BIR.
- The indexed MIR query intentionally treats missing indexed publication data as
  unavailable so `build_current_block_join_prepared_query_routing(...)` can fall
  back to prepared facts.
- Step 4 should focus on coverage, not broad prepared aggregate API churn.

## Proof

No proof run for this packet because Step 3 required no production or test code
change. The Step 2 focused proof passed and was rolled forward by the supervisor
to `test_before.log`.
