Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Shared Edge Publication Facts

# Current Packet

## Just Finished

Completed the Step 2 shared edge-publication source/dependency facts slice.

Extended `PreparedEdgePublication` with target-neutral source classification,
optional source home kind, same-source/destination value identity, matching
move coalescing/redundancy, and parallel-copy step/bundle ordering metadata
derived from existing prepared edge-transfer, value-home, move-resolution, and
parallel-copy bundle records. Added focused shared coverage for named source
homes, immediate sources, named sources without homes, same-value/coalesced
copies, and cycle/temp-save step ordering metadata.

## Suggested Next

Continue Step 2 by deciding whether the remaining computed-expression source
families need separate prepared producer facts before edge publications can
classify load-local, cast, binary, and select materialization without scanning
BIR producer instructions.

## Watchouts

- Immediate and named-source-without-home cases are now classified at the edge
  publication level, but computed load-local/cast/binary/select producer
  identity is still not available as prepared data here.
- `matching_move_redundant_by_assigned_storage` is currently true for explicit
  coalesced moves or same source/destination value id; it does not infer
  physical register spelling equivalence.
- Parallel-copy step metadata is copied only when the matching prepared move
  carries a `source_parallel_copy_step_index`; no target instruction or
  MachineInstruction details are consulted.

## Proof

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prealloc_block_entry_publications|backend_prepare_authoritative_join_ownership|backend_prepared_lookup_helper|backend_publication_plan_record)$"'`
passed. Proof log: `test_after.log`.
