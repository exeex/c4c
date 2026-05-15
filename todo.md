Status: Active
Source Idea Path: ideas/open/241_prepared_callee_save_slot_placement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate And Summarize

# Current Packet

## Just Finished

Completed Step 5 from `plan.md`: validated and summarized the prepared
callee-save slot-placement contract.

Concrete work completed:

- Full-suite build and CTest validation passed.
- Supported placement states are fixed-frame saved-register homes where
  prepared frame authority can connect each saved callee register to a
  concrete frame slot and stack offset.
- The prepared placement surface records slot id, stack offset, size,
  alignment, fixed-location state, register bank/name, save index, contiguous
  width, occupied registers, and structured register placement.
- Direct prepared-printer observations cover both a single-register GPR save
  and a grouped vector-register save without requiring AArch64 target-local
  layout reconstruction.

## Suggested Next

Runbook is ready for supervisor plan-owner close/deactivate review. A later
consumer route can teach AArch64 save/restore lowering to consume
`PreparedSavedRegister::slot_placement` directly.

## Watchouts

- Placement remains populated for fixed-frame functions only.
- Dynamic-stack functions deliberately fail closed with missing slot placement,
  preserving the pre-existing fixed-slot frame-pointer contract instead of
  fabricating target-local save homes.
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

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1`

Result: passed; 3167/3167 tests passed. Proof log: `test_after.log`.
