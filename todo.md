Status: Active
Source Idea Path: ideas/open/589_direct_edge_publication_move_freshness_ownership.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: State And Encode The Ownership Rule

# Current Packet

## Just Finished

Step 2 encoded the direct edge-publication source ownership rule in shared
freshness vocabulary without wiring the consumer. Added
`PreparedValueFreshnessUseKind::DirectEdgePublicationSource`,
`PreparedValueFreshnessSourceKind::DirectEdgePublication`,
`PreparedValueFreshnessProofKind::DirectEdgePublicationMove`, and
`PreparedValueFreshnessSourceRank::DirectEdgePublication`, plus an explicit
`edge_publication` freshness reference. The shared lookup now treats that use
kind as valid only when the authority uses the direct-edge source/proof/rank
tuple and carries both the prepared edge publication and exact move reference.

Focused lookup tests assert the stable names, valid direct edge-publication
selection, and fail-closed behavior for missing edge publication, missing move,
wrong use, and destination-only/direct-home authority. This intentionally does
not change `prepare_block_entry_parallel_copy_edge_source_facts` or any backend
target emission behavior yet.

## Suggested Next

Start Step 3 by publishing/querying
`DirectEdgePublicationSource` freshness for
`prepare_block_entry_parallel_copy_edge_source_facts`. The source authority
should be the direct edge-publication row plus the matching block-entry
parallel-copy `PreparedMoveResolution`; the proof should be
`DirectEdgePublicationMove`; the rank should be `DirectEdgePublication`.

## Watchouts

- Destination-only authority remains insufficient because direct homes and
  destination bundle legality prove placement/availability, not that the
  selected edge-publication source is fresh for the exact predecessor-successor
  move.
- Step 3 should preserve existing fail-closed statuses for missing publication,
  ambiguous publication, edge mismatch, unsupported move, publication/move
  mismatch, and missing source facts, then add freshness failure mapping without
  weakening those checks.
- Keep consumer behavior unchanged until Step 3; this packet only established
  the shared ownership vocabulary and selector contract.

## Proof

Delegated proof passed on rerun after an initial unrelated `cc1plus` kill while
compiling `backend_aarch64_instruction_dispatch_test`:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.
Proof log: `test_after.log`.
