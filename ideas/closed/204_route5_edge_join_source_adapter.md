# Route 5 edge/join-source identity adapter

## Goal

Migrate one selected edge or join-source reader or wrapper boundary to Route 5
BIR records while retaining prepared move scheduling and edge-copy policy.

## Why This Exists

The Phase B2 readiness audit classifies Route 5 schema as sufficient for CFG
edge publication and current-block join-source identity. It also records that
wrapper or edge-copy uses need a compatibility adapter because parallel-copy
scheduling, homes, move bundles, branch spelling, scratch policy, execution
site, coalescing, and final edge-copy records remain outside BIR schema.

This idea is the bounded follow-up for the Phase B2 Route 5 candidate:
selected edge/join-source identity or wrapper adapter.

## In Scope

- Select exactly one CFG edge, join-source, edge-copy, or wrapper reader that
  needs Route 5 semantic source identity.
- Use Route 5 records or indexes for predecessor/successor/destination/source
  identity, source value/name/kind, source producer block/instruction,
  optional memory-source identity, no-source status, and current-block join
  incoming source identity.
- Preserve prepared fallback for move scheduling, storage/homes, branch policy,
  wrapper formatting, and final edge-copy behavior.
- Prove no-source and memory-source status, duplicate/conflict rejection,
  absent/mismatch fallback, and adjacent branch or parallel-copy sanity when
  wrappers are touched.

## Out Of Scope

- Replacing all edge-publication, move-bundle, or current-block join helper
  groups.
- Moving parallel-copy scheduling, source/destination homes, move order,
  branch spelling, scratch policy, cycle temporary routing, execution-site
  placement, coalescing, final edge-copy records, or wrapper formatting into
  BIR schema.
- Broad edge/join lookup contraction.

## Acceptance Criteria

- The selected reader obtains only Route 5 semantic edge or join-source
  identity from BIR.
- Prepared fallback remains for move policy, storage, scheduling, wrappers, and
  emitted output.
- Proof covers success, no-source, memory-source, duplicate/conflict,
  absent/mismatch fallback, and output stability where relevant.

## Closure Note

Closed after selecting `build_current_block_join_prepared_query_routing` as the
bounded reader boundary and adding fail-closed indexed Route 5 current-block
join-source validation through `mir::find_bir_current_block_join_source_identity`.
The selected AArch64 boundary consumes Route 5 only for semantic identity
bitsets when the adapter reports availability, while prepared scheduling,
storage, wrappers, branch policy, final edge-copy records, and emitted output
remain authoritative.

Close proof used backend CTest before/after logs for the same
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
scope. The close-time guard passed with 180 backend tests passing before and
after, no new failures, and no pass-count decrease.

## Reviewer Reject Signals

- Reject a slice that uses Route 5 join identity as ownership of move
  scheduling, storage/home policy, branch spelling, cycle temporaries, final
  edge-copy records, or wrapper formatting.
- Reject wrapper-only proof without conflict, no-source, memory-source, and
  fallback coverage.
- Reject output-string changes without string-authority proof.
- Reject broad replacement of edge/join, move-bundle, or helper lookup groups.
- Reject a named-case shortcut that proves only one fixture shape while nearby
  edge/join cases remain unsupported or unexamined.
