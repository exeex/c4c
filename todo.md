Status: Active
Source Idea Path: ideas/open/163_bir_edge_join_source_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate A Low-Risk Query Consumer

# Current Packet

## Just Finished

Completed Step 4 for
`ideas/open/163_bir_edge_join_source_annotation_schema.md`.
Migrated the narrow shared Route 5 MIR edge-publication source query to read
Route 5 BIR records/index helpers:

- `mir::find_bir_cfg_edge_publication_source_identity(...)` now builds a
  function-local Route 5 BIR index from the request blocks and answers through
  `bir::route5_find_cfg_edge_publication(...)`.
- The migrated helper maps Route 5 statuses back into the existing public MIR
  result shape and preserves direct source producer, memory-source, source
  value, destination PHI, and explicit missing/unavailable statuses.
- Multi-predecessor PHI lookup preserves the Step 3 fix by carrying other
  incoming predecessor labels into the rebuilt Route 5 function before
  querying the requested predecessor edge.
- Current-block join-source MIR consumers were intentionally left unchanged.

Focused `backend_prepared_lookup_helper_test.cpp` coverage now keeps prepared
edge-copy reads as oracles and adds assertions tying the migrated MIR answer to
indexed Route 5 BIR records for load-local memory-source identity, explicit
missing-source status, and wrong-predecessor/no-source fail-closed behavior.

No target/codegen, prealloc production helper, parallel-copy scheduling, move
emission, or storage/publication-policy consumer was migrated.

## Suggested Next

Execute Step 5 by running broader backend acceptance validation for the Route
5 edge/join-source schema migration and recording closure readiness.

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
- Step 4 switched only
  `mir::find_bir_cfg_edge_publication_source_identity(...)`.
- `mir::find_bir_current_block_join_source_identity(...)` remains intentionally
  unswitched and oracle-covered for later work.
- Route 5 indexes are rebuildable from BIR records and must remain
  target-neutral lookup helpers, not prepared edge-publication lookup
  containers.

## Proof

Exact delegated Step 4 proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`1/1` matching backend test passed).
