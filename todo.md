Status: Active
Source Idea Path: ideas/open/183_phase_e_route5_edge_join_source_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select Consumer And Baseline Behavior

# Current Packet

## Just Finished

Step 1 selected `aarch64_codegen::build_current_block_join_prepared_query_routing(...)`
as the Route 5 migration consumer. This AArch64 join parallel-copy
source-recovery boundary feeds
`current_block_join_prepared_query_incoming_expression(...)` and
`current_block_join_prepared_query_source(...)` in
`src/backend/mir/aarch64/codegen/dispatch.cpp`, where matching incoming/source
instructions are skipped because prepared edge/join publication policy already
owns the block-entry parallel copy behavior.

Current prepared fallback/oracle behavior to preserve:

- The consumer first asks `mir::find_bir_current_block_join_source_identity(...)`
  for semantic join-source identity and currently falls back when that query is
  unavailable.
- Fallback builds or reuses `PreparedValueHomeLookups` and
  `PreparedEdgePublicationLookups`, then calls
  `prepare::prepare_current_block_join_parallel_copy_source_facts(...)`.
- The fallback preserves prepared value ids/names for incoming-expression
  closure and source-value identity, including named, immediate, and stack
  source cases.
- Prepared move-bundle and edge-publication facts remain the oracle for move
  policy: `PreparedMoveBundle`, `PreparedMoveResolution`,
  `PreparedEdgePublicationLookups`, and the edge publication records attached
  to `PreparedCurrentBlockJoinParallelCopySourceFacts`.
- Fail-closed prepared statuses to preserve include missing edge-publication
  lookups and unsupported moves; Route 5 fail-closed statuses already exercised
  nearby include missing successor/block, missing publication/no PHI, missing
  source producer, missing source value, missing destination, and destination
  type mismatch.

Focused existing coverage:

- `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`
  (`backend_aarch64_current_block_join_routing`) covers the selected consumer
  with both available semantic identity and prepared fallback routing.
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
  (`current_block_join_query_routing_uses_bir_identity_with_prepared_fallback`,
  inside `backend_aarch64_instruction_dispatch`) covers the same routing through
  the larger AArch64 dispatch test.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  (`verify_current_block_join_parallel_copy_source_query`, inside
  `backend_prepared_lookup_helper`) covers prepared-vs-BIR current-block
  join-source oracle equivalence, Route 5 current-block join records/indexes,
  named/immediate/stack source cases, unsupported move fail-closed behavior,
  and Route 5 missing/no-match cases.

## Suggested Next

Execute Step 2 by adding or exposing a narrow local Route 5 query boundary for
`build_current_block_join_prepared_query_routing(...)`, keyed by successor block,
destination value, and source value through `Route5EdgeJoinSourceIndex` /
`route5_find_current_block_join_source(...)`, while preserving the current
prepared fallback path unchanged for absent, incomplete, or mismatched Route 5
data.

## Watchouts

- Keep this migration to one selected consumer.
- Preserve prepared edge-copy and move-bundle helpers as fallback/oracle
  surfaces; Route 5 should provide only semantic current-block join-source
  identity for this consumer.
- Do not claim prepared API deletion or broad aggregate contraction.
- Do not replace Route 5 owned records with predecessor or BIR rescans.
- `mir::find_bir_current_block_join_source_identity(...)` currently rebuilds a
  Route 5 index internally in `src/backend/mir/query.cpp`; Step 2 should avoid
  broad rescans at the consumer by exposing a local/indexed boundary rather than
  adding another whole-function scan.

## Proof

Discovery-only packet; no build/tests run and no `test_after.log` produced.
Proposed implementation proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_current_block_join_routing|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'`

Broader supervisor-requested backend proof, if needed after implementation:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
