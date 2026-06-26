# RV64 Object Route Byval Aggregate Parameter Homes

Status: Closed
Type: Target ABI follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Lower or precisely route RV64 byval aggregate parameter homes represented in
prepared stack slots.

## Why This Exists

Closed idea 363 intentionally narrowed the parameter-home boundary to a precise
diagnostic. Closed idea 367 then advanced `src/20030914-2.c` past helper-free
variadic entry admission. Idea 354's Step 3 representative refresh confirmed
the residual:

```text
unsupported_byval_param_home: RV64 object route does not yet lower byval aggregate parameter homes in prepared stack slots
```

Representative:

- `src/20030914-2.c`

This was a target ABI parameter-home coverage gap, not a scalar vararg admission
gap and not an opaque prepared-shape failure.

## In Scope

- Audit the prepared byval aggregate parameter-home metadata for the
  representative case and focused backend fixtures.
- Define which byval aggregate stack-slot homes can be materialized by the RV64
  object route without guessing ABI state.
- Implement support for the first safe class or keep a precise unsupported
  diagnostic when required facts are missing.
- Add focused tests for supported byval parameter-home movement and fail-closed
  unsupported classes.
- Rerun `src/20030914-2.c` and document its outcome.

## Out of Scope

- Reopening helper-free variadic entry admission from idea 367.
- Implementing scalar or aggregate `va_arg` helper lowering.
- Broad aggregate transport or global memory work unrelated to byval parameter
  homes.
- Matching the representative case by source name or by its exact function
  shape.

## Acceptance Criteria

- Focused backend tests prove the selected byval aggregate parameter-home
  behavior or a more precise unsupported boundary.
- `src/20030914-2.c` is rerun and advances because of real ABI/home handling or
  remains blocked by a newly documented out-of-scope boundary.
- Existing RV64 object-emission, prepared frame-stack call contract, and
  prepared-printer coverage remains green.
- Progress is based on prepared ABI homes, not testcase names.

## Reviewer Reject Signals

- Reject named-case-only handling for `src/20030914-2.c`.
- Reject copying aggregate bytes from assumed stack layouts not published by
  the prepared parameter-home contract.
- Reject expectation downgrades, skip broadening, or allowlist filtering
  claimed as parameter-home progress.
- Reject moving `va_arg` or unrelated variadic helper work into this idea
  unless the byval home contract directly requires it.
- Reject diagnostic-only renames that retain the same unsupported byval
  parameter-home boundary.

## Closure Notes

Closed after commit `1bc45813` admitted the first safe RV64 object-route byval
aggregate parameter-home class: a default-address-space byval formal whose
prepared value home is a stack slot backed by a matching permanent
`source_kind=byval_param` frame object. The repair consumes explicit prepared
ABI/home, frame-slot, size, align, and in-bounds access facts rather than source
names or assumed stack layout.

`src/20030914-2.c` now advances past `unsupported_byval_param_home` and exposes
a distinct next boundary:

```text
unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing
```

That local-memory addressing shape is outside this idea's byval parameter-home
scope and is handed off to
`ideas/open/382_rv64_object_route_prepared_local_memory_addressing.md`.

Close-time regression guard used the existing focused byval CTest logs:

- `test_before.log`: passed=9 failed=2 total=11
- `test_after.log`: passed=9 failed=2 total=11
- no newly failing tests
- remaining failures are the pre-existing
  `backend_codegen_route_riscv64_byval_preserved_pointer_args` and
  `backend_rv64_runtime_riscv64_byval_preserved_pointer_args`

The broader post-patch backend run reported only those same two pre-existing
preserved-pointer failures. The hook-produced `test_baseline.new.log` was not
accepted as full-suite baseline state because it is not monotonic against the
accepted `test_baseline.log`.
