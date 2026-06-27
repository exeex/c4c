# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm FrameSlotValue Route Boundary

## Just Finished

Completed Step 1 inventory for `FrameSlotValue`.

Producer path:

- `src/backend/prealloc/call_plans.cpp`
  `select_prepared_call_argument_source` selects `FrameSlotValue` only when
  `argument.source_encoding == PreparedStorageEncodingKind::FrameSlot` and the
  argument was not classified as sret/local frame-slot address or byval lane.
- `copy_home_source_selection_fields` supplies `source_value_id`,
  `source_value_name`, `source_home_kind`, `source_slot_id`,
  `source_stack_offset_bytes`, `source_size_bytes`, and `source_align_bytes`
  from the prepared value home when available.
- `copy_access_source_selection_fields` can refine slot/offset/size/align from
  a prepared frame-slot memory access.
- The fallback fills `source_slot_id` and `source_stack_offset_bytes` from the
  call argument plan, then currently accepts the selection if either slot or
  offset is present. Step 2 should make the typed query stricter than that old
  compatibility acceptance.

Required typed `FrameSlotValue` facts:

- `source_value_id`
- `source_value_name`
- `source_home_kind == PreparedValueHomeKind::StackSlot`
- `source_slot_id`
- `source_stack_offset_bytes`
- `source_size_bytes`
- `source_align_bytes`

Rejected payloads for the typed query:

- missing any required fact above
- non-stack source-home kind
- address-materialization payload fields
- source-base or pointer-delta payload fields
- preservation payload fields
- byval-lane payload fields

Current consumers:

- AArch64 `StackFrameSlotCallOperandOwner::selected_frame_slot_source` already
  has a `FrameSlotValue` branch that requires source value id/name, stack-slot
  home kind, slot, offset, extent, and alignment before building a
  `MemoryOperand`.
- AArch64 `make_selected_call_argument_source` routes multiple publication and
  call-lowering sites through `selected_frame_slot_source` for
  `FrameSlotValue`.
- RV64 object emission `prepared_frame_slot_call_argument_offset` accepts
  `FrameSlotValue` as a stack source, checks source encoding/value bank/value
  id/slot against value-home data, and still reads selected fields through the
  compatibility bag.
- RV64 object emission call lowering switches on `FrameSlotValue` and currently
  lets the frame-slot source path proceed after only checking source encoding.

## Suggested Next

Begin Step 2 by adding `PreparedCallArgumentFrameSlotValueRoute` and
`as_frame_slot_value_source_route` in `src/backend/prealloc/calls.hpp`, plus
focused route-query tests. Do not add optional fields to
`PreparedCallArgumentSourceSelection`.

## Watchouts

- Do not add optional fields to `PreparedCallArgumentSourceSelection`.
- Do not disturb the completed `FrameSlotAddress` route except where a shared
  helper needs a behavior-preserving cleanup.
- `FrameSlotValue` should require source value/home identity plus source slot,
  stack offset, extent, and alignment.
- Keep `FrameSlotAddress`, `PriorPreservation`,
  `LocalFrameAddressMaterialization`, and `ByvalRegisterLane` route behavior
  unchanged unless the active step explicitly owns a shared boundary.
- Do not preserve the old typed-query behavior that accepts only slot or only
  stack offset; that compatibility should remain outside the typed API.

## Proof

Inventory-only packet; no build or CTest required. Proof command:
`git diff -- todo.md`.
