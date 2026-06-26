Status: Active
Source Idea Path: ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Lower Supported Aggregate va_arg Helper Shape

# Current Packet

## Just Finished

Step 3 lowered the first supported RV64 object-route overflow-area aggregate
`va_arg` helper shape by consuming the prepared `payload_write_address`
directly.

- Added fail-closed RV64 aggregate `va_arg` helper diagnostics for the supported
  one-field overflow-area va_list shape, prepared GPR source va_list, prepared
  scratch resources, supported copy size/alignment, and frame-slot
  `payload_write_address` coverage.
- Emitted object code that loads the current overflow pointer from the prepared
  va_list field, copies the prepared byte range into the prepared frame-slot
  payload address in 8/4/2/1-byte chunks, advances the overflow pointer by the
  prepared stride, and stores it back to the overflow-area field.
- Added focused RV64 object-emission coverage for a 9-byte aggregate payload
  copy and kept the missing-`payload_write_address` diagnostic fail-closed.
- Updated the representative RV64 aggregate-overflow helper object fixture to
  record the new boundary after helper lowering:
  `unsupported_param_home: RV64 object route requires all parameters in
  supported GPR or prepared FPR register homes`.

## Suggested Next

Proceed with the next supervisor-selected packet after the new boundary, likely
RV64 object-route support for the prepared parameter home shape exposed by
`riscv64_variadic_aggregate_overflow_helper_contract.c`.

## Watchouts

- The supported helper shape is intentionally narrow: overflow_arg_area source
  class, one-field 8-byte overflow va_list field, frame-slot
  `payload_write_address`, prepared source va_list GPR, and small explicit copy
  sizes encodable by the current load/store chunk loop.
- The helper path still does not infer payload addresses from BIR names, call
  argument side channels, testcase names, or `destination_payload_home`.
- Broader end-to-end RV64 object acceptance for the representative C fixture is
  still blocked by `unsupported_param_home`, not by aggregate helper lowering.

## Proof

Delegated proof passed and was written to `test_after.log`:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- CTest reported `100% tests passed, 0 tests failed out of 326`.
