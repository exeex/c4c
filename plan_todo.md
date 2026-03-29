# X86 Backend C-Testsuite Convergence Todo

Status: Active
Source Idea: ideas/open/34_backend_x86_c_testsuite_backend_convergence_plan.md
Source Plan: plan.md

## Active Item

- Step 2: Classify the next bounded fallback family after closing the
  scalar local-slot/store slice around `src/00003.c` and `src/00011.c`.

## Completed Items

- Activated the source idea into `plan.md`.
- Initialized `plan_todo.md` for the active plan.
- Step 1 baseline captured with `ctest --test-dir build -L x86_backend
  --output-on-failure`: 20 passed, 200 failed, 220 total. Every sampled
  failure reported `[BACKEND_FALLBACK_IR]`.
- Representative current passes:
  `src/00001.c` (`return 0;`), `src/00022.c` (local initialized return),
  `src/00059.c` (minimal conditional return), `src/00067.c` and `src/00068.c`
  (preprocessor-driven scalar global return).
- Step 2 family selected: minimal single-function `i32 sub` return slices that
  already lower to backend IR but miss an x86 emitter fast path. Representative
  seed case: `src/00002.c`, which emits:
  `%t0 = sub i32 3, 3` then `ret i32 %t0`.
- Root-cause note: `src/backend/x86/codegen/emit.cpp` has
  `parse_minimal_return_imm` and `parse_minimal_return_add_imm`, but no
  corresponding minimal subtraction matcher or emitter hook, so these cases
  fall through to printed LLVM IR.
- Nearby families explicitly excluded from this slice:
  `src/00003.c` and `src/00011.c` depend on local-slot/store lowering,
  `src/00004.c` and `src/00005.c` depend on pointer load/store support,
  `src/00006.c` to `src/00010.c` introduce loops or multi-block control flow,
  `src/00063.c` depends on scalar global-object emission,
  `src/00100.c` depends on direct-call lowering.
- Step 3 completed for the selected subtraction family by adding focused backend
  unit coverage for the `sub i32 imm, imm -> ret` slice and by keeping
  `c_testsuite_x86_backend_src_00002_c` as the external seed case.
- Step 4 completed by extending the backend adapter to preserve `sub` in the
  backend-owned binary-op surface and by teaching
  `src/backend/x86/codegen/emit.cpp` to fold the single-block subtraction slice
  into native x86 assembly.
- Step 5 completed for this slice:
  `./build/backend_lir_adapter_tests` passed,
  `ctest --test-dir build --output-on-failure -R '^c_testsuite_x86_backend_src_00002_c$'`
  passed, and `ctest --test-dir build -L x86_backend --output-on-failure`
  improved from 20 passed / 200 failed / 220 total to
  21 passed / 199 failed / 220 total with no newly introduced failure mode.
- Step 2 family reclassified for the next slice: bounded single-block scalar
  local-slot/store cases that only need immediate store tracking before the
  existing subtraction/direct-return emitter paths. Representative seeds:
  `src/00003.c` (`alloca i32`, `store i32 4`, `load`, `sub`, `ret`) and
  `src/00011.c` (two `alloca i32` locals, zero stores, reload of `x`, `ret`).
- Root-cause note for this slice:
  `src/backend/lir_adapter.cpp` normalized only the one-slot direct-return
  local-temp form, but did not preserve the closely related local-slot
  subtraction shape or the two-local scalar store/reload return shape, so both
  cases fell back before the x86 emitter reached its existing direct
  subtraction/immediate return support.
- Nearby families explicitly excluded from this slice:
  `src/00004.c` and `src/00005.c` still depend on pointer-address materializing
  plus indirect load/store lowering rather than plain scalar local-slot value
  tracking.
- Step 3 completed for the selected scalar local-slot/store family by adding
  backend adapter and x86 emitter unit coverage for the local-slot subtraction
  and bounded two-local direct-return forms while keeping
  `c_testsuite_x86_backend_src_00003_c` and
  `c_testsuite_x86_backend_src_00011_c` as the external seed cases.
- Step 4 completed by teaching `src/backend/lir_adapter.cpp` to normalize the
  local-slot subtraction and two-local scalar return slices into the backend's
  existing direct subtraction/immediate return surface instead of falling back
  on raw entry allocas.
- Step 5 completed for this slice:
  `./build/backend_lir_adapter_tests` passed, targeted
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00003_c|c_testsuite_x86_backend_src_00011_c)$'`
  passed, the exclusion check confirmed `src/00004.c` and `src/00005.c` still
  fail only with `[BACKEND_FALLBACK_IR]`, and
  `ctest --test-dir build -L x86_backend --output-on-failure` improved from
  21 passed / 199 failed / 220 total to 23 passed / 197 failed / 220 total
  with no newly introduced failure mode.

## Next Slice

- Reclassify the remaining smallest repeatable fallback family from the updated
  `x86_backend` set, starting with the now-adjacent pointer-indirection cases
  around `src/00004.c` and `src/00005.c`.
- Confirm whether those pointer-owned cases share one root cause in address
  materialization plus indirect load/store lowering, or whether `src/00005.c`
  needs to split again because of double indirection and control flow.
- Preserve `src/00003.c` and `src/00011.c` as the closed scalar local-slot
  reference slice while the pointer family is selected.

## Blockers

- None recorded yet.

## Notes

- Baseline command output was captured in `x86_backend_baseline.log` during this
  iteration for local reference; do not treat it as durable repo state.
- Preserve the legacy `c_testsuite_*` surface as the unchanged baseline while
  promoting backend-owned cases one family at a time.
- The subtraction slice now emits:
  `.intel_syntax noprefix`, `.text`, `.globl main`, `mov eax, 0`, `ret`
  for `tests/c/external/c-testsuite/src/00002.c` on
  `x86_64-unknown-linux-gnu`.
- The scalar local-slot/store slice now extends that bounded surface to
  `tests/c/external/c-testsuite/src/00003.c` and
  `tests/c/external/c-testsuite/src/00011.c` without changing the still-failing
  pointer-indirection probes `src/00004.c` and `src/00005.c`.
