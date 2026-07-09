Status: Active
Source Idea Path: ideas/open/626_prepared_dynamic_frame_callee_saved_slot_placement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Guard RV64 Consumer Use Of Prepared Placement Authority

# Current Packet

## Just Finished

Step 4 guarded the RV64 object consumer so dynamic-stack callee-saved rows are
admitted only through producer-published, complete GPR save-slot placement
facts, without adding broad dynamic-frame object lowering.

Changed files:

- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.hpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`
- `test_after.log`

Consumer behavior:

- Added `rv64_prepared_validated_dynamic_saved_gpr_frame_size(...)` and routed
  `rv64_prepared_object_stack_frame_size(...)` through it for
  `has_dynamic_stack` frame-plan rows.
- The dynamic validator remains intentionally narrow: it does not admit
  frame-pointer fixed-slot dynamic lowering, dynamic local memory accesses, or
  ordinary function frame slots. It only sizes a simple dynamic-stack object
  frame from existing frame size plus complete saved GPR slot-placement facts.
- Existing prologue/epilogue emission still calls
  `rv64_prepared_saved_callee_gpr_stack_offset(...)`, so missing placement,
  mismatched register identity, contradictory size/alignment, out-of-frame
  offsets, or non-fixed placement fail closed instead of being recovered from
  frame size, register order, source filename, final assembly shape, or dynamic
  heuristics.
- Dynamic non-GPR saved-register rows now get an RV64-local diagnostic requiring
  producer-published GPR callee-saved save-slot placement for dynamic stack
  frames; no FPR placement policy was added.

Test coverage:

- Added a dynamic-stack prepared GPR callee-saved object fixture that builds
  only when the saved register has producer-published
  `PreparedSavedRegisterSlotPlacement`.
- Added fail-closed object-route coverage for absent `slot_placement`,
  mismatched saved-register identity, wrong size, wrong alignment, non-fixed
  placement, and a dynamic FPR saved-register boundary.
- No CMake changes were required because the coverage lives in the existing
  `backend_riscv_object_emission_test` target.

## Suggested Next

Run Step 5 as a narrow admission/evidence packet against the original
idea-626 representative rows: confirm where RV64 object emission now rejects
the remaining dynamic-frame owners after the saved-GPR placement guard, and
classify the next real owner bucket without broadening into dynamic local memory
lowering unless the supervisor explicitly selects that route.

## Watchouts

- Step 4 intentionally admits only the simple dynamic-stack saved-GPR object
  shape; dynamic frame-pointer fixed-slot lowering and dynamic local memory
  lowering remain outside this slice.
- The saved GPR offset still comes from `PreparedSavedRegister::slot_placement`
  and the existing offset validator, not from saved-register ordering or a
  dynamic-frame heuristic.
- Dynamic FPR/vector placement remains out of scope for idea 626 and now fails
  with the focused dynamic GPR placement diagnostic.
- Saved-register save slots are still not ordinary fixed frame slots and should
  not be treated as entries in `frame_slot_order`.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed, including 347/347 backend tests. Proof log: `test_after.log`.
