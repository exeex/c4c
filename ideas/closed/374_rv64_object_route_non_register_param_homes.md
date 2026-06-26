# RV64 Object Route Non-Register Parameter Homes

Status: Closed
Type: Target ABI follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md`

## Goal

Lower or precisely route RV64 object-route entry parameters whose prepared
homes are not currently supported GPR or prepared FPR register homes.

## Why This Exists

Idea 368's pointer-value local-memory repair moved `src/va-arg-13.c` off the
previous local-memory diagnostic. The current object-route stop occurs earlier
than its later frame-slot-address and aggregate `va_arg` boundaries:

```text
unsupported_param_home: RV64 object route requires all parameters in supported GPR or prepared FPR register homes
```

This is an entry-parameter home coverage gap, not another local-memory
load/store issue.

Known representative evidence:

- `src/va-arg-13.c`: currently stops at `unsupported_param_home` before later
  pointer-value local-memory, frame-slot-address call-argument, and aggregate
  `va_arg` owners can be observed in the object route.

## In Scope

- Audit prepared parameter-home metadata for non-register homes in the
  representative and focused backend fixtures.
- Define the first supportable RV64 object-route parameter-home form that can
  be consumed without guessing ABI state.
- Implement support only from explicit prepared parameter-home, frame, and ABI
  facts.
- Keep unsupported stack, aggregate, missing-home, non-default address-space,
  dynamic-layout, and ABI-ambiguous parameter homes fail-closed with precise
  diagnostics.
- Add focused backend tests for supported and rejected non-register parameter
  homes.
- Rerun `src/va-arg-13.c` and record whether it advances to frame-slot address
  call arguments, aggregate `va_arg`, or another owner.

## Out of Scope

- Byval aggregate parameter homes; use
  `ideas/open/370_rv64_object_route_byval_aggregate_param_homes.md`.
- Aggregate `va_arg` helper lowering; use
  `ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md`.
- Frame-slot address call-argument materialization; use
  `ideas/open/372_rv64_object_route_frame_slot_address_call_args.md`.
- Pointer-value local-memory loads/stores; idea 368 closed that route.
- Inferring parameter layout from source syntax or testcase names.

## Acceptance Criteria

- Focused backend tests prove the selected non-register parameter-home behavior
  or a more precise unsupported boundary.
- `src/va-arg-13.c` is rerun and advances because of semantic parameter-home
  handling, or the next out-of-scope boundary is documented.
- Existing RV64 object-emission, prepared frame-stack call contract, and
  prepared-printer coverage remains green.
- Progress consumes prepared ABI-home facts instead of adding testcase-specific
  lowering.

## Closure Note

Accepted after the scalar GPR formal stack-slot home slice. The object route
now admits scalar GPR formal stack-slot homes only when prepared value-home,
ABI integer-register ownership, frame-slot, stack-object, type, size/alignment,
final-frame-bound, and signed 12-bit stack-offset facts are coherent. Unsupported
or ambiguous parameter-home shapes remain fail-closed.

The representative `tests/c/external/gcc_torture/src/va-arg-13.c` no longer
stops at `unsupported_param_home`; it advances to a later
`unsupported_instruction_fragment` boundary. That boundary is not
parameter-home-specific and is tracked separately by
`ideas/open/386_rv64_object_route_unsupported_instruction_fragment.md`.

Close gate proof used the backend subset:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
`test_before.log` and `test_after.log` both reported 326 passing tests and no
failures; the regression guard passed with non-decreasing pass count for this
lifecycle-only close.

## Reviewer Reject Signals

- Reject named-case-only handling for `src/va-arg-13.c`.
- Reject reconstructing parameter placement from source syntax, assumed stack
  layouts, or ABI guesses not published by prepared metadata.
- Reject expectation downgrades, skip broadening, allowlist filtering, or
  diagnostic-only renames claimed as parameter-home progress.
- Reject moving byval aggregate homes, aggregate `va_arg`, or frame-slot
  address call arguments into this idea unless the parameter-home contract
  directly requires it.
- Reject broad entry/ABI rewrites that do not directly prove non-register
  parameter-home behavior and fail-closed unsupported shapes.
