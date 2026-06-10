# Current Packet

Status: Complete
Source Idea Path: ideas/open/157_bir_call_boundary_source_facts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove prepared-oracle equivalence for result source facts

## Just Finished

Step 4 added a BIR-owned call-result source identity/query surface for
target-neutral result identity only. BIR now exposes primary result value
identity and result-lane value identity with call-instruction boundary checks;
it does not expose result register names, destination homes, ABI bindings,
late-publication alias booleans, stack slots, or move records.

`backend_prepared_lookup_helper_test.cpp` compares the BIR result identity
queries against `PreparedCallResultPlan::destination_value_id` and
`PreparedAfterCallResultLaneBinding` semantic identity facts. The tests cover
primary result identity, lane identity, lane-0 primary-result aliasing,
duplicate lane fail-closed behavior, ABI-only result calls, and detached or
wrong-index call boundaries. `backend_prealloc_call_boundary_classification_test.cpp`
keeps the prepared result ABI classification available while proving the new
BIR query does not reconstruct identity from result ABI placement.

## Suggested Next

No remaining Step 4 gaps are known inside this packet. The next coherent packet
is supervisor-owned: accept/commit this BIR result identity proof slice or
choose the next source-fact surface while keeping AArch64/prealloc consumers on
prepared authority.

## Watchouts

- The result identity surface intentionally reports only `Value` identity,
  call instruction index, lane index, and whether a lane aliases the primary
  result. It must not be used as result ABI placement authority.
- Lane lookup is by named value identity and fails closed when the same lane
  value appears more than once, except for the canonical lane-0 alias where
  `result_lanes[0]` repeats the primary call result.
- No AArch64 or prealloc consumers were switched in this packet.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Passed. `test_after.log` contains the successful build and backend test run:
179 backend tests passed, 0 failed.
