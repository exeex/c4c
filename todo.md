Status: Active
Source Idea Path: ideas/open/508_rv64_prepared_callee_saved_gpr_save_slot_emission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement GPR Save-Slot Emission

# Current Packet

## Just Finished

Completed Step 3 of `plan.md`: tightened RV64 object emission so prepared
callee-saved GPR slots are still consumed by the existing strict
`rv64_saved_callee_gpr_stack_offset` validator, and added a preflight
diagnostic that rejects non-GPR saved-register facts before a function can be
accepted.

Changed files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`
- `test_after.log`

Implementation rule:

- Coherent one-width RV64 GPR save slots continue to lower through the prepared
  slot authority checks for bank, register name, placement, slot placement,
  fixed stack offset, size `8`, in-frame addressability, and signed-12-bit
  offset.
- Non-GPR saved-register entries, placements, or slot placements now fail
  closed through `diagnose_unsupported_prepared_saved_register_bank` before
  prologue/epilogue emission. This prevents accepting a function by silently
  skipping an out-of-scope FPR/vector saved-register fact.
- No expectation, allowlist, unsupported marker, runtime accounting, FPR,
  F128, vector, dynamic-stack, or broad fixed-frame support was added.

Focused test result:

- `builds_prepared_prior_preserved_arg_call_object` still proves the clean GPR
  path emits `sd s1, 0(sp)` and restores `s1` before `ra`.
- `rejects_prepared_prior_preserved_arg_call_fail_closed_shapes` now also
  mutates the saved-register fact to `fpr:fs1` and expects the explicit
  non-GPR prepared callee-saved save-slot diagnostic.

Representative status:

- `tests/c/external/gcc_torture/src/20000603-1.c` object route now advances
  past the old `requires supported prepared callee-saved GPR save slots`
  diagnostic and fails closed on the out-of-scope `fpr:fs1` saved-register
  fact:
  `unsupported_stack_frame: RV64 object route does not support non-GPR prepared
  callee-saved register save slots (fpr:fs1)`.
- This is intentionally not marked fixed because FPR save-slot support is a
  non-goal for this idea.

## Suggested Next

Execute Step 4: add ordinary-C coverage for a nearby GPR-only callee-saved
save/restore path, or route the remaining representative FPR save-slot blocker
to a separate follow-up if no GPR-only ordinary-C case is available without
overfitting.

## Watchouts

- `main` remains the clean representative for accepting a GPR-only call-frame
  save and restore. `f` is not safe to fully accept until an FPR save/restore
  route exists.
- The representative has advanced to the explicit FPR fail-closed diagnostic,
  which is expected for this packet. Do not weaken that diagnostic or skip FPR
  facts to claim object-route success.
- Do not infer save slots from testcase names, raw slot numbers, source shape,
  or register spellings.
- Do not include FPR, F128, vector, dynamic-stack, or broad fixed-frame support
  in this plan.

## Proof

- `scripts/plan_review_state.py set-step --step-id 3 --step-title 'Implement GPR Save-Slot Emission'`
- `cmake --build build --target c4cll` passed.
- `cmake --build build --target backend_riscv_object_emission_test` passed.
- `ctest --test-dir build --output-on-failure -R '^backend_riscv_object_emission$'` passed.
- Representative object-route probe for
  `tests/c/external/gcc_torture/src/20000603-1.c` returned `2` with the
  expected FPR fail-closed diagnostic above; artifacts are under
  `build/agent_state/508_step3_gpr_save_slot_emission/`.
- `git diff --check -- todo.md src/backend/mir/riscv/codegen/object_emission.cpp tests/backend/mir/backend_riscv_object_emission_test.cpp` passed after the final `todo.md` update.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed
  `331/331`.
- Proof log: `test_after.log`.
