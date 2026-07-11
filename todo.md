Status: Active
Source Idea Path: ideas/open/697_bir_memory_publication_view_extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map The First Publication View Surface

# Current Packet

## Just Finished

Completed `plan.md` Step 1 discovery for the first named BIR publication-view
surface.

Selected first surface:
`tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
`check_call_argument_source_producer_materializability_contract`, specifically
the focused Route 4 current-block publication reference reader for `%sum`.
This surface proves a valid current-block publication plus missing,
wrong-type, duplicate, stale-owner, and wrong-relationship negative states.

Old Route 4 entry points:
`bir::route4_build_publication_availability_index(route4_function)` followed by
`bir::route4_validate_current_block_publication_reference(route4_index,
route4_block, route4_sum_value, 3)`, with the same validation entry point used
for `%missing`, wrong-type `%sum`, duplicate-index, stale-block, and
wrong-relationship checks.

Intended named adapter:
add a `BirPublicationView`-style Route 4 compatibility surface in BIR, for
example `bir::make_bir_publication_view(function)` plus
`bir::validate_current_block_publication_reference(view, block, value,
before_instruction_index)`, with exact naming left to the implementation
packet. The adapter should return the same `Route4IndexReferenceValidation`
payload during migration: validity, `RouteIndexValidationStatus`,
`Route4PublicationAvailabilityStatus`, current-block record pointer, reference
category/relationship, and source producer instruction index.

Route 5 decision:
defer Route 5 for the first migration. The selected proof reader is Route
4-only and does not need CFG-edge, current-block join-source, or agreement
residue. Route 5 status/agreement rows should remain diagnostic compatibility
until a later packet explicitly scopes one named publication proof surface.

Rollback point:
restore the local `route4_index` construction and direct
`route4_validate_current_block_publication_reference` calls inside
`check_call_argument_source_producer_materializability_contract`. No production
behavior, prepared authority, expectation, unsupported marker, allowlist,
timeout, runtime contract, default harness contract, or root proof log rollback
should be needed.

## Suggested Next

Implement the named Route 4-only `BirPublicationView` compatibility adapter in
`src/backend/bir/bir.hpp` plus `src/backend/bir/bir.cpp` or
`src/backend/bir/bir_route4_publication.cpp`, then migrate only the selected
`check_call_argument_source_producer_materializability_contract` current-block
Route 4 proof reader through that adapter. Keep Route 5 deferred.

## Watchouts

Keep the new surface named around BIR publication-view vocabulary, not Route 4
or route-index vocabulary. Do not expose `Route4PublicationAvailabilityIndex`
or Route 5 index/status/agreement rows as new public architecture under renamed
wrappers. Do not migrate block-entry publication readers, production
MIR/prealloc/codegen readers, Route 5 readers, memory views, prepared
authority, stack/frame/value-home authority, freshness, move-bundle authority,
MIR lowering, target materialization, Route 7 comparison proof, call/return
views, dump vocabulary, expectations, unsupported markers, allowlists,
timeouts, runtime contracts, default harness contracts, or root-level proof
logs in the next code packet.

## Proof

Discovery proof run:
`rg -n "Route4PublicationAvailabilityIndex|route4_build_publication_availability_index|route4_find_|route4_|Route5EdgeJoinSourceIndex|route5_" src/backend tests/backend`

No build was required for this discovery-only packet. Per the delegated proof
contract, `test_before.log` and `test_after.log` were not created or modified.

Recommended focused implementation proof:
`cmake --build --preset default --target backend_prepare_frame_stack_call_contract_test && build/tests/backend/bir/backend_prepare_frame_stack_call_contract_test`

CTest source registration exists as `backend_prepare_frame_stack_call_contract`
in `tests/backend/bir/CMakeLists.txt`; the current build tree did not list that
test via `ctest -N -R '^backend_prepare_frame_stack_call_contract$'`, so the
direct executable command above is the reliable focused proof for the next
packet.
