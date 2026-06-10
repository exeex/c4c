# 163 BIR edge and join-source annotation schema

## Goal

Prototype BIR edge and block annotations for CFG edge publication and
join-source identity established by Phase A Route 5.

## Why This Exists

Phase A proved edge publication identity keyed by predecessor, successor, and
destination value/name, plus source value/name/kind, source producer
block/instruction, optional memory-source identity, no-source identity, and
current-block join source facts. Phase B classified these as edge and block
annotation candidates while rejecting parallel-copy execution policy.

## In Scope

- Define edge annotations for predecessor/successor/destination/source identity,
  source producer block/instruction, optional memory-source identity, and
  no-source markers.
- Define block annotations for current-block join incoming source identity.
- Keep prepared edge publication, edge-copy source, and join-source queries as
  migration oracles.
- Preserve predecessor/successor/value keyed lookup without importing move
  scheduling.

## Out Of Scope

- Move bundle order, parallel-copy step order, cycle temporary routing,
  execution site/block placement, phase or carrier policy, coalescing and
  redundancy flags, destination registers, storage-sharing checks, and prepared
  move records.

## Acceptance Criteria

- BIR annotations can answer edge and join-source semantic identity questions
  independently of parallel-copy scheduling.
- Existing prepared edge/join answers remain available as oracles until
  consumers switch safely.
- No-source and memory-source cases are represented as semantic identities, not
  as absence of prepared move data.

## Reviewer Reject Signals

- Reject if the annotation payload includes move bundle order, cycle temporary
  routing, execution site policy, phase/carrier resources, destination physical
  registers, storage-sharing checks, or prepared move records.
- Reject if only a single edge-copy testcase is recognized by predecessor and
  successor names while other edge, no-source, and join-source cases remain
  unexamined.
- Reject expectation downgrades that turn edge-copy or join-source supported
  behavior into unsupported behavior without explicit user approval.
- Reject if BIR edge annotations are just wrappers around
  `PreparedEdgePublication` records rather than field-level semantic facts.
- Reject broad rewrites of parallel-copy scheduling or MIR move emission.

## Closure Notes

Closed after implementing Route 5 BIR edge and current-block join-source
records, BIR-owned lookup/index helpers, oracle tests for edge and join-source
identity, and a low-risk CFG edge publication source MIR query migration.

Reviewer report `review/163_edge_join_source_route_review.md` found no
blocking issues and judged closure reasonable. The residual boundary is that
`mir::find_bir_current_block_join_source_identity(...)` remains on its prior
semantic implementation while Route 5 current-block join records/indexes are
implemented and oracle-covered; this was accepted because the source idea
requires BIR-owned schema and migration oracles, not every consumer migration
in this slice.

Full-suite baseline review after Step 3 was accepted at `3427/3427`.
Closure validation used canonical broad backend logs from:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

`test_before.log` and `test_after.log` both passed `179/179`, and the
regression guard passed with no new failures or pass-count regression.
