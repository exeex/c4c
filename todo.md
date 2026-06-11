Status: Active
Source Idea Path: ideas/open/174_route_index_facade_contraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate One Consumer Boundary To The Facade

# Current Packet

## Just Finished

Step 2 tightened focused Route 4 block-entry publication facade validation
coverage in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
The selected typed validator still owns the semantic record through
`Route4PublicationAvailabilityIndex`; `RouteIndexRecordReference` remains a
reference-only facade record with no copied publication payload.

The block-entry facade now has positive and fail-closed coverage for missing
facade/index, missing block-entry record, missing destination value, stale
owner, wrong key, wrong relationship via the paired Route 4 current-block
reference, duplicate reference, and diverged typed record cases. A caller cannot
submit a wrong category to `route_index_validate_block_entry_publication_reference`,
so wrong-category coverage is not applicable to this selected entry point.

## Suggested Next

Delegate Step 3 to migrate the selected consumer boundary,
`mir::find_bir_block_entry_publication_identity`, to rely on the validated
Route 4 block-entry facade boundary while keeping `Route4PublicationAvailabilityIndex`
as the semantic owner and preserving the prepared/BIR identity checks as
oracles.

## Watchouts

- Keep the Step 3 migration to exactly one consumer:
  `mir::find_bir_block_entry_publication_identity`.
- Keep typed route records and indexes as the semantic owners; the facade
  should validate and return references, not copy semantic payloads.
- Do not turn `RouteIndexReferenceFacade` into a generic
  `PreparedFunctionLookups` replacement or scan broad BIR structures from the
  consumer.
- Wrong-category injection is not expressible through the selected block-entry
  facade API; do not add a generic reference validator only to manufacture that
  case.
- Do not include current-block publication migration, Route 7 comparison,
  AArch64 publication register selection, or prepared helper privatization in
  the Step 3 packet.

## Proof

Proof passed:
`(cmake --build build --target backend_prepared_lookup_helper_test backend_prealloc_block_entry_publications_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' --output-on-failure) > test_after.log 2>&1`.
Canonical proof log: `test_after.log`.
