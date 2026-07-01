Status: Active
Source Idea Path: ideas/open/508_rv64_prepared_callee_saved_gpr_save_slot_emission.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Ordinary-C Coverage

# Current Packet

## Just Finished

Completed Step 4 of `plan.md`: added focused ordinary-C RV64 object-runtime
coverage for prepared callee-saved GPR save/restore emission.

Changed files:

- `tests/backend/CMakeLists.txt`
- `tests/backend/case/riscv64_callee_saved_gpr_live_across_call.c`
- `todo.md`
- `test_after.log`

Coverage rationale:

- `riscv64_callee_saved_gpr_live_across_call.c` keeps an integer value live
  across a same-module call while the callee also uses prepared callee-saved
  GPR state. The runtime result depends on the caller's live value surviving
  the call, so the object route must honor the prepared GPR save/restore path.
- The focused CTest registration uses the existing
  `c4c_add_backend_rv64_object_runtime_case` helper, compiling through
  `--codegen obj`, linking the emitted object with clang, and running it under
  qemu.
- Prepared dump probing showed `saved_register bank=gpr` facts for
  `perturb_value`, `keep_value_across_call`, and `main`, including multiple
  callee-saved placements in the cross-call function. The test does not assert
  raw slot numbers, frame shape, function-name shortcuts, expectations,
  unsupported markers, allowlists, runtime accounting, or scan accounting.

Focused test result:

- `backend_obj_runtime_rv64_callee_saved_gpr_live_across_call` passed.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed
  `332/332`, including the new object-runtime case.

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

Execute Step 5: validate and hand off the completed GPR save-slot slice for
supervisor review, preserving the current FPR blocker as an out-of-scope
follow-up rather than weakening the representative.

## Watchouts

- The representative has advanced to the explicit FPR fail-closed diagnostic,
  which is expected for this packet. Do not weaken that diagnostic or skip FPR
  facts to claim object-route success.
- The new ordinary-C case is GPR-only. Do not extend this plan into FPR, F128,
  vector, dynamic-stack, or broad fixed-frame support.

## Proof

- `scripts/plan_review_state.py set-step --step-id 4 --step-title 'Add Ordinary-C Coverage'`
- `cmake --build build --target c4cll` passed.
- `ctest --test-dir build --output-on-failure -R '^backend_obj_runtime_rv64_callee_saved_gpr_live_across_call$'` passed.
- Representative object-route probe for
  `tests/c/external/gcc_torture/src/20000603-1.c` returned `2` with the
  expected FPR fail-closed diagnostic above; artifacts are under
  `build/agent_state/508_step4_ordinary_c_coverage/`.
- `git diff --check -- todo.md tests/backend/CMakeLists.txt tests/backend/case/riscv64_callee_saved_gpr_live_across_call.c` passed after the final `todo.md` update.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed
  `332/332`.
- Proof log: `test_after.log`.
