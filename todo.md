Status: Active
Source Idea Path: ideas/open/699_prealloc_named_bir_proof_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 3-4
Current Step Title: Add Or Reuse The BIR-To-Prealloc Named View Adapter / Migrate One Prealloc Proof Consumer

# Current Packet

## Just Finished

Completed `plan.md` Steps 3-4 for the selected proof consumer. Added the
`bir::validate_block_entry_publication_reference(const BirPublicationView&,
const Block&, const Value&)` adapter beside the existing current-block
publication view adapter, and migrated only
`prepare::attribute_route4_block_entry_publication_if_agreeing` to build a
`BirPublicationView` with `make_bir_publication_view`.

The helper no longer reads the public Route 4 availability index directly for
the reference validation path. Its local rollback/agreement path still builds a
`Route4BlockEntryPublicationRecord` from the same BIR successor/value evidence
to preserve the existing attribution fields and fail-closed checks. Prepared
executable authority remains unchanged: availability, destination identity,
destination home/name/id, move bundle, and move resolution still come from the
prepared records before Route 4 attribution is considered.

## Suggested Next

Migrate the next explicitly selected prealloc proof consumer to named
publication view vocabulary, or route this slice through supervisor review if a
checkpoint is desired before another consumer moves.

## Watchouts

Do not remove the older public-index
`validate_block_entry_publication_reference(const Route4PublicationAvailabilityIndex&, ...)`
adapter yet; other callers may still depend on that facade vocabulary. The
prepared-printer path still reaches this lookup indirectly, but this packet did
not edit printer vocabulary, dump rows, route dump expectations, unsupported
markers, allowlists, or tests.

## Proof

Delegated proof passed:
`bash -lc "(cmake --build --preset default && ./build/tests/backend/bir/backend_prealloc_block_entry_publications_test) 2>&1 | tee test_after.log"`.

Proof log: `test_after.log`.
