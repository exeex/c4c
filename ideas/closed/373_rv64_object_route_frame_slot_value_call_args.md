# RV64 Object Route Frame-Slot Value Call Arguments

Status: Closed
Type: Repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md`

## Completion Note

Closed after the RV64 object route admitted scalar frame-slot-value call
arguments from prepared frame-plan, value-home, frame-slot, and call-selection
facts. Focused object-emission tests cover the supported scalar path and
fail-closed neighbors, the focused backend proof stayed green at `3/3`, and
`src/20000121-1.c` now passes the one-case RV64 gcc torture backend runner
with `total=1 passed=1 failed=0`.

## Goal

Lower prepared call arguments whose scalar payload value currently lives in a
frame slot or spill home in the RV64 object route.

## Why This Exists

Idea 368's pointer-value local-memory repair moved `src/20000121-1.c` off the
previous `unsupported_local_memory_access` diagnostic. The next visible
boundary is not another local-memory access; it is call-argument publication
for a scalar value that must be recovered from a frame-slot home before the
call.

Known representative evidence:

- `src/20000121-1.c`: `doit` no longer stops on the pointer-value i8 load from
  `%p.id`; the residual is frame-slot-value call-argument publication for the
  spilled `%t1` value.

## In Scope

- Audit prepared call plans, value homes, and frame-slot metadata for scalar
  call arguments sourced from frame-slot values.
- Define the first supportable RV64 form for loading a scalar frame-slot value
  into the ABI-selected argument GPR.
- Implement object-emission support from prepared frame-slot/value-home facts,
  not source syntax.
- Keep missing slots, unsupported widths, bad alignments, non-GPR
  destinations, out-of-range offsets, aggregates, and volatile or non-default
  address-space forms fail-closed with precise diagnostics.
- Add focused backend tests for admitted and rejected frame-slot-value call
  arguments.
- Rerun `src/20000121-1.c` and record whether it passes or advances to another
  owner.

## Out of Scope

- Frame-slot address call-argument materialization; that belongs to
  `ideas/open/372_rv64_object_route_frame_slot_address_call_args.md`.
- Pointer-value local-memory loads/stores; idea 368 closed that route.
- Aggregate `va_arg` helper lowering.
- Byval aggregate parameter homes.
- General terminator lowering or source-shape shortcuts.

## Acceptance Criteria

- Focused RV64 object-emission tests prove scalar frame-slot-value call
  arguments and fail-closed unsupported neighbors.
- `src/20000121-1.c` is rerun and advances because of semantic call-argument
  publication repair, or the next out-of-scope boundary is documented.
- Existing focused backend object-emission and prepared-contract coverage
  remains green.
- Progress is based on prepared value-home and call-plan metadata.

## Reviewer Reject Signals

- Reject testcase-name shortcuts for `src/20000121-1.c` or its exact function
  shape.
- Reject hard-coded frame-slot ids, offsets, value names, or argument registers
  that are not derived from prepared metadata.
- Reject changes that pass an address when a payload value is required, or
  silently ignore ABI call-argument placement facts.
- Reject expectation downgrades, skip broadening, allowlist filtering, or
  diagnostic-only renames claimed as capability progress.
- Reject broad changes to frame-slot address call arguments, aggregate
  `va_arg`, byval homes, terminators, or unrelated local-memory behavior inside
  this idea.
