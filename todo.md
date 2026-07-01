Status: Active
Source Idea Path: ideas/open/509_rv64_fixed_prepared_stack_frame_emission.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Ordinary-C Coverage

# Current Packet

## Just Finished

Step 5 of `plan.md` added focused ordinary-C backend coverage for the repaired
RV64 large fixed-frame and fixed-slot addressing route.

Changed files:
- `tests/backend/case/riscv64_large_fixed_frame_slot_access.c`
- `tests/backend/CMakeLists.txt`
- `todo.md`

Coverage rationale:
- Added `backend_obj_runtime_rv64_large_fixed_frame_slot_access`, an ordinary-C
  RV64 object-route runtime test over a 1501-element fixed local array.
- The case writes and reads near, far, and tail fixed slots, forcing a fixed
  frame larger than signed-12-bit stack offsets and requiring prepared fixed
  slot facts for large-offset accesses.
- The case is independent from `src/20030209-1.c`; it does not use the
  representative's `80000` frame, `10000` slots, raw slot numbers, function
  names, or source shape as the proof.
- No expectations, unsupported markers, allowlists, runtime accounting, scan
  accounting, or compiler code were changed.

Representative status:
- `tests/c/external/gcc_torture/src/20030209-1.c` advances past the old
  `RV64 object route requires a supported prepared stack frame` rejection.
- The current focused probe stops at the existing non-goal diagnostic:
  `unsupported_stack_frame: RV64 object route does not support non-GPR prepared
  callee-saved register save slots (fpr:fs1)`.
- Representative artifact:
  `build/agent_state/509_step5_ordinary_c_coverage/representative_20030209-1.log`.

## Suggested Next

Execute Step 6: validate the completed slice, confirm no route drift or
expectation downgrade was used, and prepare closure or a follow-up split for the
remaining non-goal saved-register/FPR boundary.

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
- A pointer-local variant of the new ordinary-C case reached the existing
  prepared callee-saved GPR save-slot boundary before fixed-slot proof; it was
  not kept because Step 5 must not expand saved-register support.
- Low-level load/store immediate encoders remain signed-12-bit only; future
  large-offset support should continue to materialize addresses above them.
- Call-frame RA save/restore now has large-offset materialization for the
  frame prologue/epilogue only; saved GPR spill/restore slot support remains at
  the pre-existing boundary.

## Proof

- `scripts/plan_review_state.py set-step --step-id 5 --step-title 'Add Ordinary-C Coverage'`
- `cmake --build build --target c4cll`
- `ctest --test-dir build --output-on-failure -R '^backend_obj_runtime_rv64_large_fixed_frame_slot_access$'`
  passed.
- Focused representative probe through
  `tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake` for
  `tests/c/external/gcc_torture/src/20030209-1.c`; expected non-goal
  `fpr:fs1` diagnostic recorded at
  `build/agent_state/509_step5_ordinary_c_coverage/representative_20030209-1.log`.
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
  passed: 333/333 tests.
- `git diff --check -- tests/backend/CMakeLists.txt todo.md`
- `git diff --check --no-index /dev/null tests/backend/case/riscv64_large_fixed_frame_slot_access.c`
  returned only the expected no-index difference status with no whitespace
  diagnostics.
