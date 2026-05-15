Status: Active
Source Idea Path: ideas/open/241_prepared_callee_save_slot_placement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Publish And Observe Placement

# Current Packet

## Just Finished

Completed Step 4 from `plan.md`: published prepared saved-register
slot-placement facts through prepared printer output and added direct
observation coverage.

Concrete work completed:

- `prepared_printer.cpp` now emits `slot_placement` on saved-register lines in
  both prepared function summaries and the prepared frame-plan section.
- The emitted placement surface includes slot id, stack offset, size,
  alignment, fixed-location, register bank/name, save index, contiguous width,
  occupied registers, and structured register placement from
  `PreparedSavedRegister::slot_placement`.
- `backend_prepared_printer_test` now observes the mapping for both a
  single-register GPR save and a grouped vector-register save without relying
  on AArch64 target-local inference.
- Existing prepared printer, CLI prepared dump, prepared frame, and AArch64
  printer/record tests remain stable with the intentional new saved-register
  placement fields.

## Suggested Next

Proceed to the next supervisor-selected packet. A coherent next slice would be
consumer-side use of the prepared saved-register slot placement, if the active
plan intends to move beyond observation.

## Watchouts

- Placement remains populated for fixed-frame functions only. Dynamic-stack
  functions deliberately keep missing slot placement from the prior packet, so
  the pre-existing fixed-slot frame-pointer contract does not change.
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

Result: passed. Proof log: `test_after.log`.
