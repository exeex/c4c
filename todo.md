Status: Active
Source Idea Path: ideas/open/589_direct_edge_publication_move_freshness_ownership.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Or Locate Freshness For The Selected Route

# Current Packet

## Just Finished

Step 3 published direct edge-publication source freshness from
`prepare_block_entry_parallel_copy_edge_source_facts` without changing consumer
failure behavior. `PreparedEdgeCopySourceFacts` now carries freshness
candidate/status/selection fields, and the block-entry parallel-copy route
builds a selected `DirectEdgePublicationSource` authority only after the
existing exact publication, exact move, and source-value checks succeed.

The published authority uses source `DirectEdgePublication`, proof
`DirectEdgePublicationMove`, rank `DirectEdgePublication`, and references both
the exact `PreparedEdgePublication` and exact `PreparedMoveResolution`. Focused
backend helper tests prove the selected authority is visible for the valid
named direct route while route-agnostic, immediate-source, and missing-source
facts do not fabricate freshness. Existing missing publication, ambiguous
publication, edge mismatch, unsupported move, publication/move mismatch, and
source-fact statuses remain preserved.

## Suggested Next

Start Step 4 by wiring the direct edge-publication consumer to read the
published `DirectEdgePublicationSource` freshness authority and map missing,
ambiguous, or invalid source freshness to the Step 4 consumer status without
weakening the existing publication/move/source-fact checks.

## Watchouts

- Destination-only authority remains insufficient because direct homes and
  destination bundle legality prove placement/availability, not that the
  selected edge-publication source is fresh for the exact predecessor-successor
  move.
- Consumer wiring should use the selected authority already stored on
  `PreparedEdgeCopySourceFacts`; do not recreate freshness from destination
  homes or route-agnostic edge facts.
- Missing freshness is still not enforced by this Step 3 packet. Step 4 should
  add that mapping at the consumer boundary while preserving all pre-existing
  fail-closed statuses.

## Proof

Delegated proof passed on rerun after an initial unrelated `cc1plus` kill while
compiling `backend_aarch64_instruction_dispatch_test`:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.
Proof log: `test_after.log`.
