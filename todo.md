Status: Active
Source Idea Path: ideas/open/242_prepared_stack_slot_preserved_value_extent.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing Prepared Authority

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by mapping every
`PreparedCallPreservationRoute::StackSlot` producer branch in
`prepare::build_call_preserved_values`.

Stack-slot route mapping:

- `PreparedValueHomeKind::StackSlot` branch:
  `find_prepared_value_home(*value_locations, value.value_id)` supplies
  `PreparedValueHome::slot_id` and `PreparedValueHome::offset_bytes` only.
  `PreparedValueHome` has no size or alignment fields. Blocker: no exact
  prepared size/alignment authority is carried by this branch today; a later
  code packet must either attach extent to the home or resolve the home's
  slot id to an authoritative prepared frame/assignment record before any
  target consumes it.
- `value.stack_object_id` branch:
  `find_prepared_frame_slot(prepared.stack_layout, *value.stack_object_id)`
  returns `PreparedFrameSlot`. This is the existing prepared authority for
  size/alignment: `PreparedFrameSlot::size_bytes` and
  `PreparedFrameSlot::align_bytes`, alongside `slot_id` and `offset_bytes`.
- `value.assigned_stack_slot` branch:
  `PreparedRegallocValue::assigned_stack_slot` supplies
  `PreparedStackSlotAssignment::slot_id` and `offset_bytes` only.
  `PreparedStackSlotAssignment` and its `PreparedSpillSlotPlacement` carry no
  size or alignment. Allocation computes normalized value size/alignment in
  `regalloc.cpp::allocate_stack_slot`, but the assignment does not publish
  those values as prepared facts. Blocker: no retained prepared
  size/alignment authority is available from this branch today.

## Suggested Next

Start `plan.md` Step 2 by adding first-class prepared extent fields for
stack-slot preserved values, populating the `value.stack_object_id` route from
`PreparedFrameSlot::size_bytes` and `PreparedFrameSlot::align_bytes`, and
making the two missing-authority routes fail or remain absent explicitly until
they have prepared extent authority.

## Watchouts

- Do not infer stack-slot preserved-value size or alignment inside AArch64
  lowering.
- Do not weaken preserved-value or call tests to claim progress.
- Keep register-route preserved values behaviorally stable.
- The `PreparedValueHomeKind::StackSlot` and `value.assigned_stack_slot`
  producer branches are current blockers for complete extent population unless
  the next packet adds or resolves prepared authority for them.

## Proof

Documentation/mapping packet only; no build or test run required. Per packet
instruction, `test_after.log` was not created or modified.
