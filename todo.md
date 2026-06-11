Status: Active
Source Idea Path: ideas/open/205_route6_call_use_source_adapter.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The Fail-Closed Route 6 Reader Adapter

# Current Packet

## Just Finished

Step 2 added the fail-closed Route 6 reader adapter around the selected
AArch64 scalar call argument source-producer materialization path.

`find_scalar_call_argument_source_producer_materialization(...)` now accepts
Route 6 only when the selected call/argument key is unique, the Route 6 source
value id/name/value agrees with the prepared `PreparedCallArgumentPlan`, the
producer instruction points back into the current BIR block, and Route 6
reports scalar materialization available. Absent Route 6, source-id mismatch,
duplicate/ambiguous records, ABI-bound records, missing producers, stale
producer pointers, and nonmaterializable producers fall back to the existing
prepared source-producer lookup.

`materialize_scalar_call_argument_value(...)` now receives the prepared
argument plan so the Route 6 adapter can validate source identity while
prepared ABI/layout/policy facts remain authoritative.

Focused AArch64 coverage was added to the existing scalar call argument
producer fixture for Route-6-only success, source-id mismatch fallback, absent
Route 6 fallback, duplicate fallback, ABI-bound fallback, Route-6-only
rejection for those bad facts, and nonmaterializable producer rejection.

## Suggested Next

Execute Step 3 by reviewing/wiring the selected reader boundary now that the
adapter is in place. Confirm the selected reader consumes only validated Route
6 semantic identity and otherwise uses prepared fallback, with no migration of
ABI placement, wrappers, clobbers, outgoing stack sizing, helper protocols,
homes, move bundles, aggregate transport, publication policy, final call
records, or output authority.

## Watchouts

- The adapter intentionally requires a prepared argument source id before
  accepting Route 6, so Route 6 remains semantic identity gated by prepared
  call-plan authority.
- Route 6 nonmaterializable binary producers now fall back to prepared lookup;
  with no prepared producer lookup, the selected route emits no scalar producer
  materialization.
- Do not broaden the next packet into result lanes, direct-global dependency
  routing, whole call plans, helper protocol changes, expected-output rewrites,
  or unsupported-test downgrades.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Passed: 180 backend tests.

Proof log: `test_after.log`.
