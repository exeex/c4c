Status: Active
Source Idea Path: ideas/open/09_calls_preservation_republication_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Prepare Preservation Source And Destination Facts

# Current Packet

## Just Finished

Step 2 - Prepare Preservation Source And Destination Facts completed the
preparation-side preservation endpoint repair.

Repair: `src/backend/prealloc/call_plans.cpp` now builds callee-saved
preservation source endpoints from the live value home when one is published,
while keeping the preservation destination endpoint on the callee-saved storage
record. For the AArch64 recursive-formal shape that previously produced a
self-alias source/destination pair, the prepared record now names the live
pre-call source register (`x0`) separately from the callee-saved destination
register (`x20` or whichever callee-saved register regalloc selected).

Focused coverage now asserts both sides of the contract:
`backend_prepare_frame_stack_call_contract_test` has an AArch64 recursive
formal fixture that checks `preservation_source=register:x0` and a distinct
callee-saved `preservation_destination`; `backend_call_boundary_effect_plan_test`
checks that preservation home-population effects carry the distinct live source
register through the call-boundary effect plan.

Changed files:

- `src/backend/prealloc/call_plans.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `tests/backend/mir/backend_call_boundary_effect_plan_test.cpp`
- `todo.md`

## Suggested Next

Next packet can move to Step 3 prepared post-call republication facts, or first
retire the now-redundant AArch64 self-alias fallback if the supervisor wants a
cleanup slice after this preparation-side fix lands.

## Watchouts

AArch64 still has compatibility fallback paths through `find_value_home` and
legacy `PreparedCallPreservedValue` stack fields. The existing self-alias
fallback remains as compatibility debt, but normal prepared call-plan output for
the AArch64 formal-recursion regression no longer relies on it. Stack-slot
preservation home population into memory still flows through existing explicit
stack move lowering because
`CallBoundaryMoveInstructionRecord` has no destination-memory field.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_call_boundary_effect_plan|backend_prepared_lookup_helper|backend_prepared_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00040_c|c_testsuite_aarch64_backend_src_00168_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c)$'`

Result: passed, including all four AArch64 c_testsuite regressions. Proof log:
`test_after.log`.
