Status: Active
Source Idea Path: ideas/open/09_calls_preservation_republication_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Shrink AArch64 Preservation Helpers To Emission

# Current Packet

## Just Finished

Step 4 - Shrink AArch64 Preservation Helpers To Emission retired the remaining
avoidable preservation compatibility paths in the packet scope.

Repair: `src/backend/mir/aarch64/codegen/calls_moves.cpp` no longer lets
callee-saved preservation home population call `find_value_home` or synthesize a
source operand from raw value-home data when prepared endpoints are missing or
self-aliasing. The prior stack-preserved before-instruction reload helper now
requires and emits from the prepared `preservation_destination` endpoint instead
of falling back to legacy preserved slot fields.

Focused coverage now asserts the retired fallback:
`backend_aarch64_instruction_dispatch_test` changes the self-alias preservation
population fixture to prove AArch64 does not rederive an `x1 -> x20`
population from value homes when the prepared source and destination endpoints
alias.

Changed files:

- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `todo.md`

## Suggested Next

Next packet can move to Step 5 validation or reviewer/supervisor scrutiny for
route quality, with attention on any remaining non-packet AArch64 call paths
that still use generic value-home lookup for ordinary explicit moves.

## Watchouts

AArch64 callee-saved preservation population now requires a complete prepared
source endpoint; a self-aliasing or missing source endpoint emits no population
move rather than reconstructing from value homes. Stack-slot preservation home
population into memory still flows through existing explicit stack move lowering
because `CallBoundaryMoveInstructionRecord` has no destination-memory field.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_prepared_lookup_helper|backend_prepared_printer|backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00040_c|c_testsuite_aarch64_backend_src_00168_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c)$'`

Result: passed, 11/11 tests green including all four AArch64 c_testsuite
regressions. Proof log: `test_after.log`.
