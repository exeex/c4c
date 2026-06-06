Status: Active
Source Idea Path: ideas/open/114_prepared_outgoing_stack_argument_area_contract.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Acceptance Validation And Route Review

# Current Packet

## Just Finished

Step 5 completed AArch64 consumption of the shared prepared outgoing stack
argument area. `src/backend/mir/aarch64/codegen/calls.cpp` now derives
AArch64 outgoing stack reservation bytes from
`PreparedCallPlan::outgoing_stack_argument_area` instead of recomputing
authority from per-argument destination offsets and sizes.

`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` now gives the
F128 stack-argument dispatch fixture a prepared call-level area of 64 bytes
while its actual stack lane remains at offset 32 with size 16. The test checks
that AArch64 reserves/restores 64 bytes while still using x16 as the outgoing
area base and storing the lane at `[x16, #32]`. Existing manual stack-argument
fixtures that expected outgoing stack reservation now seed the shared prepared
area fact explicitly.

## Suggested Next

Run Step 6 acceptance validation and route review for the prepared outgoing
stack argument area contract.

## Watchouts

AArch64 still owns the target-specific x16 scratch base choice, stack-pointer
adjustment/restoration, source-offset adjustment after reservation, and store
ordering. Manual AArch64 tests with stack destinations must seed
`outgoing_stack_argument_area`; per-argument destination offsets and sizes are
lane facts, not reservation authority.

## Proof

Step 5 proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_instruction_dispatch$' > test_after.log`

`test_after.log` records `backend_aarch64_instruction_dispatch` passing 1/1.

Supervisor-side broader backend subset also passed:
`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Reported result: 179/179 backend tests passing.
