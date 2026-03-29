# X86 Backend C-Testsuite Convergence Todo

Status: Active
Source Idea: ideas/open/34_backend_x86_c_testsuite_backend_convergence_plan.md
Source Plan: plan.md

## Active Item

- Step 3: Add the narrowest durable validation for the selected single-block
  integer-subtraction fallback family.

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

## Next Slice

- Add a targeted backend-owned test slice that asserts native x86 assembly for
  the selected subtraction family before changing emitter behavior.
- Start with `src/00002.c` as the narrow failing case and only widen if the
  test harness cannot isolate that shape cleanly.
- After the test is in place, add the smallest `sub i32 imm, imm -> ret`
  support in `src/backend/x86/codegen/emit.cpp`.

## Blockers

- None recorded yet.

## Notes

- Baseline command output was captured in `x86_backend_baseline.log` during this
  iteration for local reference; do not treat it as durable repo state.
- Preserve the legacy `c_testsuite_*` surface as the unchanged baseline while
  promoting backend-owned cases one family at a time.
