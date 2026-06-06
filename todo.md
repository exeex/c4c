Status: Active
Source Idea Path: ideas/open/114_prepared_outgoing_stack_argument_area_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Consume The Shared Area In AArch64

# Current Packet

## Just Finished

Step 4 completed the prepared printer surface for the call-level outgoing stack
argument area. `src/backend/prealloc/prepared_printer/calls.cpp` now prints
`PreparedCallPlan::outgoing_stack_argument_area` on the call-plan line when the
prepared fact is present.

`tests/backend/bir/backend_prepared_printer_test.cpp` now seeds a manual call
plan with `outgoing_stack_argument_area=32` and a distinct per-argument stack
lane of `dest_stack_offset=8 dest_stack_size=8`, so the dump contract fails if
reviewers have to reconstruct the total area from individual arguments. No
target-specific scratch register, stack adjustment, instruction ordering, or
AArch64 codegen behavior was added.

## Suggested Next

Implement Step 5 by consuming the shared prepared outgoing stack argument area
in AArch64 instead of inferring the outgoing stack adjustment from individual
argument destinations.

## Watchouts

The prepared printer now makes the call-level area visible as
`outgoing_stack_argument_area=<bytes>`, while per-argument
`dest_stack_offset`/`dest_stack_size` fields remain lane facts. AArch64's
`outgoing_stack_argument_bytes()` remains the consumer-side inference to
replace next; the Step 4 slice did not touch AArch64 codegen, prepared
lookup/classification files, or call-plan construction files.

## Proof

Step 4 proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_printer$' > test_after.log`

`test_after.log` records `backend_prepared_printer` passing 1/1.

Supervisor-side broader backend subset also passed:
`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Reported result: 179/179 backend tests passing.
