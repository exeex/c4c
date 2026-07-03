# Current Packet

Status: Active
Source Idea Path: ideas/open/564_rv64_fpr_callee_saved_frame_slots.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused FPR Frame-Slot Coverage

## Just Finished

Executed Step 2, `Add Focused FPR Frame-Slot Coverage`.

Changed files:

- `src/backend/mir/riscv/codegen/prepared_frame_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Coverage added:

- Added `rv64_prepared_saved_callee_fpr_stack_offset(...)` as the minimal
  RV64 frame-consumer contract hook for prepared FPR callee-saved slot facts.
- Added focused backend coverage for an explicit `fpr:fs1` saved-register
  shape proving the prepared FPR bank, save index `0`, slot id, stack offset
  `24`, and 8-byte size/alignment are visible target facts.
- Added fail-closed coverage for missing prepared FPR slot placement,
  mismatched save index, malformed slot size, non-fixed slot, mismatched
  register placement bank, and out-of-frame offset.
- Existing GPR frame behavior remains covered by the existing backend object
  emission suite; no GPR frame materialization rewrite was made.

Step 3 remains a separate repair packet. This slice did not implement FPR
`fsd`/`fld` save/restore materialization or remove the current object-route
non-GPR callee-saved rejection.

## Suggested Next

Execute Step 3, `Repair RV64 FPR Callee-Saved Slot Consumption`.

Executor packet:

- Objective: extend RV64 frame lowering to consume the validated prepared FPR
  callee-saved slot facts by saving/restoring prepared FPR callee-saved
  registers.
- Owned files: `todo.md`, `src/backend/mir/riscv/codegen/prepared_frame_emit.*`,
  `src/backend/mir/riscv/codegen/object_emission.cpp`, and focused RV64 backend
  object-emission tests.
- Do not touch: prepared frame-layout production, `plan.md`, source idea files,
  `review/`, expectations, unsupported markers, representative allowlists, or
  unrelated global/F128/F64 lanes.
- Repair requirements: reuse `rv64_prepared_saved_callee_fpr_stack_offset(...)`
  for fact validation; emit appropriate 8-byte FPR stack saves/restores for
  prepared FPR callee-saved slots; preserve GPR save/restore behavior; keep
  malformed/missing FPR facts fail-closed.
- Proof command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- Done when: backend proof passes and focused object-emission coverage proves
  valid prepared FPR callee-saved save/restore materialization without
  accepting malformed FPR slot facts.

## Watchouts

- Leave prepared frame-layout production out of scope; current prepared dumps
  prove the FPR callee-saved slot facts are present.
- Do not special-case filenames, `fs1`, diagnostic text, unsupported markers,
  allowlists, or expected outputs. The new helper validates prepared facts, but
  Step 3 must still avoid register-name-only acceptance.
- `src/20030209-1.c` also publishes `fpr:fs2`; the repair should naturally
  handle multiple prepared FPR saved registers.
- The existing admission gate still rejects non-GPR saved registers before
  prologue/epilogue emission. Step 3 must move that gate to validated FPR
  support rather than merely hiding the diagnostic.

## Proof

Proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `345/345` backend tests.

Log path: `test_after.log`.
