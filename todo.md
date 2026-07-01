Status: Active
Source Idea Path: ideas/open/509_rv64_fixed_prepared_stack_frame_emission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Fixed-Frame Validation And Adjustment

# Current Packet

## Just Finished

Step 3 of `plan.md` implemented the RV64 fixed-frame validation and large
stack-pointer adjustment slice.

Changed files:
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Implementation boundary:
- `rv64_object_stack_frame_size` now validates prepared fixed-frame plans
  separately from signed 12-bit `addi` encodability. Coherent frame-plan,
  addressing, stack-layout slot membership, saved-register placement, alignment,
  and size facts can produce a large aligned frame size.
- The legacy no-frame-plan path remains limited to signed 12-bit frame sizes so
  older small-frame fixtures keep their prior behavior.
- Large local and call-frame stack-pointer adjustments now materialize the byte
  delta with `append_rv64_load_immediate` and add it to `sp`; low-level
  immediate load/store encoders remain unchanged.
- Large call-frame return-address save/restore offsets use a materialized
  scratch address above the existing base load/store helpers.
- Dynamic stack, frame-pointer fixed slots, contradictory slot extents vs
  frame-plan size, incomplete saved-register placement, size overflow, and
  unsupported alignment remain fail-closed at the stack-frame gate. Zero-size
  frame slots remain admissible for zero-frame vector/inline-asm non-goal
  fixtures so those routes keep their existing diagnostics.
- Non-GPR saved-register facts are still rejected by
  `diagnose_unsupported_prepared_saved_register_bank`; no FPR/F128/vector
  support was added.

Focused coverage:
- Added `builds_prepared_large_fixed_stack_frame_adjustment_object`, a generic
  4096-byte prepared fixed-frame fixture that verifies materialized prologue and
  epilogue stack-pointer adjustment shape without using the gcc torture row or
  its constants.
- Updated focused fail-closed expectations where Step 3 intentionally moves
  dynamic/frame-pointer/contradictory frame facts to the stack-frame diagnostic
  and lets a coherent 2048-byte frame-slot value-argument case advance to the
  later fixed-slot lowering rejection.

Representative status:
- `tests/c/external/gcc_torture/src/20030209-1.c` advances past the old
  `RV64 object route requires a supported prepared stack frame` rejection.
- The current focused probe stops at the existing non-goal diagnostic:
  `unsupported_stack_frame: RV64 object route does not support non-GPR prepared
  callee-saved register save slots (fpr:fs1)`.
- Representative artifact:
  `build/agent_state/509_step3_fixed_frame_validation/representative_20030209-1.log`.

## Suggested Next

Execute Step 4: implement prepared fixed-slot addressing for RV64 object
emission. Keep it narrow around fact-driven large slot offset materialization
above the existing low-level stack load/store/address encoders.

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
- Do not infer frame size, stack offsets, slot widths, slot alignments, or call
  frame shape from `src/20030209-1.c`, `80000`, `10000`, source layout, or raw
  slot counts.
- Step 4 should not weaken low-level load/store immediate encoders. Add or
  reuse a fact-driven address materialization helper above the encoders for
  large fixed-slot offsets.
- Call-frame RA save/restore now has large-offset materialization for the
  frame prologue/epilogue only; saved GPR spill/restore slot support remains at
  the pre-existing boundary.

## Proof

- `scripts/plan_review_state.py set-step --step-id 3 --step-title 'Implement Fixed-Frame Validation And Adjustment'`
- `cmake --build build --target c4cll`
- `cmake --build build --target backend_riscv_object_emission_test`
- `ctest --test-dir build --output-on-failure -R '^backend_riscv_object_emission$'`
- Focused representative probe through
  `tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake` for
  `tests/c/external/gcc_torture/src/20030209-1.c`; expected non-goal
  `fpr:fs1` diagnostic recorded at
  `build/agent_state/509_step3_fixed_frame_validation/representative_20030209-1.log`.
- `ctest --test-dir build --output-on-failure -R '^(backend_riscv_object_emission|backend_cli_riscv64_vrm_insn_d_source_obj|backend_c4c_as_parse_suite|backend_c4c_objdump_extract_suite)$'`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
  passed: 332/332 tests.
- `git diff --check -- src/backend/mir/riscv/codegen/object_emission.cpp tests/backend/mir/backend_riscv_object_emission_test.cpp todo.md`
