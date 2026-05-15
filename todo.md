Status: Active
Source Idea Path: ideas/open/241_prepared_callee_save_slot_placement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Populate Placement Facts

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`: populated prepared saved-register
slot-placement facts for representative fixed-frame callee-save cases.

Concrete work completed:

- `populate_frame_plan` now computes frame-plan-local fixed callee-save
  placements in deterministic saved-register order and fills
  `PreparedSavedRegister::slot_placement` with slot id, stack offset, size,
  alignment, fixed-location, register placement, save index, bank, register
  name, contiguous width, and occupied-register facts.
- Existing `PreparedStackLayout::frame_slots`, frame size/alignment, and
  `frame_slot_order` publication remain unchanged so prepared printer,
  variadic, and CLI dump contracts keep their prior observable layout.
- The placement path is prepared-layer owned; no AArch64 save-index,
  register-spelling, or sorted-slot reconstruction was added.
- Dynamic-stack functions remain fail-closed for this fixed-frame packet, so
  existing no-fixed-slot frame-pointer behavior is preserved.
- Focused prepared frame/call contract coverage now checks one-register GPR
  placement and grouped multi-register placement without mutating existing
  frame-slot order.

## Suggested Next

Proceed to Step 4: publish and observe the placement relation in prepared dump
or another prepared observation surface so tests can assert the mapping without
direct structure inspection.

## Watchouts

- Placement is populated for fixed-frame functions only. Dynamic-stack
  functions deliberately keep missing slot placement for this packet so the
  pre-existing fixed-slot frame-pointer contract does not change.
- Prepared dump output still does not print the new placement mapping; Step 4
  should make the relationship directly observable.
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

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_printer|backend_cli_dump_prepared_bir_local_arg_call_contract|backend_prepare_frame_stack_call_contract|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer)$') > test_after.log 2>&1`

Result: passed. Also ran `git diff --check`, which passed. Proof log:
`test_after.log`.
