Status: Active
Source Idea Path: ideas/open/695_bir_route_facade_named_compatibility_adapters.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Behavior Preservation And Private Compatibility

# Current Packet

## Just Finished

Completed `plan.md` Step 2, Step 3, and Step 4 for the selected Route 4
prealloc consumer. Added `bir::validate_block_entry_publication_reference` as
a named publication adapter accepting
`const Route4PublicationAvailabilityIndex&`, `const Block&`, and
`const Value&`, returning the existing `Route4IndexReferenceValidation` by
delegating to `route4_validate_block_entry_publication_reference`.

Migrated only
`src/backend/prealloc/prepared_lookups.cpp`
`attribute_route4_block_entry_publication_if_agreeing` to call the named
adapter directly. The caller no longer constructs `RouteIndexReferenceFacade`;
route-numbered facade types and validation functions remain available for
compatibility.

## Suggested Next

After this slice is committed, request plan-owner lifecycle review. The active
runbook's first-consumer migration and preservation proof are complete; any
additional migration should be a follow-up lifecycle decision rather than
silent scope expansion.

## Watchouts

This packet intentionally did not migrate the separate
`src/backend/mir/query.cpp` Route 4 consumer or any Route 7 consumer. It also
did not change tests, expectations, allowlists, baseline files, runtime
behavior, default harness contracts, Route 5 publication cleanup, prepared
authority, target lowering, or stack authority.

## Proof

Implementation proof passed with the supervisor-selected command:
`set -o pipefail; (cmake --build --preset default && build/tests/backend/bir/backend_prealloc_block_entry_publications_test) > test_after.log 2>&1`

Broader backend before/after guard also passed with behavior-preserving counts:
`ctest --test-dir build -j --output-on-failure -R backend` reported
305 passed / 0 failed before and 305 passed / 0 failed after. The monotonic
checker passed with `--allow-non-decreasing-passed`; accepted log state was
rolled forward to `test_before.log`.
