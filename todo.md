Status: Active
Source Idea Path: ideas/open/163_bir_edge_join_source_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Lookup/Index Helpers

# Current Packet

## Just Finished

Completed Step 3 for
`ideas/open/163_bir_edge_join_source_annotation_schema.md`.
Added function-local Route 5 lookup/index helpers over the BIR edge and
current-block join-source annotation records without switching production
consumers:

- `Route5EdgeJoinSourceIndex` groups Route 5 edge records, current-block
  join-source records, and value-link records rebuilt from a `bir::Function`.
- `route5_build_edge_join_source_index(...)` rebuilds edge records from PHI
  incoming predecessor/successor/destination tuples and join-source records
  from block-local PHI incoming facts.
- `route5_find_cfg_edge_publication(...)` looks up predecessor/successor/
  destination keyed edge answers by stable BIR block label/id and destination
  value name/type.
- `route5_find_current_block_join_source(...)` looks up successor/destination/
  source keyed join-source answers by stable BIR block label/id and value
  name/type or immediate source identity.
- The helpers return explicit Route 5 statuses for memory-source, no-source,
  missing-source, missing destination, missing successor, and no-match/type
  mismatch cases instead of relying on absent records.

Focused `backend_prepared_lookup_helper_test.cpp` coverage now proves:

- Edge index success for load-local memory-source publication, including Route
  3 memory identity preservation.
- Edge index success for multi-predecessor PHIs where the requested predecessor
  is not the first successor/destination candidate.
- Edge index fail-closed behavior for missing source producer, destination
  type mismatch, missing destination, wrong predecessor/no-source, and wrong
  successor.
- Current-block join index success for named and immediate PHI incoming
  sources.
- Current-block join index fail-closed behavior for destination type mismatch,
  missing source value, missing destination, and missing source producer.

No MIR, target/codegen, or prealloc production consumer was switched.

## Suggested Next

Execute Step 4 by migrating the narrowest shared MIR Route 5 edge or
join-source query consumer to read Route 5 BIR record/index helpers while
preserving prepared/MIR oracle checks.

## Watchouts

- Keep Route 5 annotations target-neutral and semantic: edge and block records
  own CFG/value/source identity, not parallel-copy execution policy.
- Do not import move bundle order, parallel-copy step order, cycle temporary
  routing, execution site/block placement, phase or carrier policy, coalescing
  or redundancy flags, destination registers, storage-sharing checks,
  `PreparedEdgePublication`, or prepared move records.
- Do not import target/prealloc storage details as schema: destination homes,
  destination storage kind, destination registers, source/destination storage
  sharing, register views, stack-source extension policy, and aggregate copy
  authority remain prepared/target policy.
- Explicit no-source and memory-source records must be representable as
  semantic identities; absence alone should not stand in for unavailable or
  no-source when an edge/join identity exists.
- Step 2 intentionally did not switch
  `mir::find_bir_cfg_edge_publication_source_identity(...)` or
  `mir::find_bir_current_block_join_source_identity(...)`.
- Step 3 intentionally did not switch those MIR consumers either.
- Route 5 indexes are rebuildable from BIR records and must remain
  target-neutral lookup helpers, not prepared edge-publication lookup
  containers.

## Proof

Exact delegated Step 3 proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`1/1` matching backend test passed).
