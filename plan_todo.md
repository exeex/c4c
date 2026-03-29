# X86 Backend C-Testsuite Convergence Todo

Status: Active
Source Idea: ideas/open/34_backend_x86_c_testsuite_backend_convergence_plan.md
Source Plan: plan.md

## Active Item

- Step 2: Classify the next bounded fallback family after closing the
  single-block local pointer round-trip slice at `src/00004.c`.

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
- Step 2 family reclassified for the next slice: the adjacent pointer-owned
  probes do not share one smallest shippable root cause. `src/00004.c` is a
  bounded single-block local pointer round-trip (`store ptr %lv.x, ptr %lv.p`,
  indirect scalar store through `%t0`, indirect scalar reload through `%t1`)
  while `src/00005.c` adds double indirection plus multi-block control flow.
- Root-cause note for the selected `src/00004.c` slice:
  `src/backend/lir_adapter.cpp` preserved plain scalar local-slot rewrites but
  did not track a local pointer slot that aliases one scalar alloca across
  `store ptr`, `load ptr`, indirect `store i32`, and indirect `load i32`, so
  the bounded round-trip kept falling through to LLVM IR before the existing
  immediate-return x86 emitter path could fire.
- Nearby families explicitly excluded from this slice:
  `src/00005.c` still depends on double-indirection tracking across branches,
  `src/00006.c` to `src/00010.c` remain broader multi-block/control-flow
  families, and `src/00012.c` onward stay outside the selected pointer
  round-trip mechanism.
- Step 3 completed for the selected local pointer round-trip slice by adding
  backend adapter and x86 emitter unit coverage for the bounded `src/00004.c`
  shape while keeping `c_testsuite_x86_backend_src_00004_c` as the external
  seed case.
- Step 4 completed by teaching `src/backend/lir_adapter.cpp` to normalize the
  one-scalar/one-pointer local alias round-trip into a direct return immediate
  instead of falling back on raw pointer loads and stores.
- Step 5 completed for this slice:
  `./build/backend_lir_adapter_tests` passed,
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00004_c|c_testsuite_x86_backend_src_00005_c)$'`
  now passes `src/00004.c` while preserving `src/00005.c` as the only
  remaining targeted fallback, `ctest --test-dir build -L x86_backend
  --output-on-failure` improved from 23 passed / 197 failed / 220 total to
  24 passed / 196 failed / 220 total with no newly introduced failure mode,
  and the broader `ctest --test-dir build -j8 --output-on-failure` post-change
  sweep completed with 2364 passed / 196 failed / 2560 total.

## Next Slice

- Start from `src/00005.c` as the next bounded family: double-indirection local
  pointer tracking with multi-block control flow.
- Confirm whether the first shippable slice is the indirect branch predicate
  loads, the indirect store through `**pp`, or a smaller control-flow rewrite
  that must precede both.
- Preserve `src/00004.c` as the closed single-block local pointer reference
  slice while the `src/00005.c` family is selected.

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
- The local pointer round-trip slice now extends that bounded surface to
  `tests/c/external/c-testsuite/src/00004.c` by collapsing the single-block
  one-scalar/one-pointer alias path into the existing direct return-immediate
  emitter surface; `src/00005.c` remains the next excluded double-indirection
  plus control-flow family.
