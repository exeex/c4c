# RV64 Object Route Frame-Slot Base-Offset Local Memory

Status: Open
Type: Repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Lower prepared local memory accesses that require frame-slot base-plus-offset
addressing in the RV64 object route.

## Why This Exists

Idea 354's Step 3 representative refresh found residual structured object-route
failures after the original child ideas closed. Three representatives now stop
at:

```text
unsupported_local_memory_access: RV64 object route requires prepared frame-slot base-plus-offset local memory addressing
```

Representatives:

- `src/20000217-1.c`
- `src/20000121-1.c`
- `src/va-arg-13.c`

This is no longer an opaque prepared-module-shape failure. It is a target
object-emission memory-addressing gap whose shared diagnostic has covered both
direct frame-slot accesses and pointer-value local-memory accesses.

## Route Correction

After a focused implementation admitted direct frame-slot subobject offset
composition, the representative rerun still produced `total=3 passed=0
failed=3` with the same diagnostic. That focused slice is retained as useful
RV64 object-route capability, but it is not representative progress for this
source idea.

The active plan now treats pointer-value local-memory loads/stores as the next
in-scope owner for idea 368. Frame-slot address call-argument materialization
is split into `ideas/open/372_rv64_object_route_frame_slot_address_call_args.md`
and must not be silently absorbed here.

## In Scope

- Audit the prepared memory facts and frame-slot metadata available for the
  representative failures.
- Define the exact supported RV64 address forms for frame-slot base-plus-offset
  local memory accesses.
- Implement semantic RV64 object lowering for the first supportable load/store
  width and offset forms.
- Implement semantic RV64 object lowering for supportable pointer-value
  local-memory loads/stores when prepared metadata proves the pointer register,
  width, alignment, offset, and address-space facts.
- Keep unsupported diagnostics precise for offset, width, alignment, aggregate,
  or address materialization shapes that remain outside this idea.
- Add focused backend tests for supported and fail-closed local memory forms.
- Rerun the listed representative cases and document their outcomes.

## Out of Scope

- Broad aggregate `va_arg` helper lowering, even when `src/va-arg-13.c` is one
  representative for this memory-addressing boundary.
- Frame-slot address call-argument materialization; use
  `ideas/open/372_rv64_object_route_frame_slot_address_call_args.md`.
- Byval aggregate parameter homes.
- General terminator lowering or control-flow rewrites.
- Inferring memory layout from source syntax instead of consuming prepared
  frame-slot and memory metadata.
- Skipping or weakening gcc_torture runtime checks.

## Acceptance Criteria

- Focused RV64 object-emission tests prove supported frame-slot base-plus-offset
  and pointer-value local memory loads/stores.
- Unsupported local memory shapes keep a precise diagnostic instead of falling
  back to the generic prepared-module-shape error.
- At least one representative advances because of the semantic memory-addressing
  repair, and all listed representatives are rerun and recorded.
- Existing focused backend object-emission and prepared-contract coverage
  remains green.

## Reviewer Reject Signals

- Reject testcase-name shortcuts for `src/20000217-1.c`, `src/20000121-1.c`,
  `src/va-arg-13.c`, or their exact source shapes.
- Reject hard-coded offsets or frame-slot ids that are not derived from
  prepared memory metadata.
- Reject changes that silently truncate, widen, or misalign local memory
  accesses to make a representative pass.
- Reject expectation downgrades, skip broadening, or allowlist filtering
  claimed as progress.
- Reject diagnostic-only renames that leave the same unsupported addressing
  boundary behind new wording.
