Status: Active
Source Idea Path: ideas/open/698_bir_call_return_route_view_extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map The First Call-Boundary Surface

# Current Packet

## Just Finished

Completed `plan.md` Step 1 mapping for the first Route 6
call-boundary surface. Selected
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` reader
`verify_bir_call_result_source_identity_lookup()` as the first low-risk proof
surface because it already proves BIR call-result value and result-lane
identity without requiring ABI placement, target lowering, or executable
call/return behavior.

Step 1 decision:
- Selected proof reader: `verify_bir_call_result_source_identity_lookup()` in
  CTest `backend_prepared_lookup_helper`.
- Old Route 6 entry point: `route6_build_call_use_source_index()` plus
  `route6_find_call_result_source()` and
  `route6_find_call_result_lane_source()`; direct rollback helpers are
  `route6_call_result_source_record()` and
  `route6_call_result_lane_source_record()`.
- Intended named call-boundary view entry point:
  `BirCallBoundaryView`-style reader over the existing Route 6 index, with
  call-result lookup methods for primary result identity and result-lane
  identity. The view should preserve the same missing-result, wrong-call,
  missing-call, duplicate-lane, and no-match statuses.
- Rollback point: keep `Route6CallUseSourceIndex` and the existing
  `route6_*call_result*` helpers as the compatibility implementation while the
  selected reader migrates to named call-boundary vocabulary.
- Supervisor proof command for the implementation packet:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

## Suggested Next

Add the named `BirCallBoundaryView`-style adapter for the selected call-result
identity surface, delegating to the old Route 6 result source index and record
helpers without changing behavior.

## Watchouts

Keep the first migration result-identity only: primary call result and
result-lane source identity at the BIR call boundary. Do not pull in call
argument publication/source-producer materialization, target consumed-plans
helpers, ABI lowering, prepared authority, frame layout, value homes,
freshness, move bundles, stack destination authority, publication/memory
views, dump vocabulary, expectations, unsupported markers, allowlists,
timeouts, or runtime contracts.

Minimal Route 8 boundary observation: the selected Route 6 result identity
reader does not need Route 8. Route 8 currently indexes same-block scalar
return-chain identities through `route8_build_return_chain_index()` and
`route8_find_return_chain_record()` with fail-closed duplicate/no-match
behavior; leave any public return-chain view decision deferred unless a later
packet explicitly selects that surface.

## Proof

No build or ctest was required for this mapping-only packet. Read-only
inspection identified the future supervisor proof command as
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
No `test_after.log` was written for this no-proof packet.
