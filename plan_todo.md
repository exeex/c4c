# x86 Runtime Case Convergence Todo

Status: Active
Source Idea: ideas/open/31_backend_x86_runtime_case_convergence_plan.md
Source Plan: plan.md

## Active Item

- [ ] Step 3: Promote `two_arg_local_arg` as the next narrow parameter-passing follow-on case

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

## Completed

- [x] Activated `ideas/open/31_backend_x86_runtime_case_convergence_plan.md` into the active runbook.
- [x] Promoted `tests/c/internal/backend_case/call_helper.c` onto the x86 backend-owned asm path with a narrow zero-arg direct-call helper slice.
- [x] Promoted `tests/c/internal/backend_case/param_slot.c` onto the x86 backend-owned asm path with a narrow single-argument modified-parameter helper slice.
- [x] Promoted `tests/c/internal/backend_case/two_arg_helper.c` onto the x86 backend-owned asm path with a narrow register-only two-argument direct-call helper slice.

## Next Intended Slice

- Promote `two_arg_local_arg` next, then widen only to `two_arg_second_local_arg` if the same bounded two-argument direct-call parameter seam still holds.

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
