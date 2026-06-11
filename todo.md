Status: Active
Source Idea Path: ideas/open/174_route_index_facade_contraction.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Contract Only Redundant Facade Or Prepared Exposure

# Current Packet

## Just Finished

Step 3 migrated exactly `mir::find_bir_block_entry_publication_identity` to
cross the Route 4 block-entry facade boundary through
`route_index_reference_facade` and
`route_index_validate_block_entry_publication_reference`.

The consumer now builds the typed `Route4PublicationAvailabilityIndex`, asks
the facade validator for a typed block-entry publication reference, and reads
semantic publication facts only from the validated
`Route4BlockEntryPublicationRecord` still owned by the Route 4 index. The old
consumer-local `block_entry_records` scan and direct
`route4_find_block_entry_publication` fallback were removed.

## Suggested Next

Delegate Step 4 to inspect remaining exposure around the migrated Route 4
block-entry publication path and contract only the surface that is genuinely
redundant after this consumer migration.

## Watchouts

- Step 3 intentionally touched only
  `mir::find_bir_block_entry_publication_identity`; nearby current-block,
  edge-publication, Route 7, prepared helper, and AArch64 paths remain on their
  existing routes.
- Keep `Route4PublicationAvailabilityIndex` as the semantic owner; do not copy
  publication payload into `RouteIndexRecordReference` or the facade.
- Step 4 should inspect remaining users before deleting or privatizing any
  direct helper. Do not contract a surface that other consumers still require.
- The migrated consumer no longer supports the old untyped name-only
  `block_entry_records` scan; it relies on typed facade validation.

## Proof

Proof passed for Step 3:
`(cmake --build build --target backend_prepared_lookup_helper_test backend_prealloc_block_entry_publications_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' --output-on-failure) > test_after.log 2>&1`.
Canonical proof log: `test_after.log`.
