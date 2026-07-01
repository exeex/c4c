Status: Active
Source Idea Path: ideas/open/509_rv64_fixed_prepared_stack_frame_emission.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Fixed-Slot Addressing

# Current Packet

## Just Finished

Step 4 of `plan.md` implemented prepared fixed-slot addressing for RV64 object
emission.

Changed files:
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Implementation boundary:
- Prepared stack-slot value homes now have a fact-returning `size_t` offset
  path alongside the existing signed-12-bit helper. Code-emitting consumers
  that can use large prepared fixed-slot offsets call the fact path and then
  materialize above the low-level encoders.
- Prepared direct frame-slot memory accesses now validate slot offset, width,
  alignment, extent, frame bounds, and published function membership when both
  sides publish valid function names. Legacy slots with invalid function names
  keep their prior compatibility.
- Large fixed-slot GPR loads, GPR stores, FPR local loads/stores, scalar value
  homes, selected stack-result homes, binary/cast/select stack publications,
  global-load stack publications, and formal/home move paths now use
  materialized stack-offset address helpers when the prepared offset exceeds
  signed 12-bit range.
- Frame-slot address materialization now uses the prepared materialization fact
  and emits `addi rd, sp, off` for small offsets or materializes the absolute
  stack offset and adds `sp` for large offsets.
- Low-level stack load/store/FPR encoders still require signed 12-bit
  immediates; no impossible immediate encoding was admitted.
- Dynamic stack, frame-pointer fixed slots, missing/incoherent facts,
  unsupported widths/alignments, F128/vector, and broad non-goal saved-register
  support remain fail-closed.

Covered slot/addressing shapes:
- Added `builds_prepared_large_fixed_slot_addressing_object`, a generic
  8192-byte fixed-frame fixture that covers a large-offset frame-slot address
  materialization, a large-offset pointer store, a large-offset scalar store,
  and a large-offset scalar load without using the gcc torture row or its
  constants.
- Existing small fixed-slot local, FPR local, subobject, call, move-bundle, and
  publication tests remain green under the new helper boundary.

Representative status:
- `tests/c/external/gcc_torture/src/20030209-1.c` advances past the old
  `RV64 object route requires a supported prepared stack frame` rejection.
- The current focused probe stops at the existing non-goal diagnostic:
  `unsupported_stack_frame: RV64 object route does not support non-GPR prepared
  callee-saved register save slots (fpr:fs1)`.
- Representative artifact:
  `build/agent_state/509_step4_fixed_slot_addressing/representative_20030209-1.log`.

## Suggested Next

Execute Step 5: add ordinary-C coverage for the repaired large fixed-frame and
fixed-slot addressing route. Keep it independent from `src/20030209-1.c` and
avoid encoding the representative constants or source shape as the proof.

## Watchouts

- Keep the route limited to RV64 object emission consuming explicit prepared
  fixed-frame facts.
- Do not infer frame size, stack offsets, widths, alignments, or slot layout
  from testcase names, raw slot counts, source shape, or magic constants.
- Do not include dynamic stack, FPR, F128, vector, varargs, producer-layout, or
  broad ABI support in this plan.
- Preserve the FPR/F128/vector boundary: `diagnose_unsupported_prepared_saved_register_bank`
  must continue rejecting non-GPR saved-register facts. The representative
  prepared plan now exposes the existing `fpr:fs1` non-GPR saved-register
  diagnostic after fixed-frame admission succeeds.
- Do not infer frame size, stack offsets, slot widths, slot alignments, or
  ordinary-C coverage shape from `src/20030209-1.c`, `80000`, `10000`, source
  layout, or raw slot counts.
- Low-level load/store immediate encoders remain signed-12-bit only; future
  large-offset support should continue to materialize addresses above them.
- Call-frame RA save/restore now has large-offset materialization for the
  frame prologue/epilogue only; saved GPR spill/restore slot support remains at
  the pre-existing boundary.

## Proof

- `scripts/plan_review_state.py set-step --step-id 4 --step-title 'Implement Fixed-Slot Addressing'`
- `cmake --build build --target c4cll`
- `cmake --build build --target backend_riscv_object_emission_test`
- `ctest --test-dir build --output-on-failure -R '^backend_riscv_object_emission$'`
- Focused representative probe through
  `tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake` for
  `tests/c/external/gcc_torture/src/20030209-1.c`; expected non-goal
  `fpr:fs1` diagnostic recorded at
  `build/agent_state/509_step4_fixed_slot_addressing/representative_20030209-1.log`.
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
  passed: 332/332 tests.
- `git diff --check -- src/backend/mir/riscv/codegen/object_emission.cpp tests/backend/mir/backend_riscv_object_emission_test.cpp todo.md`
