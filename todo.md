Status: Active
Source Idea Path: ideas/open/205_route6_call_use_source_adapter.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Wire The Selected Reader To The Adapter

# Current Packet

## Just Finished

Step 3 verified that no additional implementation wiring is needed for the
selected AArch64 scalar call argument source-producer boundary.

`lower_scalar_call_argument_producers(...)` already builds the Route 6 call-use
source index for the selected `CallInst`, finds the prepared argument plan for
the argument index, and passes both into
`materialize_scalar_call_argument_value(...)`.

`materialize_scalar_call_argument_value(...)` already routes the selected
top-level call argument through
`find_scalar_call_argument_source_producer_materialization(...)`, which accepts
only validated Route 6 semantic source identity and otherwise falls back to the
prepared source-producer lookup. Recursive producer operand materialization
passes no selected call/argument key, so Route 6 does not broaden beyond the
selected call argument source role.

Prepared ABI placement, wrappers, clobbers, outgoing stack sizing, byval lanes,
variadic FPR counts, helper protocols, homes, move bundles, aggregate
transport, publication policy, final call records, and output authority remain
unchanged. No code or test churn was needed.

## Suggested Next

Execute Step 4 by proving adapter completeness and route quality for the
selected Route 6 scalar call argument source-producer boundary.

## Watchouts

- The adapter intentionally requires a prepared argument source id before
  accepting Route 6, so Route 6 remains semantic identity gated by prepared
  call-plan authority.
- Route 6 nonmaterializable binary producers now fall back to prepared lookup;
  with no prepared producer lookup, the selected route emits no scalar producer
  materialization.
- Step 4 should assess whether the existing success, absent fallback,
  source-id mismatch, duplicate, ABI-bound, nonmaterializable, prepared
  fallback, and unchanged-output coverage is enough for route-quality proof.
- Do not broaden into result lanes, direct-global dependency routing, whole
  call plans, helper protocol changes, expected-output rewrites, or
  unsupported-test downgrades.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Passed: 180 backend tests.

Proof log: `test_after.log`.
