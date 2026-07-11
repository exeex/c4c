Status: Active
Source Idea Path: ideas/open/698_bir_call_return_route_view_extraction.md
Source Plan Path: plan.md
Current Step ID: 3-4
Current Step Title: Add The Named Call-Boundary View And Migrate One Proof Reader

# Current Packet

## Just Finished

Completed `plan.md` Step 3 and Step 4 for the selected call-result identity
surface. Added `BirCallBoundaryView` in `src/backend/bir/bir.hpp` with
`make_bir_call_boundary_view()`, `find_call_result_source()`, and
`find_call_result_lane_source()` implemented in
`src/backend/bir/bir_route6_call_publication.cpp` over the existing Route 6
call-use source index.

Migrated exactly one proof reader:
`verify_bir_call_result_source_identity_lookup()` in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`. The reader now
uses named call-boundary vocabulary for indexed primary result identity,
result-lane identity, missing-result, wrong-call, missing-call, duplicate-lane,
and no-match behavior. The old `Route6CallUseSourceIndex` and
`route6_*call_result*` helpers remain as compatibility implementation/rollback
surface.

## Suggested Next

Select the next call-boundary proof surface, if any, and decide whether it
needs another named view adapter or can reuse `BirCallBoundaryView`.

## Watchouts

No target ABI lowering, executable call/return lowering, MIR lowering, stack
destination authority, publication/memory cleanup, expectations, unsupported
markers, allowlists, or timeouts were changed.

Route 8 is still deferred/not needed for this reader: the migrated proof uses
only Route 6 call-result identity and result-lane identity at the BIR
call-boundary surface.

## Proof

Ran the delegated proof command:
`bash -lc "(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log"`.

Result: passed. `ctest` reported 304/304 backend tests passing. Proof log:
`test_after.log`.
