Status: Active
Source Idea Path: ideas/open/508_rv64_prepared_callee_saved_gpr_save_slot_emission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace RV64 Save-Slot Consumption

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: traced prepared saved-register facts into the
RV64 object-emission prologue and epilogue paths and defined the narrow Step 3
implementation boundary.

Trace artifacts are under
`build/agent_state/508_step2_save_slot_trace/`.

Producer fact path:

- `src/backend/prealloc/frame_plan.cpp` builds `PreparedSavedRegister` entries
  in `populate_frame_plan` from regalloc callee-saved assignments, sorts them,
  and assigns complete slot placements with
  `make_saved_register_slot_placement`.
- The representative prepared dump still has coherent GPR facts:
  `f` saves `gpr:s1` at `slot#10+stack16`, and `main` saves `gpr:s1` at
  `slot#10+stack8`.
- `f` also has `fpr:fs1` at `slot#11+stack24`; that is outside this idea's
  GPR scope but must not be silently ignored at final function acceptance.

RV64 consumer path:

- `emit_prepared_function` in
  `src/backend/mir/riscv/codegen/object_emission.cpp` computes
  `stack_frame_bytes` through `rv64_object_stack_frame_size`, then selects
  `make_rv64_call_frame_prologue_fragment` for functions with calls or
  `make_rv64_stack_frame_prologue_fragment` for non-call stack frames.
- Those prologue helpers call `append_rv64_saved_callee_gpr_spills`, which
  validates each saved-register entry through
  `rv64_saved_callee_gpr_stack_offset` and emits an 8-byte stack store with
  `append_rv64_store_register_to_stack`.
- Return lowering routes through `fragment_for_prepared_return`; all return
  forms call either `append_rv64_call_frame_epilogue` or
  `append_rv64_stack_frame_epilogue`, which call
  `append_rv64_saved_callee_gpr_restores` in reverse save order and emit
  8-byte stack loads with `append_rv64_load_stack_to_register`.
- `prepared_saved_register_stack_end` in
  `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp` already includes all
  saved registers when sizing text-route frames; object-route
  `rv64_object_stack_frame_size` similarly expands the frame over all prepared
  saved-register slot extents before alignment.

Narrow helper boundary for Step 3:

- Keep `rv64_saved_callee_gpr_stack_offset` as the strict authority check for
  one-width RV64 callee-saved GPR facts: bank `Gpr`, real RV64 callee-saved
  register, matching placement and slot placement, complete fixed slot
  metadata, size `8`, in-frame offset, and signed-12-bit stack offset.
- Split iteration from validation if needed: GPR spill/restore helpers may
  emit only validated GPR entries, but object emission needs an explicit
  preflight that rejects any non-GPR saved-register entry before a whole
  function is accepted. Do not let the GPR helper skip `fpr:fs1` and then
  accept representative `f`; that would hide an ABI preservation hole.
- Keep incomplete, mismatched, non-callee-saved, multi-register, non-8-byte,
  out-of-frame, out-of-immediate-range, dynamic-stack, FPR, vector, and F128
  saved-register facts fail-closed.

Existing focused tests:

- `make_prepared_prior_preserved_arg_call_module` already creates a call-frame
  object fixture with a coherent saved `gpr:s1` slot.
- `builds_prepared_prior_preserved_arg_call_object` already checks the prologue
  `sd s1, 0(sp)` shape and epilogue `ld s1, 0(sp)` before `ld ra, 24(sp)`.
- `rejects_prepared_prior_preserved_arg_call_fail_closed_shapes` already
  mutates the saved-slot size to prove malformed GPR facts fail closed.
- Step 3 should extend this area with an explicit non-GPR/FPR saved-register
  rejection and, if practical, a no-call stack-frame fixture so both
  `make_rv64_stack_frame_prologue_fragment` and call-frame prologue/epilogue
  paths are covered.

## Suggested Next

Execute Step 3: implement RV64 object-emission GPR save-slot consumption behind
the boundary above, add/update focused object-emission tests for GPR
prologue/epilogue shape and non-GPR fail-closed behavior, and keep the
representative `f` fail-closed if its out-of-scope `fpr:fs1` save remains
unsupported.

## Watchouts

- `main` is the clean representative for accepting a GPR-only call-frame save
  and restore. `f` is not safe to fully accept until an FPR save/restore route
  exists or a preflight rejects that non-GPR fact with a clearer diagnostic.
- The existing generic diagnostic conflates malformed GPR facts with non-GPR
  saved-register facts. Step 3 may preserve the old text for compatibility, but
  a more specific fail-closed diagnostic would make the boundary easier to
  test.
- Do not infer save slots from testcase names, raw slot numbers, source shape,
  or register spellings.
- Do not include FPR, F128, vector, dynamic-stack, or broad fixed-frame support
  in this plan.

## Proof

- `scripts/plan_review_state.py set-step --step-id 2 --step-title 'Trace RV64 Save-Slot Consumption'`
- Source trace used `rg` plus excerpts from
  `src/backend/prealloc/frame_plan.cpp`,
  `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`,
  `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`, and
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`.
- Trace excerpts were saved under
  `build/agent_state/508_step2_save_slot_trace/`.
- `git diff --check -- todo.md` passed.
- Proof log: `test_after.log`.
