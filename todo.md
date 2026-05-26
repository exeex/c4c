Status: Active
Source Idea Path: ideas/open/09_calls_preservation_republication_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prepare Post-Call Republication Facts

# Current Packet

## Just Finished

Step 3 - Prepare Post-Call Republication Facts completed the prepared
republication handoff for preserved values.

Repair: `src/backend/prealloc/call_plans.cpp` now makes preservation
republication effects carry the prepared post-call value home as the effect
destination, using the preserved value's prepared source/home endpoint instead
of a value-id-only placeholder. `src/backend/mir/aarch64/codegen/calls_moves.cpp`
now lowers callee-saved republication from the prepared source endpoint into
that prepared destination endpoint and no longer chooses the post-call
destination register with `find_value_home`.

Focused coverage now asserts the prepared destination fact and the AArch64
consumer behavior: `backend_call_boundary_effect_plan_test` checks
republication destination homes, `backend_aarch64_call_boundary_owner_test`
makes value-home lookup data intentionally disagree with the prepared endpoint,
and `backend_aarch64_instruction_dispatch_test` keeps same-block later
call-argument reuse flowing through explicit prepared preservation endpoints.

Changed files:

- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/prealloc/call_plans.cpp`
- `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_call_boundary_effect_plan_test.cpp`
- `todo.md`

## Suggested Next

Next packet can move to Step 4 and shrink AArch64 preservation helpers toward
emission-only code by retiring compatibility paths that still reconstruct
preservation facts outside prepared endpoints.

## Watchouts

AArch64 callee-saved republication now requires the prepared republication
destination endpoint to be a register endpoint; manually constructed call-plan
fixtures need to include the same prepared endpoints that real call planning
publishes. Population still has a self-alias compatibility fallback, and
stack-slot preservation home population into memory still flows through
existing explicit stack move lowering because `CallBoundaryMoveInstructionRecord`
has no destination-memory field.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_prepared_lookup_helper|backend_prepared_printer|backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00040_c|c_testsuite_aarch64_backend_src_00168_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c)$'`

Result: passed, 11/11 tests green including all four AArch64 c_testsuite
regressions. Proof log: `test_after.log`.
