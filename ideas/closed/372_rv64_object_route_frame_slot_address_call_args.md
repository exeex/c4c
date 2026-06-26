# RV64 Object Route Frame-Slot Address Call Arguments

Status: Closed
Type: Repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md`

## Goal

Lower prepared call arguments that require materializing a frame-slot address
in the RV64 object route.

## Why This Exists

Idea 368 Step 3 proved that direct frame-slot subobject-offset local-memory
support did not advance its three representatives. The prepared audits show a
separate residual owner: call arguments whose source selection is a local
frame-slot address rather than a scalar load/store payload.

Known representative evidence:

- `src/20000217-1.c`: `main` passes `%lv.x` and `%lv.y` addresses with
  `arg.source_selection=local_frame_address_materialization`.
- `src/va-arg-13.c`: `test` passes frame-slot addresses for `%t7` and `%t14`
  and records `missing_frame_slot_arg_publication=yes` with
  `missing_frame_slot_arg_kind=frame_slot_address`.

This is related to local frame-slot metadata but is not the same implementation
owner as pointer-value local-memory loads/stores.

## In Scope

- Audit prepared call plans and address-materialization records for
  frame-slot-address call arguments.
- Define the first supportable RV64 address materialization form for a stack
  slot plus constant offset passed in a GPR argument register.
- Implement object-emission support using prepared frame-slot ids, stack
  offsets, value homes, and call-argument ABI placements.
- Keep unsupported dynamic stack, non-default address space, TLS, aggregate,
  missing-slot, out-of-range offset, and non-GPR destination cases fail-closed
  with precise diagnostics.
- Add focused backend tests for admitted and rejected frame-slot address
  call-argument forms.
- Rerun relevant representatives and record whether they pass or advance to
  aggregate `va_arg`, byval, or terminator owners.

## Out of Scope

- Pointer-value local-memory loads/stores; idea 368 owns that route.
- Aggregate `va_arg` helper lowering.
- Byval aggregate parameter homes.
- General terminator lowering or control-flow rewrites.
- Inferring stack layout from source syntax or testcase names.
- Skipping, allowlist-filtering, or weakening gcc_torture runtime checks.

## Acceptance Criteria

- Focused RV64 object-emission tests prove materialization of supportable
  frame-slot addresses into call-argument GPRs.
- Unsupported frame-slot address call-argument shapes keep precise diagnostics.
- At least one representative advances because of semantic frame-slot address
  call-argument repair, and all listed representatives are rerun and recorded.
- Existing focused backend object-emission and prepared-contract coverage
  remains green.

## Completion Notes

Closed on 2026-06-26 after the RV64 object route admitted focused
`LocalFrameAddressMaterialization` GPR call arguments in commit `b7276260`.
Focused backend tests prove the positive stack-slot-address call-argument form
and fail-closed adjacent unsupported shapes.

Representative results:

- `src/20000217-1.c` was rerun and advanced off the frame-slot-address call
  publication owner. It still fails at generic `unsupported_instruction_fragment`,
  with packet probes showing the visible residual in `showbug` as scalar
  compare-result lowering for `sge` feeding `trunc i1 -> i16`.
- `src/va-arg-13.c` remains blocked earlier by `unsupported_param_home`, owned
  by `ideas/open/374_rv64_object_route_non_register_param_homes.md`, before
  its later frame-slot address publications can be proved in file-level object
  mode.

The scalar compare/trunc residual from `src/20000217-1.c` is tracked separately
by `ideas/open/375_rv64_object_route_scalar_compare_trunc_lowering.md`.

Close-time focused backend proof passed with non-decreasing regression guard:
`3/3` before, `3/3` after.

## Reviewer Reject Signals

- Reject testcase-name shortcuts for `src/20000217-1.c`,
  `src/va-arg-13.c`, or their exact source shapes.
- Reject hard-coded frame-slot ids, stack offsets, or argument registers that
  are not derived from prepared call-plan and frame-plan metadata.
- Reject changes that materialize the wrong address, pass payload values when
  an address is required, or silently ignore ABI call-argument placement facts.
- Reject expectation downgrades, skip broadening, allowlist filtering, or
  diagnostic-only renames claimed as capability progress.
- Reject broad changes to aggregate `va_arg`, byval, terminator, or unrelated
  pointer-memory behavior inside this idea.
