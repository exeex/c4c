Status: Active
Source Idea Path: ideas/open/241_prepared_callee_save_slot_placement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Placement Authority

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: inspected prepared callee-save register and
frame-slot authority without implementation edits.

Current production facts:

- `PreparedFrameSlot` is produced by `stack_layout::assign_frame_slots` from
  prepared stack objects that require home slots. Each slot already has a
  `PreparedFrameSlotId`, object identity, function identity, byte offset,
  size, alignment, and fixed-location bit.
- `BirPreAlloc::run_stack_layout` owns the per-function stack-object collection
  and initializes the global `next_slot_id` used by `assign_frame_slots`.
- `populate_frame_plan` reads `prepared.stack_layout.frame_slots` by function,
  publishes `PreparedFramePlanFunction::frame_slot_order`, and sorts slot IDs
  by authoritative `PreparedFrameSlot::offset_bytes`.
- `populate_frame_plan` also produces `PreparedSavedRegister` entries from
  regalloc values assigned to target callee-saved registers. Each saved record
  carries register bank, register spelling, contiguous width, occupied register
  names, `save_index`, and optional register placement, then is sorted by
  bank, save index, width, and register name.
- No prepared-layer record currently ties a `PreparedSavedRegister` to a
  `PreparedFrameSlotId` or prepared stack offset. AArch64 has optional
  `CalleeSaveInstructionRecord::slot_id` and `stack_offset_bytes` fields ready
  to consume that link, but `PreparedFramePlanFunction` currently publishes
  saved registers and frame-slot order as separate streams.

## Suggested Next

Proceed to the first implementation packet: define the prepared carrier in
`src/backend/prealloc/prealloc.hpp`, rooted in `PreparedSavedRegister`, so each
saved callee register can optionally publish its explicit
`PreparedFrameSlotId` and prepared stack offset. The focused proof subset for
that packet should build and run the prepared/AArch64 carrier tests:
`cmake --build build --target backend_prepare_frame_stack_call_contract_test
backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test`
then `ctest --test-dir build -R
'^(backend_prepare_frame_stack_call_contract|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer)$'
--output-on-failure`.

## Watchouts

- Execution can proceed to carrier definition; the missing authority is not
  source intent, but the lack of a prepared data edge between saved registers
  and frame slots.
- Do not place saved-register slots by deriving offsets in AArch64 from
  `save_index`, sorted slot order, or register spelling. AArch64 should consume
  prepared slot facts after they are published.
- `src/backend/mir/aarch64/codegen/traversal.cpp` currently treats any nonempty
  `saved_callee_registers` as outside the simple fixed-frame path, and
  `machine_printer.cpp` still rejects callee-save frame nodes; those are later
  consumers, not the first packet target.
- Do not emit AArch64 callee-save stores or loads in this prerequisite.
- Do not infer slots in AArch64 target lowering from `save_index`,
  `frame_slot_order`, register names, or sorted offsets.
- Do not broaden into variadic save areas, outgoing call stack areas, general
  frame allocation redesign, or preserved-value extent work.
- Treat expectation weakening, named-case shortcuts, and target-local layout
  reconstruction as route drift.

## Proof

Inspection-only packet. No build was required by the delegated proof contract;
ran `git diff --check` after updating `todo.md`.
