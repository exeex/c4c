Status: Active
Source Idea Path: ideas/open/589_direct_edge_publication_move_freshness_ownership.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Strengthen Observability And Closure Inventory

# Current Packet

## Just Finished

Step 4 wired `prepare_current_block_join_parallel_copy_source_facts` to require
the selected `DirectEdgePublicationSource` freshness authority before accepting
named direct edge-publication sources from
`prepare_block_entry_parallel_copy_edge_source_facts`.

`PreparedCurrentBlockJoinParallelCopySourceFact` now carries the source
freshness candidates/status/selection, and named direct sources fail closed
through missing, invalid, or ambiguous source-freshness statuses unless the
selected authority has the exact source value id/name, use kind, source kind,
proof kind, rank, edge publication, and move identity. Immediate edge sources
remain authority-free. Focused backend helper tests cover accepted explicit
freshness plus missing/no-candidate, invalid, ambiguous, wrong-value,
wrong-use, and destination-only rejection, while preserving existing missing
publication, ambiguous publication, edge mismatch, unsupported move,
publication/move mismatch, and source-fact checks.

## Suggested Next

Start Step 5 by strengthening observability and closure inventory for the
direct edge-publication freshness route, including any printer/status exposure
or inventory checks needed before lifecycle closure.

## Watchouts

- Step 4 intentionally gates only named direct edge-publication sources;
  immediate edge sources still do not publish or require freshness authority.
- Destination-only freshness remains invalid for this route; do not replace
  the exact `DirectEdgePublication` publication/move authority with home-based
  placement evidence.
- The new edge-copy source freshness statuses currently map to generic missing
  BIR edge-publication source status in helper expectations; Step 5 should
  decide whether additional observability needs a more specific external
  status/printer surface.

## Proof

Delegated proof passed on rerun:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.
Proof log: `test_after.log`.
