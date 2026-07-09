Status: Active
Source Idea Path: ideas/open/626_prepared_dynamic_frame_callee_saved_slot_placement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate Producer Authority

# Current Packet

## Just Finished

Step 2 located the prepared-frame producer authority for callee-saved GPR
save-slot placements:

- Producer function target: `populate_frame_plan(PreparedBirModule&)` in
  `src/backend/prealloc/frame_plan.cpp`. It already gathers saved
  callee-saved register identity from `prepared.regalloc`, derives `save_index`
  via `callee_saved_span_save_index`, and owns the final
  `PreparedFramePlanFunction::saved_callee_registers` publication.
- Producer helper target: local
  `make_saved_register_slot_placement(...)` in
  `src/backend/prealloc/frame_plan.cpp`. It already builds the complete
  `PreparedSavedRegisterSlotPlacement` payload from a saved register plus
  slot id, stack offset, size, and alignment.
- Frame-layout authority feeding the producer: `BirPreAlloc::run_stack_layout()`
  and `stack_layout::assign_frame_slots(...)` publish per-function fixed-slot
  frame size/alignment and `PreparedFrameSlot` offsets before
  `populate_frame_plan` runs. `populate_frame_plan` also computes the
  callee-saved area after fixed frame slots using `next_prepared_frame_slot_id`,
  `saved_register_slot_unit_size`, and `align_prepared_offset`.
- Existing carrier fields are sufficient for Step 3: `PreparedSavedRegister`
  already has optional `slot_placement`, and
  `PreparedSavedRegisterSlotPlacement` already carries bank, register name,
  width, occupied units, save index, register placement, slot id,
  stack-offset bytes, size, alignment, and `fixed_location`.
- Current dynamic-stack gap: the existing publication block is guarded by
  `if (!plan.has_dynamic_stack)`, so dynamic rows keep saved-register identity
  but publish no `slot_placement`. A fixed-frame comparison row currently
  shows complete `slot_placement=slot#...+stack... slot_size=8 slot_align=8`
  facts, confirming the helper/carrier path is live for non-dynamic frames.
- Fail-closed condition: if a saved callee-saved GPR has no complete
  producer-published `slot_placement`, or if unit size/register placement
  authority is missing, RV64 must continue rejecting the object route rather
  than deriving placement from frame size, register order, source filename, or
  final assembly shape.

## Suggested Next

Run Step 3 as a narrow producer implementation packet in
`src/backend/prealloc/frame_plan.cpp`: remove the dynamic-stack exclusion from
callee-saved GPR slot-placement publication only after confirming the computed
callee-saved area remains after fixed slots and uses `fp`-stable offsets for
dynamic frames. Add focused producer coverage proving at least one idea-626 row
emits complete `slot_placement` facts in the prepared dump before RV64 object
emission.

## Watchouts

- Do not infer callee-saved save-slot placements in RV64 from frame size,
  register order, source filename, or final assembly shape.
- Keep FPR placement, move-bundle authority, local/global memory repair, call
  policy, and expectation changes outside this idea.
- Preserve dynamic stack operation metadata and existing frame size/alignment
  facts while adding placement authority.
- The implementation should not add a new broad carrier unless Step 3 finds an
  unstated authority gap; current evidence says the existing
  `PreparedSavedRegisterSlotPlacement` carrier is enough.
- Do not add generated callee-saved slot ids to `frame_slot_order` unless the
  consumer contract explicitly needs it; existing fixed-frame placement tests
  expect save slots to remain producer-published placement facts, not ordinary
  fixed frame slots.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed: 347/347 backend tests. Proof log: `test_after.log`.
