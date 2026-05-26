Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Shared Edge Publication Facts

# Current Packet

## Just Finished

Completed the Step 2 prepared edge-publication lookup-key hardening slice.

Replaced the bit-packed XOR edge-publication index key with an explicit
target-neutral `PreparedEdgePublicationKey` tuple over predecessor label,
successor label, and destination value id, with equality/hash support for the
lookup map. Added focused shared coverage proving distinct overlapping
successor/value-id and predecessor/successor id ranges remain separate through
the public prepared edge-publication lookup helpers.

## Suggested Next

Continue Step 2 by extending the shared edge-publication facts with
target-neutral source classification/dependency records for plain home reads,
rematerializable immediates, load-local/cast/binary/select materialization, and
parallel-copy cycle/temp-save ordering.

## Watchouts

- The edge-publication map may still have normal hash-bucket collisions, but
  key equality now compares the full logical tuple instead of relying on packed
  bit ranges.
- `PreparedEdgePublication` currently records source value id/name only when
  the source is a named prepared value with a known value home. Immediate and
  computed-expression classification is still the next Step 2 slice.
- `destination_storage_kind` comes from the matching block-entry move when
  available, otherwise from the destination home kind. It deliberately does not
  interpret target register spelling.
- The current unique lookup returns `nullptr` on ambiguity; callers that need
  to diagnose duplicate publications should use the vector lookup.

## Proof

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prealloc_block_entry_publications|backend_prepare_authoritative_join_ownership|backend_prepared_lookup_helper|backend_publication_plan_record)$"'`
passed. Proof log: `test_after.log`.
