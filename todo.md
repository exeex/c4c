Status: Active
Source Idea Path: ideas/open/114_prepared_outgoing_stack_argument_area_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Preserve The Area Through Lookups And Classification

# Current Packet

## Just Finished

Step 2 completed the shared prepared outgoing stack argument area contract.
`src/backend/prealloc/calls.hpp` now publishes
`PreparedOutgoingStackArgumentArea` and optional
`PreparedCallPlan::outgoing_stack_argument_area`; `src/backend/prealloc/call_plans.cpp`
computes that optional area from the maximum of every complete per-argument
`destination_stack_offset_bytes + destination_stack_size_bytes` and leaves it
absent when no nonzero complete stack destination exists. No AArch64 scratch
register, stack-adjustment, or instruction-ordering policy was added to shared
prealloc.

`tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp` now
extends the outgoing-stack scalar fixture to two stack destinations and asserts
that the shared area exists with `size_bytes == 16`, so the test fails if the
area is absent or if only the first stack destination contributes.

## Suggested Next

Implement Step 3 by preserving the new optional outgoing stack argument area
through the prepared lookup/classification surfaces that need to reason about
call-boundary stack authority, without changing AArch64 codegen or prepared
printer behavior in that packet.

## Watchouts

`plan_prepared_aggregate_transport()` and boundary-effect endpoints still keep
their per-argument destination offset and size fields. AArch64's
`outgoing_stack_argument_bytes()` remains the consumer-side inference to replace
later; Step 2 deliberately did not touch AArch64 codegen, prepared printers,
prepared lookup/classification files, or unrelated tests.

## Proof

Step 2 proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_frame_stack_call_contract$' > test_after.log`

`test_after.log` records `backend_prepare_frame_stack_call_contract` passing
1/1.

Supervisor-side broader backend subset also passed:
`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Reported result: 179/179 backend tests passing.
