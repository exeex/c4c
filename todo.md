Status: Active
Source Idea Path: ideas/open/699_prealloc_named_bir_proof_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map The First Prealloc Route 4 Consumer

# Current Packet

## Just Finished

Completed `plan.md` Step 1 mapping for the first Route 4 prealloc proof
consumer.

Selected consumer: `prepare::attribute_route4_block_entry_publication_if_agreeing`
in `src/backend/prealloc/prepared_lookups.cpp`, reached from
`prepare::find_prepared_current_block_entry_publication`.

Old route entry point: the consumer builds a local
`bir::Route4PublicationAvailabilityIndex` with
`bir::route4_build_publication_availability_index` and reads it through
`bir::validate_block_entry_publication_reference(
const Route4PublicationAvailabilityIndex&, const Block&, const Value&)`.

Named publication view entry point for the implementation packet:
`bir::validate_block_entry_publication_reference(
const BirPublicationView&, const Block&, const Value&)`, backed by
`bir::make_bir_publication_view`. The current named publication view already
exists for current-block publication proof; the first implementation packet
should add or reuse the matching block-entry overload as the narrow
BIR-to-prealloc boundary adapter.

Rollback point: keep the old local Route 4 index build plus
`Route4PublicationAvailabilityIndex` overload as private compatibility, and
switch only this consumer back to that old call path if the named-view
migration fails.

## Suggested Next

Add the block-entry overload on `BirPublicationView`, then migrate only
`attribute_route4_block_entry_publication_if_agreeing` to call the named view
while preserving the existing attribution fields and fallback behavior.

## Watchouts

The selected consumer must remain diagnostic/agreement-only. Do not copy Route
4 validation status into executable prepared authority, and do not change
prepared block-entry publication availability, value homes, destination storage,
move bundles, freshness, stack-source authority, prepared MIR, object, or
runtime behavior.

Preserve fail-closed outcomes for missing BIR successor/value evidence, wrong
destination type, wrong successor, duplicate Route 4 records, stale owner
records, and BIR-only PHI identity that lacks prepared move publication
readiness. The prepared-printer path calls this same prepared lookup through
`find_agreeing_route4_block_entry_publication`, so leave printer vocabulary and
dump rows unchanged until a later packet explicitly migrates that consumer.

## Proof

No build or ctest was run for this mapping-only packet, per delegated proof.
Future implementation proof command:
`cmake --build --preset default && ctest --test-dir build -R '^backend_prealloc_block_entry_publications$' --output-on-failure`.

No `test_after.log` was produced because this packet is read-only mapping plus
`todo.md` recording.
