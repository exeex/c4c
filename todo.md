Status: Active
Source Idea Path: ideas/open/241_prepared_callee_save_slot_placement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The Placement Carrier

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: defined the prepared saved-register
slot-placement carrier and attached it to `PreparedSavedRegister`.

Concrete work completed:

- Added `PreparedSavedRegisterSlotPlacement` in
  `src/backend/prealloc/prealloc.hpp`.
- The carrier preserves register bank, register name, contiguous width,
  occupied register names, save index, optional register placement, optional
  frame slot id, optional stack offset, optional size, optional alignment, and
  the fixed-location bit.
- Added `has_complete_prepared_saved_register_slot_placement` so incomplete
  placement data remains fail-closed instead of being silently consumed.
- Added a focused prepared frame/call contract check proving the carrier can
  hold the structured facts and rejects incomplete stack-offset or register
  placement facts.

## Suggested Next

Proceed to Step 3: populate `PreparedSavedRegister::slot_placement` from
prepared frame/layout authority, preserving deterministic saved-register order
and avoiding target-local reconstruction from `save_index`, register spelling,
or `frame_slot_order`.

## Watchouts

- The carrier is defined but not populated yet; current prepared output still
  publishes saved registers and frame slots separately.
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

Ran the delegated proof:

`(cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test -j2 && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer)$' --output-on-failure) > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
