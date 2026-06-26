# RV64 Object Route Prepared Local Memory Addressing

Status: Open
Type: Target object-route follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/closed/370_rv64_object_route_byval_aggregate_param_homes.md`

## Goal

Lower or precisely route RV64 object-route prepared local-memory accesses whose
base addressing is not yet a supported prepared frame-slot or pointer-value
base-plus-offset shape.

## Why This Exists

Closing idea 370 moved `src/20030914-2.c` past the byval aggregate
parameter-home boundary. The representative now stops later with:

```text
unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing
```

This is not a byval parameter-home gap. It is a local-memory addressing and
home-shape gap exposed after the byval home can be admitted from explicit
prepared ABI facts.

Known representative evidence:

- `src/20030914-2.c`: advances past `unsupported_byval_param_home` after commit
  `1bc45813`, then stops at the prepared local-memory addressing diagnostic
  above.

## In Scope

- Audit the prepared local-memory access metadata for the representative and
  focused RV64 object-emission fixtures.
- Define the first supportable local-memory addressing shape that can be
  materialized from explicit prepared frame-slot, pointer-value, offset, size,
  and alignment facts.
- Implement support only for fact-complete default-address-space accesses that
  do not require reconstructing source layout or unpublished aggregate bytes.
- Keep missing base facts, unsupported widths, non-default address spaces,
  dynamic offsets, ambiguous aggregate homes, and out-of-bounds accesses
  fail-closed with precise diagnostics.
- Add focused backend tests for supported local-memory addressing and adjacent
  rejected shapes.
- Rerun `src/20030914-2.c` and document whether it advances to aggregate
  `va_arg`, global/data work, or another distinct owner.

## Out of Scope

- Byval aggregate parameter-home admission; idea 370 closed that boundary.
- Non-register entry parameter homes; use
  `ideas/open/374_rv64_object_route_non_register_param_homes.md`.
- Aggregate `va_arg` helper lowering; use
  `ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md`.
- Global/data-section materialization unrelated to prepared local memory.
- Source-name handling for `src/20030914-2.c` or reconstruction from source
  syntax.

## Acceptance Criteria

- Focused backend tests prove the selected prepared local-memory addressing
  behavior or a narrower unsupported boundary.
- `src/20030914-2.c` is rerun and advances because of semantic prepared
  local-memory handling, or the next out-of-scope boundary is documented.
- Existing RV64 object-emission, prepared frame-stack call contract, and
  prepared-printer coverage remains green.
- Progress consumes prepared frame-slot, pointer-value, offset, size, and align
  facts instead of testcase-specific matching.

## Reviewer Reject Signals

- Reject named-case-only handling for `src/20030914-2.c`.
- Reject deriving local-memory bases or aggregate byte ranges from source
  syntax, assumed stack layout, physical registers, raw object offsets, or log
  text not published by prepared metadata.
- Reject expectation downgrades, skip broadening, allowlist filtering, or
  diagnostic-only renames claimed as local-memory addressing progress.
- Reject reintroducing the exact old
  `unsupported_local_memory_access: RV64 object route requires prepared
  frame-slot or pointer-value base-plus-offset local memory addressing`
  boundary behind a new helper name.
- Reject broad global/data-section, aggregate `va_arg`, or parameter-home
  rewrites unless the prepared local-memory addressing contract directly
  requires the change.
