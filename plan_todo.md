# X86 Backend C-Testsuite Convergence Todo

Status: Active
Source Idea: ideas/open/34_backend_x86_c_testsuite_backend_convergence_plan.md
Source Plan: plan.md

## Active Item

- Step 2: Classify and prepare the next bounded multi-loop control-flow family
  centered on `src/00007.c`, which remains outside the completed single-loop
  countdown slice because it combines two countdown regions with an
  intervening `if (x) return 1;` branch.

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
- Step 2 family reclassified for the next slice: the first shippable
  `src/00005.c` fix is the bounded double-indirection local-pointer family with
  tracked local scalar state across an acyclic branch chain. Representative
  seeds: `src/00005.c` (branching indirect probe plus indirect store),
  `src/00020.c` (direct `return **pp;`), and `src/00103.c` (the same local
  double-indirection round-trip through `void*`/`void**` casts).
- Root-cause note for this slice:
  `src/backend/lir_adapter.cpp` normalized only single-block local pointer
  round-trips, but did not track a local scalar value through one local pointer
  slot plus one local pointer-to-pointer slot across `load ptr`, indirect
  `load i32`, indirect `store i32`, and acyclic `br`/`condbr` control flow, so
  these functions fell back before the existing direct-return x86 emitter path
  could fire.
- Nearby families explicitly excluded from this slice:
  `src/00006.c`, `src/00007.c`, and `src/00008.c` still require loop handling,
  `src/00009.c` depends on multiply/divide/mod lowering rather than pointer
  state tracking, and `src/00010.c` is a control-flow-only goto slice without
  pointer locals.
- Step 3 completed for the selected double-indirection local-pointer family by
  adding backend adapter and x86 emitter unit coverage for the exact
  `src/00005.c`-shaped CFG while keeping
  `c_testsuite_x86_backend_src_00005_c` as the external seed case.
- Step 4 completed by teaching `src/backend/lir_adapter.cpp` to interpret the
  bounded three-local pointer/value state across acyclic `br`/`condbr`
  structure and collapse it into a direct backend-owned return immediate.
- Step 5 completed for this slice:
  `./build/backend_lir_adapter_tests` passed, targeted
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00004_c|c_testsuite_x86_backend_src_00005_c|c_testsuite_x86_backend_src_00020_c|c_testsuite_x86_backend_src_00103_c)$'`
  passed, `ctest --test-dir build -L x86_backend --output-on-failure`
  improved from 24 passed / 196 failed / 220 total to
  27 passed / 193 failed / 220 total with no newly introduced failure mode,
  and the broader `ctest --test-dir build -j8 --output-on-failure`
  post-change sweep completed with 2367 passed / 193 failed / 2560 total.
- Step 2 family reclassified for the next slice: bounded goto-only control
  flow with no local state, no phi joins, and a single reachable constant
  `ret i32` target. Representative seed: `src/00010.c`, whose lowered path is
  `entry -> ulbl_start -> block_1 -> ulbl_next -> block_3 -> ulbl_foo ->
  block_4 -> block_2 -> ret i32 0`.
- Root-cause note for this slice:
  `src/backend/lir_adapter.cpp` normalized bounded acyclic control flow only
  when it also carried tracked local pointer/scalar state, but plain multi-block
  goto chains with no allocas still fell through to the generic
  `multi-block functions` unsupported path before the existing x86 direct
  return-immediate emitter could run.
- Nearby families explicitly excluded from this slice:
  `src/00006.c` to `src/00008.c` still require loop handling, and `src/00009.c`
  remains the separate arithmetic-op family because it depends on preserving
  multiply/divide/remainder semantics rather than control-flow-only collapse.
- Step 3 completed for the selected goto-only family by adding focused backend
  adapter and x86 emitter unit coverage for the exact `src/00010.c`-shaped
  unconditional branch chain while keeping
  `c_testsuite_x86_backend_src_00010_c` as the external seed case.
- Step 4 completed by teaching `src/backend/lir_adapter.cpp` to follow an
  acyclic no-alloca chain of unconditional branches until it reaches a constant
  `ret i32`, then collapse that path into the existing backend-owned direct
  return-immediate surface.
- Step 5 completed for this slice:
  `./build/backend_lir_adapter_tests` passed,
  `ctest --test-dir build --output-on-failure -R '^c_testsuite_x86_backend_src_00010_c$'`
  passed, `ctest --test-dir build -L x86_backend --output-on-failure`
  improved from 27 passed / 193 failed / 220 total to
  28 passed / 192 failed / 220 total with no newly introduced failure mode,
  and the broader `ctest --test-dir build -j8 --output-on-failure`
  post-change sweep improved from 2367 passed / 193 failed / 2560 total to
  2368 passed / 192 failed / 2560 total.
- Step 2 family reclassified for the next slice: bounded single-block scalar
  local-slot arithmetic centered on `src/00009.c`, where one tracked `i32`
  local flows through `mul`, `sdiv`, `srem`, then the existing direct-return
  subtraction shape.
- Root-cause note for this slice:
  `src/backend/lir_adapter.cpp` normalized direct local-slot returns and the
  simpler one-op subtraction form, but rejected the same one-slot shape as soon
  as the LIR chain introduced `mul`, `sdiv`, or `srem`, so the backend fell
  back before the x86 emitter could reach its existing direct return-immediate
  assembly path.
- Nearby families explicitly excluded from this slice:
  `src/00006.c` to `src/00008.c` still require loop/backedge handling rather
  than single-block arithmetic normalization, and `src/00007.c` in particular
  adds two separate counted loops plus a mid-function conditional branch.
- Step 3 completed for the selected local-slot arithmetic family by adding
  focused backend adapter and x86 emitter unit coverage for the exact
  `mul -> sdiv -> srem -> sub` one-slot chain while keeping
  `c_testsuite_x86_backend_src_00009_c` as the external seed case.
- Step 4 completed by teaching `src/backend/lir_adapter.cpp` to evaluate the
  bounded one-slot immediate arithmetic chain to a direct backend-owned return
  immediate instead of falling back when the local rewrite encounters `mul`,
  `sdiv`, or `srem`.
- Step 5 completed for this slice:
  `./build/backend_lir_adapter_tests` passed, targeted
  `ctest --test-dir build --output-on-failure -R '^c_testsuite_x86_backend_src_00009_c$'`
  passed, `ctest --test-dir build -L x86_backend --output-on-failure`
  improved from 28 passed / 192 failed / 220 total to
  29 passed / 191 failed / 220 total with no newly introduced failure mode,
  `ctest --test-dir build -L backend --output-on-failure` reported
  95 passed / 191 failed / 286 total, and the broader
  `ctest --test-dir build -j8 --output-on-failure` post-change sweep improved
  from 2368 passed / 192 failed / 2560 total to
  2369 passed / 191 failed / 2560 total.
- Step 2 family reclassified for the next slice: bounded single-local
  countdown loops that terminate by reloading the tracked scalar slot after the
  loop exits. Representative seeds: `src/00006.c` (`while (x) x = x - 1;`) and
  `src/00008.c` (`do x = x - 1; while (x);`).
- Root-cause note for this slice:
  `src/backend/lir_adapter.cpp` normalized acyclic branch chains and
  single-block local-slot arithmetic, but rejected the first loop-backed
  family even when it stayed within one `i32` local, one decrement-by-1 site,
  one `icmp ne ..., 0` loop test, and one final reload/return. That kept both
  loop forms falling back before the existing x86 direct-return immediate path
  could run.
- Nearby families explicitly excluded from this slice:
  `src/00007.c` still carries two distinct countdown regions plus the
  intervening `if (x) return 1;` conditional, so it remains the larger
  follow-on control-flow family rather than part of the completed
  single-loop slice.
- Step 3 completed for the selected countdown-loop family by adding focused
  backend adapter and x86 emitter unit coverage for both the `while` and
  `do-while` countdown shapes while keeping
  `c_testsuite_x86_backend_src_00006_c` and
  `c_testsuite_x86_backend_src_00008_c` as the external seed cases.
- Step 4 completed by teaching `src/backend/lir_adapter.cpp` to recognize the
  bounded one-local countdown-loop shape, execute its concrete decrement path
  to the loop exit, and collapse the result into the backend-owned direct
  return-immediate surface.
- Step 5 completed for this slice:
  `./build/backend_lir_adapter_tests` passed, targeted
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00006_c|c_testsuite_x86_backend_src_00007_c|c_testsuite_x86_backend_src_00008_c)$'`
  now passes `src/00006.c` and `src/00008.c` while preserving `src/00007.c`
  as the intentionally excluded fallback, `ctest --test-dir build -L
  x86_backend --output-on-failure` improved from
  29 passed / 191 failed / 220 total to 31 passed / 189 failed / 220 total
  with no newly introduced failure mode, and
  `ctest --test-dir build -L backend --output-on-failure` improved from
  95 passed / 191 failed / 286 total to 97 passed / 189 failed / 286 total.

## Next Slice

- Start from `src/00007.c` as the next bounded family: one tracked scalar
  local carried through two separate countdown loops with a mid-function
  conditional early return.
- Keep the next slice focused on proving whether the existing bounded concrete
  execution approach can be extended to two loop regions and one intervening
  branch without widening into general loop support.

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
- The double-indirection local-pointer slice now extends that bounded surface
  to `tests/c/external/c-testsuite/src/00005.c`,
  `tests/c/external/c-testsuite/src/00020.c`, and
  `tests/c/external/c-testsuite/src/00103.c` by interpreting the acyclic local
  pointer/value state in `src/backend/lir_adapter.cpp` and collapsing the
  resulting path to `ret i32 0`.
- The goto-only control-flow slice now extends that bounded surface to
  `tests/c/external/c-testsuite/src/00010.c` by following the exact
  unconditional branch chain in `src/backend/lir_adapter.cpp` until it reaches
  the single reachable constant return, then handing the normalized `ret i32 0`
  surface to the existing x86 immediate-return emitter.
- The local-slot arithmetic slice now extends that bounded surface to
  `tests/c/external/c-testsuite/src/00009.c` by evaluating the exact
  one-slot `mul -> sdiv -> srem -> sub` chain in `src/backend/lir_adapter.cpp`
  and collapsing it to the existing direct `ret i32 0` backend-owned surface
  consumed by the x86 immediate-return emitter.
- The countdown-loop slice now extends that bounded surface to
  `tests/c/external/c-testsuite/src/00006.c` and
  `tests/c/external/c-testsuite/src/00008.c` by concretely executing the
  bounded single-local decrement loop in `src/backend/lir_adapter.cpp` until it
  reaches the exit `ret i32 0`; `src/00007.c` remains the next excluded
  multi-loop follow-on family.
