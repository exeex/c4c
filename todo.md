Status: Active
Source Idea Path: ideas/open/09_calls_preservation_republication_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Prepare Preservation Source And Destination Facts

# Current Packet

## Just Finished

Step 2 - Prepare Preservation Source And Destination Facts completed.

Added explicit prepared preservation endpoints to
`PreparedCallPreservedValue`:

- `preservation_source` records the semantic value plus prepared source
  storage/register/stack facts available before the call.
- `preservation_destination` records the preservation home, including stack
  slot extent/spill placement or callee-saved register/save-index placement.
- `preservation_reason` distinguishes callee-saved register preservation,
  stack-slot preservation, and register-to-stack preservation for caller-saved
  clobber reuse.

`build_call_preserved_values` now prepares those facts while preserving the
existing route/home fields. `plan_prepared_call_boundary_effects` consumes the
prepared endpoints for preservation home population and keeps the existing
fallback synthesis for manual test fixtures that do not populate the new
fields.

Updated focused coverage:

- `backend_call_boundary_effect_plan_test` asserts prepared source and
  destination endpoints for callee-saved preservation and a caller-saved
  register source preserved into a stack slot.
- `backend_prepare_frame_stack_call_contract_test` asserts stack-preserved
  values publish explicit source/destination/reason facts in real prepared
  call plans.
- `backend_prepared_printer_test` asserts the prepared dump prints the new
  stack preservation endpoint/reason facts.

Changed files:

- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/prepared_printer/functions.cpp`
- `src/backend/prealloc/prepared_printer/runtime_helpers.cpp`
- `tests/backend/mir/backend_call_boundary_effect_plan_test.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

## Suggested Next

Next Step 2 or Step 3 packet: teach the AArch64 call-preservation lowering to
read preservation population sources and stack preservation destinations from
the prepared endpoints, then add a focused AArch64 dispatch test proving the
new endpoint consumption without removing the existing fallback paths yet.

## Watchouts

`src/backend/mir/aarch64/codegen/calls_moves.cpp` was intentionally left
untouched. AArch64 still has the existing fallback behavior; the new prepared
facts are now available for the next packet to consume and then retire those
fallbacks incrementally. The real stack-preservation fixture currently proves
stack-home endpoints; the caller-saved register-to-stack shape is asserted in
the prepared boundary-effect fixture.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_call_boundary_effect_plan|backend_prepared_lookup_helper|backend_prepared_printer)$'`

Result: passed. Proof log: `test_after.log`.
