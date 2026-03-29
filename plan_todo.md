# x86 Runtime Case Convergence Todo

Status: Active
Source Idea: ideas/open/31_backend_x86_runtime_case_convergence_plan.md
Source Plan: plan.md

## Active Item

- [ ] Step 4: Promote `branch_if_eq` as the next bounded compare-and-branch follow-on case, then widen only to the smallest adjacent equality and unsigned compare runtime cases if the same minimal x86 compare-lowering seam still holds

## Todo

- [x] Reproduce the current `call_helper.c` x86 behavior with `BACKEND_OUTPUT_KIND=asm` and capture the exact fallback symptom.
- [x] Localize the bounded backend adapter / routing seam that emits LLVM text instead of backend-owned asm for the helper-call case.
- [x] Add or tighten the narrowest test or assertion that proves the helper-call case must stay on the backend-owned asm path.
- [x] Implement the smallest x86 backend change needed to make `call_helper.c` emit assembly text and pass runtime validation.
- [x] Re-run targeted x86 backend and runtime checks for `call_helper.c`.
- [x] Rebuild and run the full `ctest` suite, then compare against the pre-change baseline.
- [x] Start the next narrow parameter-passing slice only after the helper-call slice is stable.
- [x] Promote `param_slot` onto the x86 backend-owned asm path with a narrow single-argument modified-parameter helper slice.
- [x] Re-run the next bounded parameter slice against `two_arg_helper` before widening to local-arg rewrites.
- [x] Promote `two_arg_local_arg` onto the x86 backend-owned asm path with a narrow first-local/two-argument direct-call slice.
- [x] Promote `two_arg_second_local_arg` onto the x86 backend-owned asm path with the same single-local/two-argument direct-call slice when the normalized load feeds the second call operand.
- [x] Promote `two_arg_second_local_rewrite` onto the x86 backend-owned asm path with the same bounded single-local/two-argument direct-call slice when one local `+ 0` rewrite is preserved before the second call operand.
- [x] Promote `two_arg_both_local_arg` onto the x86 backend-owned asm path with the bounded two-local/two-argument direct-call slice.
- [x] Widen the same bounded two-local direct-call seam to `two_arg_both_local_first_rewrite`, `two_arg_both_local_second_rewrite`, and `two_arg_both_local_double_rewrite`.
- [x] Re-run focused backend and runtime checks for the bounded both-local family, then compare a full-suite rerun against the previous runbook baseline.

## Completed

- [x] Activated `ideas/open/31_backend_x86_runtime_case_convergence_plan.md` into the active runbook.
- [x] Promoted `tests/c/internal/backend_case/call_helper.c` onto the x86 backend-owned asm path with a narrow zero-arg direct-call helper slice.
- [x] Promoted `tests/c/internal/backend_case/param_slot.c` onto the x86 backend-owned asm path with a narrow single-argument modified-parameter helper slice.
- [x] Promoted `tests/c/internal/backend_case/two_arg_helper.c` onto the x86 backend-owned asm path with a narrow register-only two-argument direct-call helper slice.
- [x] Promoted `tests/c/internal/backend_case/two_arg_local_arg.c` onto the x86 backend-owned asm path with a narrow first-local/two-argument direct-call helper slice.
- [x] Promoted `tests/c/internal/backend_case/two_arg_second_local_arg.c` onto the x86 backend-owned asm path with the same single-local/two-argument direct-call helper slice when the local load is the second argument.
- [x] Promoted `tests/c/internal/backend_case/two_arg_second_local_rewrite.c` onto the x86 backend-owned asm path with the same bounded single-local `+ 0` rewrite feeding the second helper-call argument.
- [x] Promoted `tests/c/internal/backend_case/two_arg_both_local_arg.c`, `two_arg_both_local_first_rewrite.c`, `two_arg_both_local_second_rewrite.c`, and `two_arg_both_local_double_rewrite.c` onto the x86 backend-owned asm path with the same bounded two-local/two-argument direct-call helper slice.

## Next Intended Slice

- Promote `branch_if_eq` next, then widen only to the smallest adjacent equality and unsigned compare runtime cases if the same bounded x86 compare-lowering seam still holds.

## Blockers

- None recorded at activation time.

## Resume Notes

- The umbrella roadmap in [ideas/open/__backend_port_plan.md](/workspaces/c4c/ideas/open/__backend_port_plan.md) explicitly says not to implement directly from the umbrella and names this child idea as the next x86 activation target.
- The first execution slice should stay tightly bounded to helper-call routing and must not widen into broad ABI or linker work.
- Reproduced failure: `backend_runtime_call_helper` feeds `build/internal_backend/call_helper.ll` to Clang as assembler and fails immediately because the x86 backend falls through to LLVM text for the zero-arg direct helper-call shape.
- Implemented fix: `src/backend/x86/codegen/emit.cpp` now recognizes the adapted two-function zero-arg direct-call shape and emits a backend-owned helper body plus `call helper` main stub instead of printing LLVM text.
- Param-slot seam: `src/backend/x86/codegen/emit.cpp` now recognizes the bounded two-function single-argument modified-parameter helper shape and emits backend-owned x86 asm for the helper plus direct-call stub instead of falling back to LLVM text.
- Param-slot validation: `./build/backend_lir_adapter_tests` passed, `ctest -R 'backend_runtime_(call_helper|param_slot)$|^backend_lir_adapter_tests$'` passed, and full `ctest` now reports `17` failures out of `2339` tests, down from the previously recorded `18`, with `backend_runtime_param_slot` now green.
- Two-arg-helper seam: `src/backend/x86/codegen/emit.cpp` now recognizes the bounded two-function register-only `add_pair(i32 %p.x, i32 %p.y)` plus direct `main` call shape and emits backend-owned x86 asm using `edi`/`esi` for the helper call instead of falling back to LLVM text.
- Two-arg-helper validation: `ctest -R '^backend_runtime_two_arg_helper$|^backend_lir_adapter_tests$' --output-on-failure` passed, and the full suite improved from `17` failures out of `2339` tests in `test_before.log` to `16` failures out of `2339` tests in `test_after.log`, with `backend_runtime_two_arg_helper` resolved and no new failing tests according to the regression guard.
- Two-arg-local-arg seam: `src/backend/x86/codegen/emit.cpp` now accepts the bounded `main` shape that stores one immediate into a single local slot, reloads it, and passes that value as the first `add_pair(i32, i32)` argument while keeping the second argument immediate on the existing minimal two-register helper path.
- Two-arg-local-arg validation: `ctest -R '^backend_runtime_two_arg_(helper|local_arg)$|^backend_lir_adapter_tests$' --output-on-failure` passed, and the full suite improved from `19` failures out of `2339` tests in `test_fail_before.log` to `15` failures out of `2339` tests in `test_fail_after.log`, resolving `backend_runtime_call_helper`, `backend_runtime_param_slot`, `backend_runtime_two_arg_helper`, and `backend_runtime_two_arg_local_arg` with no new failing tests according to the regression guard.
- Two-arg-second-local-arg seam: `src/backend/x86/codegen/emit.cpp` now accepts the same bounded single-local store/load pattern when the normalized loaded value feeds the second `add_pair(i32, i32)` call operand, keeping the first operand immediate on the existing minimal two-register helper path.
- Two-arg-second-local-arg validation: `ctest -R '^backend_runtime_two_arg_(helper|local_arg|second_local_arg)$|^backend_lir_adapter_tests$' --output-on-failure` passed. The patched main workspace full suite now reports `14` failures out of `2339` tests in `test_fail_after.log`, resolving `backend_runtime_two_arg_second_local_arg`; the regression guard also reported no newly failing tests versus the clean baseline log.
- Two-arg-second-local-rewrite seam: `src/backend/x86/codegen/emit.cpp` now accepts the same single-local `store -> load -> add 0 -> store -> load -> call` shape, preserving one bounded local rewrite before the normalized second `add_pair(i32, i32)` call operand while keeping the first operand immediate on the existing minimal two-register helper path.
- Two-arg-second-local-rewrite validation: `./build/backend_lir_adapter_tests` passed, `ctest -R '^backend_runtime_two_arg_(helper|local_arg|second_local_arg|second_local_rewrite)$|^backend_lir_adapter_tests$' --output-on-failure` passed, `backend_runtime_two_arg_first_local_rewrite` also passed on the same bounded matcher, and the full suite now reports `12` failures out of `2339` tests in `test_fail_after.log`, down from the previously recorded `14` failures in this runbook state. No local `test_fail_before.log` artifact was present to rerun the scripted regression guard this turn.
- Two-local seam: `src/backend/x86/codegen/emit.cpp` now accepts the bounded two-slot caller shape where `main` stores immediates into `%lv.x` and `%lv.y`, then feeds `add_pair(i32, i32)` through either direct loads or one preserved `+ 0` rewrite per slot before the helper call.
- Two-local validation: `ctest -R '^backend_lir_adapter_tests$|^backend_runtime_two_arg_both_local_(arg|first_rewrite|second_rewrite|double_rewrite)$' --output-on-failure` passed, and the full-suite rerun improved from `12` failures out of `2339` tests in `test_fail_before.log` to `8` failures out of `2339` tests in `test_fail_after.log`. `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log` passed with the four both-local runtime failures resolved and no new failing tests.
- Next blocker boundary: the remaining planned Step 4 follow-ons are the compare-and-branch runtime cases first (`branch_if_eq`, `branch_if_ne`, `branch_if_uge`, `branch_if_ugt`, `branch_if_ule`, `branch_if_ult`), followed later by adjacent local-slot/local-address and then global-addressing slices.
