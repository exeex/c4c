# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm LocalFrameAddressMaterialization Boundary

## Just Finished

Completed Step 1 inventory for `LocalFrameAddressMaterialization`.

Producer paths:

- `src/backend/prealloc/call_plans.cpp`
  `select_prepared_call_argument_source` selects
  `LocalFrameAddressMaterialization` for local aggregate address publication or
  byval pointer arguments when the source is either a register home with a
  same-block local-frame-address derivation or a computed-address home with a
  pointer base value.
- Both producer branches require a latest prepared frame-slot materialization
  for the selected source value name.
- The producer fills materialization block label, instruction index,
  frame-slot id, and byte offset through
  `copy_materialization_source_selection_fields`, then adjusts
  `address_materialization_byte_offset` by the selected pointer byte delta.
- `source_stack_offset_bytes` is valid only when the adjusted materialization
  byte offset is nonnegative. The route currently returns no selection without
  that stack offset.
- `source_pointer_byte_delta` is required and may be zero.
- `source_size_bytes` and `source_align_bytes` come from byval ABI payload
  size/alignment when present, otherwise from the source home with current
  fallback defaults.

Required typed facts:

- `source_value_id` and `source_value_name`
- `source_home_kind` of `Register` or `PointerBasePlusOffset`, matching the
  producer branch
- `source_slot_id`
- `source_stack_offset_bytes`
- `source_size_bytes`
- `source_align_bytes`
- `source_pointer_byte_delta`
- complete address materialization payload: block label, instruction index,
  frame-slot id, and adjusted byte offset

Rejected payloads:

- missing any required fact above
- negative adjusted materialization byte offset
- materialization frame-slot id that contradicts source slot id
- preservation payload fields
- byval-lane payload fields
- `FrameSlotAddress`/`FrameSlotValue` typed-route acceptance without pointer
  delta and complete materialization payload

Current consumers:

- `plan_prepared_aggregate_transport` consumes this route for stack-copy byval
  aggregate transport and reads stack offset, size, alignment, source slot, and
  source value name.
- RV64 `prepared_call_emit.cpp` emits local materialized addresses directly
  from `source_stack_offset_bytes` for simple prepared calls and separately
  uses aggregate transport for byval stack copies.
- RV64 `object_emission.cpp` validates local frame-address materialization
  fields for object emission and frame-slot address publication paths.
- AArch64 `StackFrameSlotCallOperandOwner::selected_local_frame_address_source`
  and `make_selected_call_argument_source` read materialization fields and
  stack offset through the compatibility bag.
- AArch64 local aggregate address call-argument lowering requires
  `LocalFrameAddressMaterialization` and fails with explicit diagnostics when
  selection/source materialization is incomplete.

## Suggested Next

Begin Step 2 by adding a typed
`PreparedCallArgumentLocalFrameAddressMaterializationRoute` and
`as_local_frame_address_materialization_route` in
`src/backend/prealloc/calls.hpp`, plus focused route-query tests.

## Watchouts

- Do not add optional fields to `PreparedCallArgumentSourceSelection`.
- Do not merge `LocalFrameAddressMaterialization` with `ByvalRegisterLane`
  unless lifecycle review creates a separate coupling plan.
- Preserve completed `FrameSlotAddress` and `FrameSlotValue` typed contracts.
- The previous broad validation before regeneration was 3356/3356 passing.
- Keep byval aggregate transport as a consumer of the local-materialization
  route; do not absorb byval register-lane payloads into this typed route.

## Proof

Inventory-only packet; no build or CTest required. Proof command:
`git diff -- todo.md`.
