Status: Active
Source Idea Path: ideas/open/114_prepared_outgoing_stack_argument_area_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Print The Call-Level Area

# Current Packet

## Just Finished

Step 3 completed the prepared lookup/classification preservation surface for
the call-level outgoing stack argument area. `src/backend/prealloc/prepared_lookups.hpp`
and `src/backend/prealloc/prepared_lookups.cpp` now index
`PreparedCallPlan::outgoing_stack_argument_area` by call position and expose it
through `find_indexed_prepared_outgoing_stack_argument_area()` without
recomputing from individual stack argument lanes.

`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` asserts that the
indexed area preserves the original call-level prepared fact, remains absent for
calls without the optional area, and differs from a narrower per-argument stack
lane. `tests/backend/bir/backend_prealloc_call_boundary_classification_test.cpp`
asserts that call-boundary argument classification keeps the prepared
`call_plan` authority that exposes the call-level area, again distinct from the
individual stack lane size. No AArch64 scratch register, stack-adjustment,
instruction-ordering, or printer policy was added.

## Suggested Next

Implement Step 4 by printing the call-level outgoing stack argument area in the
prepared printer surfaces, without changing AArch64 codegen or consuming the
area for target instruction ordering in that packet.

## Watchouts

`plan_prepared_aggregate_transport()` and boundary-effect endpoints still keep
their per-argument destination offset and size fields. AArch64's
`outgoing_stack_argument_bytes()` remains the consumer-side inference to replace
later. Step 3 deliberately did not touch AArch64 codegen, prepared printers, or
Step 2 call-plan construction files.

## Proof

Step 3 proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_prealloc_call_boundary_classification)$' > test_after.log`

`test_after.log` records `backend_prepared_lookup_helper` and
`backend_prealloc_call_boundary_classification` passing 2/2.

Supervisor-side broader backend subset also passed:
`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Reported result: 179/179 backend tests passing.
