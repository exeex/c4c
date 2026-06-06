Status: Active
Source Idea Path: ideas/open/114_prepared_outgoing_stack_argument_area_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The Shared Prepared Area Contract

# Current Packet

## Just Finished

Step 1 completed the analysis-only inspection of the prepared outgoing stack
argument shape. Today `PreparedCallArgumentPlan` carries per-argument
`destination_stack_offset_bytes` and `destination_stack_size_bytes`; the size is
computed in `prepared_call_stack_argument_size_bytes()` and assigned through
`plan_call_argument_destination()` before each argument is appended in
`populate_call_plans()`. `PreparedCallPlan` has no call-level outgoing stack
area field, so shared prealloc loses the total area after publishing individual
argument destinations.

The only call-level total is currently inferred in AArch64 by
`outgoing_stack_argument_bytes(const PreparedCallPlan&)`, which scans all
arguments for `destination_stack_offset_bytes + destination_stack_size_bytes`
and aligns the maximum to the stack-pointer alignment. That inferred value is
used for AArch64 stack reservation/restoration and for adjusting frame-slot
sources after reservation. Prepared lookups index calls and selected arguments
but do not expose the area. The prepared printer emits only per-argument
`dest_stack_offset` / `dest_stack_size` and aggregate transport destination
fields, so the call-level extent is not printed.

## Suggested Next

Implement Step 2 by adding the smallest shared target-neutral area contract in
`src/backend/prealloc/calls.hpp` and `src/backend/prealloc/call_plans.cpp`:
add a `PreparedOutgoingStackArgumentArea`-style struct with a total byte extent
such as `size_bytes`, store it as
`std::optional<PreparedOutgoingStackArgumentArea> outgoing_stack_argument_area`
on `PreparedCallPlan`, and compute it in `populate_call_plans()` after all
arguments are built from the maximum of every complete
`destination_stack_offset_bytes + destination_stack_size_bytes`. Keep absence as
`std::nullopt`; do not encode AArch64 `x16`, stack pointer adjustment,
restoration, or instruction ordering in the shared struct.

First test change should extend
`tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp` with a
multi-stack-argument call-plan fixture that fails if the shared area is absent
or if the implementation only uses the first stack destination. Existing nearby
checks to reuse are `check_aarch64_outgoing_stack_scalar_argument_lifetime_contract()`
and the stack-argument call-plan helpers in that file.

## Watchouts

`plan_prepared_aggregate_transport()` and boundary-effect endpoints should keep
their per-argument destination offset and size fields; Step 2 only publishes the
call-level area. `find_call_boundary_argument_plan()` currently matches
stack-slot arguments using the per-argument destination offset and should not be
rewired in this packet. AArch64's `outgoing_stack_argument_bytes()` is the
consumer-side inference to replace later, not in Step 2.

## Proof

Step 1 was analysis-only; no build or test was run, per packet instruction.

Recommended Step 2 proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_frame_stack_call_contract$'`
