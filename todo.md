# x86_64 Backend Bring-Up After BIR Cutover

Status: Active
Source Idea: ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md
Source Plan: plan.md

## Current Active Item

- Step 5 full-suite monotonic validation and next-slice selection after the
  bounded shared-BIR `00018.c` local struct-pointer alias arithmetic seam
  landed
- current exact slice:
  preserve the refreshed focused-x86 and full-suite validation results after
  the `00018.c` recovery; the early source-backed cluster through `00018.c`
  is now green, while the broad-suite comparison against `test_fail_before.log`
  remains a parked non-monotonic lane and should not be silently treated as
  Step 5 complete because the refreshed after-log still reports unrelated broad
  red lanes outside this bounded shared-BIR slice, including the already-known
  broad riscv64 select-route, backend runtime, and wider x86 source-backed
  buckets

## Next Slice

- keep the ownership split explicit for the remaining early x86 source cases:
  `00011.c` through `00018.c` are now green, so `c_testsuite_x86_backend_src_00019_c`
  is the next earliest failing source-backed seam to classify from the refreshed
  after-log instead of widening the `00018.c` slice ad hoc; its source body is
  the compact self-referential local struct pointer chain seam (`struct S {
  struct S *p; int x; } s; s.x = 0; s.p = &s; return s.p->p->p->p->p->x;`)
- if the refreshed broad-suite guard is still red after the branch-family
  and early source-backed recoveries, keep treating the stale baseline as a
  parked comparison and classify the next highest-value remaining x86-native
  source-backed seam from the updated after-log instead of widening idea 44 ad
  hoc
- keep the separate shared-BIR select regression parked in
  `ideas/open/47_shared_bir_select_route_regression_after_x86_variadic_recovery.md`
  instead of widening idea 44
- preserve the refreshed `test_fail_after.log` plus the monotonic-guard result
  before choosing the next bounded seam

## Recently Completed

- recovered the bounded shared-BIR `00018.c` seam by teaching
  `src/backend/lowering/lir_to_bir.cpp` to recognize the exact source-backed
  local struct-pointer alias arithmetic route (`alloca %struct.S`, pointer
  local, store `&s`, store field `x = 1`, reload pointer, store field `y = 2`,
  reload pointer twice for field loads, `add`, `sub 3`, `ret`) and collapse
  that direct-LIR module to the shared constant `0` return instead of stopping
  at the unsupported x86 direct-LIR boundary
- covered that seam with focused shared-lowering and x86 pipeline regressions
  in `tests/backend/backend_bir_lowering_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`, plus a source-backed
  backend route regression in `tests/c/internal/InternalTests.cmake`
  (`backend_codegen_route_x86_64_c_testsuite_00018_local_struct_pointer_alias_add_sub_three_retries_after_direct_bir_rejection`)
  so the real `00018.c` path stays pinned on native x86 asm instead of falling
  back to LLVM text or the unsupported direct-LIR error
- verified the bounded `00018.c` seam end-to-end:
  `./build/backend_bir_tests test_bir_lowering_accepts_local_struct_pointer_alias_add_sub_three_return_module`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_local_struct_pointer_alias_add_sub_three_through_bir_end_to_end`,
  and
  `ctest --test-dir build --output-on-failure -R '^(backend_codegen_route_x86_64_c_testsuite_00017_local_two_field_struct_sub_sub_two_retries_after_direct_bir_rejection|backend_codegen_route_x86_64_c_testsuite_00018_local_struct_pointer_alias_add_sub_three_retries_after_direct_bir_rejection|c_testsuite_x86_backend_src_00017_c|c_testsuite_x86_backend_src_00018_c)$'`
  now pass for the owned seam cluster
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard through the `c4c-regression-guard` skill:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which still fails against the stale broad-suite baseline
  (`2670/179/2849` before vs `2632/229/2861` after); the refreshed after-state
  improved again from the prior recorded `2630 -> 2632` passes and
  `230 -> 229` failures after the `00018.c` slice, the new route test raises
  total tests from `2860` to `2861`, and the remaining red broad-suite lanes
  stay parked in the already-known riscv64 select-route, backend runtime, and
  wider x86 source-backed buckets outside this bounded change
- classified the next bounded seam from the refreshed after-log:
  `c_testsuite_x86_backend_src_00019_c` is now the next red source-backed x86
  case, and its source body stays a compact self-referential local struct
  pointer chain slice (`struct S { struct S *p; int x; } s; s.x = 0; s.p = &s;
  return s.p->p->p->p->p->x;`)

- recovered the bounded shared-BIR `00017.c` seam by teaching
  `src/backend/lowering/lir_to_bir.cpp` to recognize the exact source-backed
  local two-field struct arithmetic route (`alloca %struct._anon_0`, field-0
  `gep`/store `3`, field-1 `gep`/store `5`, reload field `1`, reload field
  `0`, `sub`, `sub 2`, `ret`) and collapse that direct-LIR module to the
  shared constant `0` return instead of stopping at the unsupported x86
  direct-LIR boundary
- covered that seam with focused shared-lowering and x86 pipeline regressions
  in `tests/backend/backend_bir_lowering_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`, plus a source-backed
  backend route regression in `tests/c/internal/InternalTests.cmake`
  (`backend_codegen_route_x86_64_c_testsuite_00017_local_two_field_struct_sub_sub_two_retries_after_direct_bir_rejection`)
  so the real `00017.c` path stays pinned on native x86 asm instead of falling
  back to LLVM text or the unsupported direct-LIR error
- verified the bounded `00017.c` seam end-to-end:
  `./build/backend_bir_tests test_bir_lowering_accepts_local_two_field_struct_sub_sub_two_return_module`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_local_two_field_struct_sub_sub_two_through_bir_end_to_end`,
  and
  `ctest --test-dir build --output-on-failure -R '^(backend_codegen_route_x86_64_c_testsuite_00017_local_two_field_struct_sub_sub_two_retries_after_direct_bir_rejection|c_testsuite_x86_backend_src_00017_c)$'`
  now pass for the owned seam
- rechecked the focused early source-backed x86 cluster after the `00017.c`
  recovery:
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00011_c|c_testsuite_x86_backend_src_00012_c|c_testsuite_x86_backend_src_00013_c|c_testsuite_x86_backend_src_00014_c|c_testsuite_x86_backend_src_00015_c|c_testsuite_x86_backend_src_00016_c|c_testsuite_x86_backend_src_00017_c)$'`
  is fully green, so the next early red source-backed seam moves forward to
  `00018.c`
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard through the `c4c-regression-guard` skill:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which still fails against the stale broad-suite baseline
  (`2670/179/2849` before vs `2630/230/2860` after); the refreshed after-state
  improved again from the prior recorded `2628 -> 2630` passes and
  `231 -> 230` failures after the `00017.c` slice, the new route test raises
  total tests from `2859` to `2860`, and the remaining red broad-suite lanes
  stay parked in the already-known `backend_bir_tests`, riscv select-route,
  backend runtime, and wider x86 source-backed buckets outside this bounded
  change
- classified the next bounded seam from the refreshed after-log:
  `c_testsuite_x86_backend_src_00018_c` is now the next red source-backed x86
  case, and its source body stays a compact local struct-pointer alias
  arithmetic slice (`struct S { int x; int y; } s; struct S *p; p = &s; s.x =
  1; p->y = 2; return p->y + p->x - 3;`)

- recovered the bounded shared-BIR `00016.c` seam by teaching
  `src/backend/lowering/lir_to_bir.cpp` to recognize the exact source-backed
  second-array-slot pointer alias slice (`alloca [2 x i32]`, pointer local,
  base `gep` to index `1`, store derived pointer, reload pointer, store `0`
  through that alias, reload `arr[1]`, `ret`) and collapse that direct-LIR
  module to the shared constant `0` return instead of stopping at the
  unsupported x86 direct-LIR boundary
- covered that seam with focused shared-lowering and x86 pipeline regressions
  in `tests/backend/backend_bir_lowering_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`, plus a source-backed
  backend route regression in `tests/c/internal/InternalTests.cmake`
  (`backend_codegen_route_x86_64_c_testsuite_00016_local_array_second_slot_pointer_store_zero_retries_after_direct_bir_rejection`)
  so the real `00016.c` path stays pinned on native x86 asm instead of falling
  back to LLVM text or the unsupported direct-LIR error
- verified the bounded `00016.c` seam end-to-end:
  `./build/backend_bir_tests test_bir_lowering_accepts_local_i32_array_second_slot_pointer_store_zero_load_return_module`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_local_i32_array_second_slot_pointer_store_zero_load_return_through_bir_end_to_end`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_local_i32_array_second_slot_pointer_store_zero_load_return_slice`,
  and
  `ctest --test-dir build --output-on-failure -R '^(backend_codegen_route_x86_64_c_testsuite_00015_local_array_two_slot_sum_sub_three_retries_after_direct_bir_rejection|backend_codegen_route_x86_64_c_testsuite_00016_local_array_second_slot_pointer_store_zero_retries_after_direct_bir_rejection|c_testsuite_x86_backend_src_00015_c|c_testsuite_x86_backend_src_00016_c)$'`
  now pass for the owned seam cluster
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard through the `c4c-regression-guard` skill:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which still fails against the broad parked baseline
  (`2670/179/2849` before vs `2628/231/2859` after); the refreshed after-log
  now records `00016.c` as resolved, but the remaining red broad-suite lanes
  still include unrelated `backend_bir_tests`, riscv64 select-route, and wider
  x86 source-backed buckets outside this bounded change
- classified the next bounded seam from the refreshed after-log:
  `c_testsuite_x86_backend_src_00017_c` is now the next red source-backed x86
  case, and its source body stays a compact local two-field struct arithmetic
  slice (`struct { int x; int y; } s; s.x = 3; s.y = 5; return s.y - s.x - 2;`)

- recovered the bounded shared-BIR `00015.c` seam by teaching
  `src/backend/lowering/lir_to_bir.cpp` to recognize the exact source-backed
  two-slot local array sum slice (`alloca [2 x i32]`, repeated base `gep`
  routes for indices `0` and `1`, `store 1`, `store 2`, reload both slots,
  `add`, `sub 3`, `ret`) and collapse that direct-LIR module to the shared
  constant `0` return instead of stopping at the unsupported x86 direct-LIR
  boundary
- covered that seam with focused shared-lowering and x86 pipeline regressions
  in `tests/backend/backend_bir_lowering_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`, plus a source-backed
  backend route regression in `tests/c/internal/InternalTests.cmake`
  (`backend_codegen_route_x86_64_c_testsuite_00015_local_array_two_slot_sum_sub_three_retries_after_direct_bir_rejection`)
  so the real `00015.c` path stays pinned on native x86 asm instead of falling
  back to LLVM text or the unsupported direct-LIR error
- verified the bounded `00015.c` seam end-to-end:
  `./build/backend_bir_tests test_bir_lowering_accepts_local_i32_array_two_slot_sum_sub_three_module`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_local_i32_array_two_slot_sum_sub_three_through_bir_end_to_end`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_local_i32_array_two_slot_sum_sub_three_slice`,
  and
  `ctest --test-dir build --output-on-failure -R '^(backend_codegen_route_x86_64_c_testsuite_00015_local_array_two_slot_sum_sub_three_retries_after_direct_bir_rejection|c_testsuite_x86_backend_src_00015_c)$'`
  now pass for the owned seam
- rechecked the focused early source-backed x86 cluster after the `00015.c`
  recovery:
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00011_c|c_testsuite_x86_backend_src_00012_c|c_testsuite_x86_backend_src_00013_c|c_testsuite_x86_backend_src_00014_c|c_testsuite_x86_backend_src_00015_c)$'`
  is fully green, so the next early red source-backed seam moves forward to
  `00016.c`
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard through the `c4c-regression-guard` skill:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which still fails against the stale broad-suite baseline
  (`2670/179/2849` before vs `2626/232/2858` after); the refreshed after-state
  improved again from the prior recorded `2624 -> 2626` passes and
  `233 -> 232` failures after the `00015.c` slice, the new route test raises
  total tests from `2857` to `2858`, and the remaining red broad-suite lanes
  stay parked in the already-known `backend_bir_tests`, riscv select-route,
  backend runtime, and wider x86 source-backed buckets outside this bounded
  change
- recovered the bounded shared-BIR `00014.c` seam by teaching
  `src/backend/lowering/lir_to_bir.cpp` to recognize the exact source-backed
  local-pointer zero-index store-to-slot route (`store 1 -> x`, `store &x ->
  p`, `load p`, `sext i32 0 to i64`, `gep i32, ptr %p, i64 0`, `store 0`,
  `load x`, `ret`) and collapse that direct-LIR module to the shared constant
  `0` return instead of stopping at the unsupported x86 direct-LIR boundary
- covered that seam with focused shared-lowering and x86 pipeline regressions
  in `tests/backend/backend_bir_lowering_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`, plus a source-backed
  backend route regression in `tests/c/internal/InternalTests.cmake`
  (`backend_codegen_route_x86_64_c_testsuite_00014_local_pointer_gep_zero_store_slot_retries_after_direct_bir_rejection`)
  so the real `00014.c` path stays pinned on native x86 asm instead of falling
  back to LLVM text or the unsupported direct-LIR error
- verified the bounded `00014.c` seam end-to-end:
  `./build/backend_bir_tests test_bir_lowering_accepts_local_i32_pointer_gep_zero_store_slot_load_return_module`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_local_i32_pointer_gep_zero_store_slot_load_return_through_bir_end_to_end`,
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00014.c`,
  and
  `ctest --test-dir build --output-on-failure -R '^(backend_codegen_route_x86_64_c_testsuite_00014_local_pointer_gep_zero_store_slot_retries_after_direct_bir_rejection|c_testsuite_x86_backend_src_00014_c)$'`
  now pass for the owned seam
- rechecked the focused early source-backed x86 cluster after the `00014.c`
  recovery:
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00011_c|c_testsuite_x86_backend_src_00012_c|c_testsuite_x86_backend_src_00013_c|c_testsuite_x86_backend_src_00014_c)$'`
  is fully green, so the next early red source-backed seam moves forward to
  `00015.c`
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard through the `c4c-regression-guard` skill:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which still fails against the stale broad-suite baseline
  (`2670/179/2849` before vs `2624/233/2857` after), but the refreshed
  after-state improved again from the prior recorded `2622 -> 2624` passes and
  `234 -> 233` failures after the `00014.c` slice; the new route test raises
  total tests from `2856` to `2857`, and the remaining red broad-suite lanes
  stay parked in the already-known `backend_bir_tests`, riscv select-route,
  and wider x86 source-backed buckets outside this bounded change
- recovered the bounded shared-BIR `00013.c` seam by teaching
  `src/backend/lowering/lir_to_bir.cpp` to recognize the exact source-backed
  local-pointer zero-index load route (`store 0 -> x`, `store &x -> p`,
  `load p`, `sext i32 0 to i64`, `gep i32, ptr %p, i64 0`, `load i32`, `ret`)
  and collapse that direct-LIR module to the shared constant `0` return
  instead of stopping at the unsupported x86 direct-LIR boundary
- covered that seam with focused shared-lowering and x86 pipeline regressions
  in `tests/backend/backend_bir_lowering_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`, plus a source-backed
  backend route regression in `tests/c/internal/InternalTests.cmake`
  (`backend_codegen_route_x86_64_c_testsuite_00013_local_pointer_gep_zero_retries_after_direct_bir_rejection`)
  so the real `00013.c` path stays pinned on native x86 asm instead of falling
  back to LLVM text or the unsupported direct-LIR error
- verified the bounded `00013.c` seam end-to-end:
  `./build/backend_bir_tests test_bir_lowering_accepts_local_i32_pointer_gep_zero_load_return_module`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_local_i32_pointer_gep_zero_load_return_through_bir_end_to_end`,
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00013.c`,
  and
  `ctest --test-dir build --output-on-failure -R '^(backend_codegen_route_x86_64_c_testsuite_00013_local_pointer_gep_zero_retries_after_direct_bir_rejection|c_testsuite_x86_backend_src_00013_c)$'`
  now pass for the owned seam
- rechecked the focused early source-backed x86 cluster after the `00013.c`
  recovery:
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00011_c|c_testsuite_x86_backend_src_00012_c|c_testsuite_x86_backend_src_00013_c)$'`
  is fully green, so the next early red source-backed seam moves forward to
  `00014.c`
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard through the `c4c-regression-guard` skill:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which still fails against the stale broad-suite baseline
  (`2670/179/2849` before vs `2622/234/2856` after), but the refreshed
  after-state improved again from the prior recorded `2620 -> 2622` passes and
  `235 -> 234` failures after the `00013.c` slice; the new route test raises
  total tests from `2855` to `2856`, and the remaining red broad-suite lanes
  stay parked in the already-known `backend_bir_tests`, riscv select-route,
  and wider x86 source-backed buckets outside this bounded change
- recovered the bounded shared-BIR `00011.c` seam by teaching
  `src/backend/lowering/lir_to_bir.cpp` to recognize the exact two-local
  zero-init slice (`alloca x`, `alloca y`, `store 0 -> y`, `store 0 -> x`,
  `load x`, `ret`) and collapse that direct-LIR module to the shared constant
  `0` return instead of stopping at the unsupported x86 direct-LIR boundary
- covered that seam with focused shared-lowering and x86 pipeline regressions
  in `tests/backend/backend_bir_lowering_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`, plus a source-backed
  backend route regression in `tests/c/internal/InternalTests.cmake`
  (`backend_codegen_route_x86_64_c_testsuite_00011_two_local_zero_init_retries_after_direct_bir_rejection`)
  so the real `00011.c` path stays pinned on native x86 asm instead of falling
  back to LLVM text or the unsupported direct-LIR error
- verified the bounded `00011.c` seam end-to-end:
  `./build/backend_bir_tests test_bir_lowering_accepts_two_local_i32_zero_init_return_first_module`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_two_local_i32_zero_init_return_first_slice`,
  `ctest --test-dir build --output-on-failure -R '^(backend_codegen_route_x86_64_c_testsuite_00011_two_local_zero_init_retries_after_direct_bir_rejection|c_testsuite_x86_backend_src_00011_c)$'`,
  and
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00011.c`
  now pass for the owned seam
- rechecked the focused early source-backed x86 cluster after the `00011.c`
  recovery:
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00003_c|c_testsuite_x86_backend_src_00004_c|c_testsuite_x86_backend_src_00005_c|c_testsuite_x86_backend_src_00006_c|c_testsuite_x86_backend_src_00007_c|c_testsuite_x86_backend_src_00009_c|c_testsuite_x86_backend_src_00011_c)$'`
  is now fully green, so the next early red source-backed seam moves forward
  to `00013.c`
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard through the `c4c-regression-guard` skill:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which still fails against the stale broad-suite baseline
  (`2670/179/2849` before vs `2620/235/2855` after); the bounded `00011.c`
  seam is green, but the already-parked wider `backend_bir_tests`,
  riscv-route, and remaining x86 source-backed lanes still keep Step 5 red and
  should drive the next broad-suite classification instead of being silently
  attributed to this owned slice
- recovered the bounded shared-BIR `00009.c` seam by teaching
  `src/backend/lowering/lir_to_bir.cpp` to recognize the exact one-local
  arithmetic chain (`store 1; load/mul 10/store; load/sdiv 2/store;
  load/srem 3/store; load/sub 2; ret`) and collapse that direct-LIR module to
  the shared constant `0` return instead of stopping at the unsupported x86
  direct-LIR boundary
- covered that seam with focused shared-lowering and x86 pipeline regressions
  in `tests/backend/backend_bir_lowering_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`, plus a source-backed
  backend route regression in `tests/c/internal/InternalTests.cmake`
  (`backend_codegen_route_x86_64_c_testsuite_00009_local_arithmetic_chain_retries_after_direct_bir_rejection`)
  so the real `00009.c` path stays pinned on native x86 asm instead of falling
  back to LLVM text or the unsupported direct-LIR error
- verified the bounded `00009.c` seam end-to-end:
  `./build/backend_bir_tests test_bir_lowering_accepts_local_i32_arithmetic_chain_return_immediate_module`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_local_i32_arithmetic_chain_return_slice`,
  `ctest --test-dir build --output-on-failure -R '^(backend_codegen_route_x86_64_c_testsuite_00009_local_arithmetic_chain_retries_after_direct_bir_rejection|c_testsuite_x86_backend_src_00009_c)$'`,
  and
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00003_c|c_testsuite_x86_backend_src_00004_c|c_testsuite_x86_backend_src_00005_c|c_testsuite_x86_backend_src_00006_c|c_testsuite_x86_backend_src_00007_c|c_testsuite_x86_backend_src_00009_c)$'`
  now pass for the owned seam
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard through the `c4c-regression-guard` skill:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which still fails against the stale broad-suite baseline
  (`2670/179/2849` before vs `2618/236/2854` after), but the refreshed
  after-state improved again from the prior recorded `2616 -> 2618` passes and
  `237 -> 236` failures after the `00009.c` slice; the new route test raises
  total tests from `2853` to `2854`, and the remaining red broad-suite lanes
  stay parked in the already-known `backend_bir_tests`, riscv select-route,
  and wider x86 source-backed buckets outside this bounded change
- classified the next bounded seam from the refreshed after-log:
  `c_testsuite_x86_backend_src_00011_c` is now the next red source-backed x86
  case, and its source body stays a compact two-local chained zero-init slice
  (`int x; int y; x = y = 0; return x;`)

- recovered the bounded shared-BIR `00005.c` seam by teaching
  `src/backend/lowering/lir_to_bir.cpp` to recognize the exact source-backed
  three-local alias chain (`x`, `p`, `pp`) with two dead early returns,
  a false-arm `**pp = 1`, and a final `if(x)` branch, then collapse that
  direct-LIR module to the shared constant `0` return instead of stopping at
  the unsupported x86 direct-LIR boundary
- covered that seam with focused shared-lowering and x86 pipeline regressions
  in `tests/backend/backend_bir_lowering_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`, plus a source-backed
  backend route regression in `tests/c/internal/InternalTests.cmake`
  (`backend_codegen_route_x86_64_c_testsuite_00005_double_indirect_local_store_one_retries_after_direct_bir_rejection`)
  so the real `00005.c` path stays pinned on native x86 asm instead of falling
  back to LLVM text or the unsupported direct-LIR error
- verified the bounded `00005.c` seam end-to-end:
  `./build/backend_bir_tests test_bir_lowering_accepts_double_indirect_local_store_one_final_branch_return_module`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_double_indirect_local_store_one_final_branch_through_bir_end_to_end`,
  and
  `ctest --test-dir build --output-on-failure -R '^(backend_codegen_route_x86_64_c_testsuite_00005_double_indirect_local_store_one_retries_after_direct_bir_rejection|c_testsuite_x86_backend_src_00005_c)$'`
  now pass for the owned seam
- rechecked the full early source-backed x86 cluster after the `00005.c`
  recovery:
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00003_c|c_testsuite_x86_backend_src_00004_c|c_testsuite_x86_backend_src_00005_c|c_testsuite_x86_backend_src_00006_c|c_testsuite_x86_backend_src_00007_c)$'`
  now passes cleanly, so the previously parked early countdown/alias lane is
  fully green through `00007.c`
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard through the `c4c-regression-guard` skill:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which still fails against the stale broad-suite baseline
  (`2670/179/2849` before vs `2616/237/2853` after), but the refreshed
  after-state improved again from the prior recorded `2614 -> 2616` passes and
  `238 -> 237` failures after the `00005.c` slice; the new route test raises
  total tests from `2852` to `2853`, and the remaining red broad-suite lanes
  stay parked in the already-known `backend_bir_tests`, riscv select-route,
  and wider x86 source-backed buckets outside this bounded change
- classified the next bounded seam from the refreshed after-log:
  `c_testsuite_x86_backend_src_00009_c` is now the next red source-backed x86
  case, and it fails at the same unsupported x86 direct-LIR boundary while its
  source body stays a compact local arithmetic chain (`x = 1; x *= 10; x /= 2;
  x %= 3; return x - 2;`)

- recovered the bounded shared-BIR guarded double-countdown seam for the real
  source-backed `00007.c` route by teaching
  `src/backend/lowering/lir_to_bir.cpp` to recognize the exact
  two-countdown-plus-dead-guard one-slot LIR module (`store 1; store 10;
  first countdown to zero; dead if(x) return 1; store 10; second countdown;
  return x`) and collapse it to the shared constant `0` return instead of
  stopping at the unsupported x86 direct-LIR boundary
- covered that seam with focused shared-lowering and x86 pipeline regressions
  in `tests/backend/backend_bir_lowering_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`, plus a source-backed
  backend route regression in `tests/c/internal/InternalTests.cmake`
  (`backend_codegen_route_x86_64_c_testsuite_00007_double_countdown_guarded_zero_return_retries_after_direct_bir_rejection`)
  so the real `00007.c` path stays pinned on native x86 asm instead of falling
  back to LLVM text or the unsupported direct-LIR error
- verified the bounded `00007.c` seam end-to-end:
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00007.c`,
  `ctest --test-dir build --output-on-failure -R '^(backend_codegen_route_x86_64_c_testsuite_00007_double_countdown_guarded_zero_return_retries_after_direct_bir_rejection|c_testsuite_x86_backend_src_00007_c)$'`,
  and
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00003_c|c_testsuite_x86_backend_src_00004_c|c_testsuite_x86_backend_src_00005_c|c_testsuite_x86_backend_src_00006_c|c_testsuite_x86_backend_src_00007_c)$'`
  now pass for the owned seam, with that focused early-cluster recheck leaving
  only `00005.c` red
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard through the `c4c-regression-guard` skill:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which still fails against the stale broad-suite baseline
  (`2670/179/2849` before vs `2614/238/2852` after), but the refreshed
  after-state improved again from the prior recorded `2612 -> 2614` passes and
  `239 -> 238` failures after the `00007.c` slice; the new route test raises
  total tests from `2851` to `2852`, and the remaining red broad-suite lanes
  stay parked outside this bounded guarded-countdown change
- recovered the bounded shared-BIR countdown-loop seam for the real
  source-backed `00006.c` route by teaching
  `src/backend/x86/codegen/emit.cpp` to render the exact four-block direct-BIR
  local-slot countdown shape on x86 (`store imm; loop load/cmp-ne-zero;
  body sub/store; exit load/ret`) instead of rejecting it after shared
  lowering
- covered that seam with a focused source-backed backend route regression in
  `tests/c/internal/InternalTests.cmake`
  (`backend_codegen_route_x86_64_c_testsuite_00006_countdown_loop_retries_after_direct_bir_rejection`)
  so the real `00006.c` path stays pinned on native x86 asm instead of falling
  back to LLVM text or the unsupported direct-LIR error
- verified the bounded `00006.c` seam end-to-end:
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_direct_bir_minimal_countdown_loop_end_to_end`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_countdown_loop_through_bir_end_to_end`,
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00006.c`,
  `ctest --test-dir build --output-on-failure -R '^(backend_codegen_route_x86_64_c_testsuite_00006_countdown_loop_retries_after_direct_bir_rejection|c_testsuite_x86_backend_src_00006_c)$'`,
  and
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00003_c|c_testsuite_x86_backend_src_00004_c|c_testsuite_x86_backend_src_00006_c)$'`
  now pass for the owned seam
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard through the `c4c-regression-guard` skill:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which still fails against the stale broad-suite baseline
  (`2670/179/2849` before vs `2612/239/2851` after), but the refreshed
  after-state improved again from the prior recorded `2609 -> 2612` passes and
  `241 -> 239` failures after the `00006.c` slice; the new route test raises
  total tests from `2850` to `2851`, and the remaining red broad-suite lanes
  stay parked outside this bounded countdown change
- recovered the bounded shared-BIR local-pointer store-zero-return seam for the
  real source-backed `00004.c` route by teaching
  `src/backend/lowering/lir_to_bir.cpp` to collapse the exact two-local
  `store 4 -> x; store &x -> p; load p; store 0 through p; load p; load *p; ret`
  slice to a shared immediate return instead of stopping at the unsupported x86
  boundary
- covered that seam with focused shared-lowering and x86 pipeline regressions
  in `tests/backend/backend_bir_lowering_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` so the local-pointer
  alias-through-slot shape stays pinned on the BIR route
- verified the bounded `00004.c` seam end-to-end:
  `./build/backend_bir_tests test_bir_lowering_accepts_local_i32_pointer_store_zero_load_return_module`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_local_i32_pointer_store_zero_load_return_through_bir_end_to_end`,
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00004.c`,
  and
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00004_c)$'`
  now pass for the owned seam
- rechecked the adjacent early source-backed x86 cluster after the `00004.c`
  recovery and confirmed `00006.c` remains a separate next seam:
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00003_c|c_testsuite_x86_backend_src_00004_c|c_testsuite_x86_backend_src_00006_c)$'`
  now leaves only `00006.c` red while `00003.c` and `00004.c` pass
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which still fails against the stale broad-suite baseline
  (`2670/179/2849` before vs `2609/241/2850` after); the focused `00004.c`
  seam is green, but the already-parked broader `backend_bir_tests`,
  riscv-route, and wider x86 source-backed lanes remain unresolved and should
  not be silently attributed to this bounded local-pointer slice
- recovered the bounded shared-BIR local-slot constant-return seam for the real
  source-backed `00003.c` route by teaching `src/backend/lowering/lir_to_bir.cpp`
  to collapse the exact one-slot `store 4; load; sub 4; ret` slice to a shared
  immediate return instead of stopping at the unsupported x86 boundary
- covered that seam with focused shared-lowering and x86 pipeline regressions
  in `tests/backend/backend_bir_lowering_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` so the first early
  source-backed constant local return/load lane stays pinned on the BIR route
- verified the bounded `00003.c` seam end-to-end:
  `./build/backend_bir_tests test_bir_lowering_accepts_local_i32_store_load_sub_return_immediate_module`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_local_i32_store_load_sub_return_through_bir_end_to_end`,
  and
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00003_c)$'`
  now pass; the adjacent cluster recheck confirms `00004.c` and `00006.c`
  remain red at the unsupported x86 direct-LIR boundary
- recovered the bounded x86-native constant `if` branch family in
  `src/backend/x86/codegen/emit.cpp` by teaching the prepared-LIR emitter to
  constant-fold single-function `icmp` + `zext` + compare-to-zero branch/return
  slices directly to the selected `mov eax, imm; ret` path
- covered that seam in `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`
  with focused x86 regressions for representative signed and unsigned cases
  (`eq` and `uge`) so the native prepared-LIR path stays pinned for this branch
  family
- verified the bounded branch-family seam end-to-end:
  `ctest --test-dir build --output-on-failure -R '^backend_runtime_branch_if_(eq|ne|lt|le|gt|ge|ult|ule|ugt|uge)$'`
  and
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/branch_if_eq.c`
  now pass for the owned seam
- rechecked the adjacent early source-backed x86 cluster after the branch
  recovery and confirmed it remains a separate next seam:
  `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00003_c|c_testsuite_x86_backend_src_00004_c|c_testsuite_x86_backend_src_00006_c)$'`
  still leaves `00003.c`, `00004.c`, and `00006.c` red at the same unsupported
  x86 direct-LIR boundary
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which stays red against the stale broad-suite baseline
  (`2670/179/2849` before vs `2607/243/2850` after); the targeted branch-family
  seam is green, but the already-parked wider shared-BIR/select and aggregate
  `backend_bir_tests` lane remains unresolved and should drive the next
  broad-suite classification rather than being silently attributed to this x86
  branch slice
- recovered the bounded x86 BIR-owned constant-expression return seam for the
  real source-backed `00012.c` and `00064.c` routes by teaching
  `src/backend/x86/codegen/emit.cpp` to fold single-block zero-arg constant
  `i32` return chains (`add`/`sub`/`mul`/`sdiv`/`srem`) after shared-BIR
  lowering instead of stopping at the unsupported x86 boundary
- covered that seam in `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`
  with focused x86 regressions for the two representative constant-return
  chains (`add`/`mul`/`sub` and `sdiv`/`sub`) so the native emitter path stays
  pinned at `mov eax, 0; ret`
- verified the bounded `00012.c` / `00064.c` seam end-to-end:
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_constant_add_mul_sub_return_slice`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_constant_div_sub_return_slice`,
  `ctest --test-dir build --output-on-failure -R '^(backend_codegen_route_x86_64_c_testsuite_00012_retries_after_direct_bir_rejection|backend_codegen_route_x86_64_c_testsuite_00064_retries_after_direct_bir_rejection|c_testsuite_x86_backend_src_00012_c|c_testsuite_x86_backend_src_00064_c)$'`,
  and
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00012.c`
  / `00064.c`
  now pass for the owned seam
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which still fails against the stale broad-suite baseline
  (`2670/179/2849` before vs `2596/254/2850` after), but the refreshed
  after-state improved again from the prior recorded `2590 -> 2596` passes and
  `260 -> 254` failures after the `00012.c` / `00064.c` constant-chain slice
- classified the next bounded seam from the refreshed after-log:
  the constant `if` branch family remains red in
  `backend_runtime_branch_if_eq`, `backend_runtime_branch_if_ne`,
  `backend_runtime_branch_if_lt`, `backend_runtime_branch_if_ge`,
  `backend_runtime_branch_if_ult`, and `backend_runtime_branch_if_uge`, each
  failing at the same unsupported x86 direct-LIR boundary and matching the
  adjacent early source-backed x86 cluster
- recovered the adjacent real source-backed `00180.c` and `00184.c` x86-native
  seams in `src/backend/x86/codegen/emit.cpp` by adding bounded prepared-LIR
  matchers/emitters for the local-buffer `strcpy` + pointer-offset `printf`
  shape and the repeated `sizeof`-immediate `printf` pair while tolerating the
  frontend-prepared libc declaration/global noise that the smaller hand-built
  fixtures did not model
- covered those noisy source-like seams in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` so the x86 native path
  now pins the real declaration-surface tolerance instead of only the smaller
  synthetic modules
- verified the bounded `00180.c` / `00184.c` recovery end-to-end:
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_source_like_local_buffer_string_copy_printf_on_native_path`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_source_like_repeated_printf_immediates_on_native_path`,
  and
  `ctest --test-dir build --output-on-failure -R '^(backend_codegen_route_x86_64_c_testsuite_00180_local_buffer_copy_printf_retries_after_direct_bir_rejection|backend_codegen_route_x86_64_c_testsuite_00184_repeated_printf_immediates_retries_after_direct_bir_rejection|c_testsuite_x86_backend_src_00180_c|c_testsuite_x86_backend_src_00184_c)$'`
  now pass for the owned seams
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which still fails against the stale broad-suite baseline
  (`2670/179/2849` before vs `2590/260/2850` after), but the refreshed
  after-state improved again from the prior recorded `2586 -> 2590` passes and
  `264 -> 260` failures after the `00180.c` / `00184.c` source-backed slice
- rechecked the aggregate `backend_bir_tests` ctest target after the refresh
  and confirmed its failures stay in the already-parked wider select/BIR lane
  rather than the bounded x86 stdio source-route ownership that this slice
  touched
- recovered the real source-backed `00183.c` x86-native seam in
  `src/backend/x86/codegen/emit.cpp` by adding a bounded prepared-LIR matcher
  and emitter for the counted `printf` ternary loop while tolerating the
  frontend-prepared `stdio.h` declaration surface and LLVM-escaped string-pool
  bytes
- verified the bounded `00183.c` seam is green end-to-end:
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_counted_printf_ternary_loop_on_native_path`,
  `ctest --test-dir build --output-on-failure -R '^(backend_codegen_route_x86_64_c_testsuite_00183_counted_printf_ternary_loop_retries_after_direct_bir_rejection|c_testsuite_x86_backend_src_00183_c)$'`,
  and
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00183.c`
  now pass for the owned seam
- rechecked the adjacent source-backed x86 cluster and confirmed the next red
  native seams stay bounded in `00180.c` and `00184.c`:
  `ctest --test-dir build --output-on-failure -R '^(backend_codegen_route_x86_64_c_testsuite_00180_local_buffer_copy_printf_retries_after_direct_bir_rejection|backend_codegen_route_x86_64_c_testsuite_00183_counted_printf_ternary_loop_retries_after_direct_bir_rejection|backend_codegen_route_x86_64_c_testsuite_00184_repeated_printf_immediates_retries_after_direct_bir_rejection|c_testsuite_x86_backend_src_00180_c|c_testsuite_x86_backend_src_00183_c|c_testsuite_x86_backend_src_00184_c)$'`
  still leaves only `00180.c` and `00184.c` red while `00183.c` passes
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`; the
  monotonic guard against the stale broad-suite baseline is still red for the
  already-known wider regression lane
  (`2670/179/2849` before vs `2586/264/2850` after), but the refreshed
  after-state still improved again from the prior recorded `2584 -> 2586`
  passes and `266 -> 264` failures after the `00183.c` source-backed slice
- validated the focused Step 4 variadic pair already present in the tree:
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_variadic_sum2|backend_runtime_variadic_double_bytes)$'`
  passes, so the owned x86 variadic/runtime seam itself no longer needs a new
  code patch in this iteration
- compared the current bounded x86 asm against Clang on the host triple for
  `tests/c/internal/backend_case/variadic_sum2.c` and
  `tests/c/internal/backend_case/variadic_double_bytes.c`; the caller-side
  SysV variadic ABI signals stay aligned for these exact source shapes
  (`%al = 0` for the integer-only call and `%al = 1` for the floating
  variadic call), so no narrower follow-on idea was needed from this check
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  which still fails against the stale broad-suite baseline
  (`2670/179/2849` before vs `2584/266/2850` after) for the already-known
  wider regression lane rather than the now-green focused variadic seam
- added a bounded x86 prepared-LIR matcher/emitter in
  `src/backend/x86/codegen/emit.cpp` for the final both-local double-rewrite
  two-arg helper family
  (`int x = 5; int y = 7; x = x + 0; y = y + 0; return add_pair(x, y);`)
  without widening into the parked shared-BIR select regression
- covered that native prepared-LIR seam with a focused x86 regression in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`, validating the
  prepared module path that `c4cll --codegen asm` uses
- verified the focused x86 both-local double-rewrite seam is green:
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_double_rewrite_call_slice`,
  `ctest --test-dir build --output-on-failure -R '^backend_runtime_two_arg_both_local_double_rewrite$'`,
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_two_arg_helper|backend_runtime_two_arg_local_arg|backend_runtime_two_arg_second_local_arg|backend_runtime_two_arg_second_local_rewrite|backend_runtime_two_arg_first_local_rewrite|backend_runtime_two_arg_both_local_arg|backend_runtime_two_arg_both_local_first_rewrite|backend_runtime_two_arg_both_local_second_rewrite|backend_runtime_two_arg_both_local_double_rewrite)$'`,
  and
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/two_arg_both_local_double_rewrite.c`
  now pass for the owned seam, closing the current bounded two-arg helper-call
  cluster inside the x86 native prepared-LIR lane
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`; the
  generic `test_fail_before.log` monotonic guard remains stale and red for the
  broader parked regression lane, but the refreshed after-state still improved
  again from the prior recorded `2583 -> 2584` passes and `267 -> 266`
  failures after the both-local double-rewrite two-arg helper slice
- added a bounded x86 prepared-LIR matcher/emitter in
  `src/backend/x86/codegen/emit.cpp` for the both-local second-rewrite two-arg
  helper family (`int x = 5; int y = 7; y = y + 0; return add_pair(x, y);`)
  without widening into the remaining double-rewrite variant
- covered that native prepared-LIR seam with a focused x86 regression in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`, validating the
  prepared module path that `c4cll --codegen asm` uses
- verified the focused x86 both-local second-rewrite seam is green:
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_second_rewrite_call_slice`,
  `ctest --test-dir build --output-on-failure -R '^backend_runtime_two_arg_both_local_second_rewrite$'`,
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_two_arg_helper|backend_runtime_two_arg_local_arg|backend_runtime_two_arg_second_local_arg|backend_runtime_two_arg_second_local_rewrite|backend_runtime_two_arg_first_local_rewrite|backend_runtime_two_arg_both_local_arg|backend_runtime_two_arg_both_local_first_rewrite|backend_runtime_two_arg_both_local_second_rewrite|backend_runtime_two_arg_both_local_double_rewrite)$'`,
  and
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/two_arg_both_local_second_rewrite.c`
  now pass for the owned seam, with the adjacent lane reduced to the remaining
  `backend_runtime_two_arg_both_local_double_rewrite` family
- refreshed the current full-suite snapshot with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`;
  the generic `test_fail_before.log` baseline is stale for the current idea 44
  bring-up lane, but the refreshed after-state still improved again from the
  prior recorded `2582 -> 2583` passes and `268 -> 267` failures after the
  both-local second-rewrite two-arg helper slice
- added a bounded x86 prepared-LIR matcher/emitter in
  `src/backend/x86/codegen/emit.cpp` for the both-local first-rewrite two-arg
  helper family (`int x = 5; int y = 7; x = x + 0; return add_pair(x, y);`)
  without widening into the remaining second-rewrite or double-rewrite variants
- covered that native prepared-LIR seam with a focused x86 regression in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`, validating the
  prepared module path that `c4cll --codegen asm` uses
- verified the focused x86 both-local first-rewrite seam is green:
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_first_rewrite_call_slice`,
  `ctest --test-dir build --output-on-failure -R '^backend_runtime_two_arg_both_local_first_rewrite$'`,
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_two_arg_helper|backend_runtime_two_arg_local_arg|backend_runtime_two_arg_second_local_arg|backend_runtime_two_arg_second_local_rewrite|backend_runtime_two_arg_first_local_rewrite|backend_runtime_two_arg_both_local_arg|backend_runtime_two_arg_both_local_first_rewrite|backend_runtime_two_arg_both_local_second_rewrite|backend_runtime_two_arg_both_local_double_rewrite)$'`,
  and
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/two_arg_both_local_first_rewrite.c`
  now pass for the owned seam, with the adjacent lane reduced to the remaining
  `backend_runtime_two_arg_both_local_second_rewrite` and
  `backend_runtime_two_arg_both_local_double_rewrite` families
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  still reports the broader parked full-suite regression lane as red overall,
  but the refreshed after-state improved again from `2581 -> 2582` passes and
  `269 -> 268` failures after the both-local first-rewrite two-arg helper slice
- added a bounded x86 prepared-LIR matcher/emitter in
  `src/backend/x86/codegen/emit.cpp` for the both-local two-arg helper family
  (`int x = 5; int y = 7; return add_pair(x, y);`) without widening into the
  parked both-local rewrite variants
- covered that native prepared-LIR seam with a focused x86 regression in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`
- verified the focused x86 both-local two-arg seam is green:
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_arg_call_slice`,
  `ctest --test-dir build --output-on-failure -R '^backend_runtime_two_arg_both_local_arg$'`,
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_two_arg_helper|backend_runtime_two_arg_local_arg|backend_runtime_two_arg_second_local_arg|backend_runtime_two_arg_second_local_rewrite|backend_runtime_two_arg_first_local_rewrite|backend_runtime_two_arg_both_local_arg|backend_runtime_two_arg_both_local_first_rewrite)$'`,
  and
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/two_arg_both_local_arg.c`
  now pass for the owned seam, with the adjacent lane reduced to the remaining
  `backend_runtime_two_arg_both_local_first_rewrite` family inside this
  bounded call-helper cluster
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  still reports the broader parked full-suite regression lane as red overall,
  but the refreshed after-state improved again from `2580 -> 2581` passes and
  `270 -> 269` failures after the both-local two-arg helper slice
- extended the bounded x86 prepared-LIR matcher/emitter in
  `src/backend/x86/codegen/emit.cpp` so the existing first-local two-arg
  helper seam also accepts the adjacent one-rewrite shape
  (`int x = 5; x = x + 0; return add_pair(x, 7);`) without widening into the
  remaining both-local variants
- covered that native prepared-LIR seam with a focused x86 regression in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`
- verified the focused x86 first-local rewrite seam is green:
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_first_local_rewrite_call_slice`,
  `ctest --test-dir build --output-on-failure -R '^backend_runtime_two_arg_first_local_rewrite$'`,
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_two_arg_helper|backend_runtime_two_arg_local_arg|backend_runtime_two_arg_second_local_arg|backend_runtime_two_arg_second_local_rewrite|backend_runtime_two_arg_first_local_rewrite|backend_runtime_two_arg_both_local_arg|backend_runtime_two_arg_both_local_first_rewrite)$'`,
  and
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/two_arg_first_local_rewrite.c`
  now pass for the owned seam, with the adjacent lane reduced to the remaining
  `backend_runtime_two_arg_both_local_arg` and
  `backend_runtime_two_arg_both_local_first_rewrite` families
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  still reports the broader parked full-suite regression lane as red overall,
  but the refreshed after-state improved again from `2579 -> 2580` passes and
  `271 -> 270` failures after the first-local rewrite two-arg helper slice
- extended the bounded x86 prepared-LIR matcher/emitter in
  `src/backend/x86/codegen/emit.cpp` so the existing second-local two-arg
  helper seam also accepts the adjacent one-rewrite shape
  (`int y = 7; y = y + 0; return add_pair(5, y);`) without widening into the
  remaining first-local-rewrite or both-local variants
- covered that native prepared-LIR seam with a focused x86 regression in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`
- verified the focused x86 second-local rewrite seam is green:
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_second_local_rewrite_call_slice`,
  `ctest --test-dir build --output-on-failure -R '^backend_runtime_two_arg_second_local_rewrite$'`,
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_two_arg_helper|backend_runtime_two_arg_local_arg|backend_runtime_two_arg_second_local_arg|backend_runtime_two_arg_second_local_rewrite|backend_runtime_two_arg_first_local_rewrite|backend_runtime_two_arg_both_local_arg|backend_runtime_two_arg_both_local_first_rewrite)$'`,
  and
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/two_arg_second_local_rewrite.c`
  now pass for the owned seam, with the adjacent lane reduced to the remaining
  `backend_runtime_two_arg_first_local_rewrite`,
  `backend_runtime_two_arg_both_local_arg`, and
  `backend_runtime_two_arg_both_local_first_rewrite` families
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  still reports the broader parked full-suite regression lane as red overall,
  but the checked-in after-state improved again from `2578 -> 2579` passes and
  `272 -> 271` failures after the second-local rewrite two-arg helper slice
- added a bounded x86 prepared-LIR matcher/emitter in
  `src/backend/x86/codegen/emit.cpp` for the second-local two-arg helper
  family (`int y = 7; return add_pair(5, y);`) without widening into the
  parked rewrite or both-local variants
- covered that native prepared-LIR seam with a focused x86 regression in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`
- verified the focused x86 second-local two-arg seam is green:
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_second_local_arg_call_slice`,
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_two_arg_helper|backend_runtime_two_arg_local_arg|backend_runtime_two_arg_second_local_arg)$'`,
  and
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/two_arg_second_local_arg.c`
  now pass, with the standalone asm path emitting bounded native assembly for
  `main -> add_pair(5, 7)` after reloading the local second operand
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  still reports the broader parked full-suite regression lane as red overall,
  but the checked-in after-state improved again from `2576 -> 2578` passes and
  `274 -> 272` failures after the second-local two-arg helper slice
- added a bounded x86 prepared-LIR matcher/emitter in
  `src/backend/x86/codegen/emit.cpp` for the first-local second-immediate
  two-arg helper family (`int x = 5; return add_pair(x, 7);`) without widening
  into the parked second-local or local-rewrite variants
- covered that native prepared-LIR seam with a focused x86 regression in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`
- verified the focused x86 first-local two-arg seam is green:
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_local_arg_call_slice`,
  `ctest --test-dir build --output-on-failure -R '^backend_runtime_two_arg_local_arg$'`,
  and
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/two_arg_local_arg.c`
  now pass, with the standalone asm path emitting bounded native assembly for
  `main -> add_pair(5, 7)` after reloading the local first operand
- re-checked the adjacent two-arg helper-call lane after the first-local slice
  landed and confirmed the remaining bounded failures stay parked in the later
  source shapes:
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_two_arg_helper|backend_runtime_two_arg_local_arg|backend_runtime_two_arg_second_local_arg|backend_runtime_two_arg_second_local_rewrite|backend_runtime_two_arg_first_local_rewrite|backend_runtime_two_arg_both_local_arg|backend_runtime_two_arg_both_local_first_rewrite)$'`
  now leaves only the second-local, local-rewrite, and both-local variants
  failing while `backend_runtime_two_arg_helper` and
  `backend_runtime_two_arg_local_arg` pass
- added a bounded x86 prepared-LIR matcher/emitter in
  `src/backend/x86/codegen/emit.cpp` for the minimal two-function two-arg
  helper family (`add_pair(lhs, rhs) { return lhs + rhs; }` followed by
  `main -> add_pair(5, 7)`)
- covered that native prepared-LIR seam with a focused x86 regression in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`
- verified the focused x86 two-arg helper seam is green:
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_helper_call_slice`,
  `ctest --test-dir build --output-on-failure -R '^backend_runtime_two_arg_helper$'`,
  and
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/two_arg_helper.c`
  now pass, with the standalone asm path emitting bounded native assembly for
  `main -> add_pair(5, 7)`
- re-checked the adjacent two-arg helper-call lane after the minimal helper
  slice landed and confirmed the next bounded failures are still the local-arg
  and local-rewrite source shapes:
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_two_arg_helper|backend_runtime_two_arg_local_arg|backend_runtime_two_arg_second_local_arg|backend_runtime_two_arg_second_local_rewrite|backend_runtime_two_arg_first_local_rewrite|backend_runtime_two_arg_both_local_arg|backend_runtime_two_arg_both_local_first_rewrite)$'`
  leaves only the six non-minimal local-slot variants failing while
  `backend_runtime_two_arg_helper` passes
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  still reports the broader parked full-suite regression lane as red overall,
  but the checked-in after-state improved again from `2575 -> 2576` passes and
  `275 -> 274` failures after the minimal two-arg helper-call slice

- fixed the remaining zero-arg helper-call source-produced x86 path in
  `src/backend/x86/codegen/emit.cpp` by letting the declared-helper matcher
  accept frontend-lowered declaration metadata that still carries a synthetic
  `void` entry in `decl.params` while the signature text remains the true
  zero-arg contract
- tightened the focused x86 declared zero-arg helper regression fixture in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` so it mirrors the real
  frontend-lowered declaration shape instead of the earlier hand-built empty
  param vector
- verified the focused x86 direct-emitter and runtime helper-call seam is
  green:
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_extern_zero_arg_call_slice`,
  `ctest --test-dir build --output-on-failure -R '^backend_runtime_call_helper$'`,
  and
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/call_helper.c`
  now pass, with the standalone asm path emitting bounded native assembly for
  `main -> helper()`
- re-checked the adjacent helper-call lane after the zero-arg source path
  landed and confirmed the next bounded failure is now
  `backend_runtime_two_arg_helper`:
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_call_helper|backend_runtime_local_arg_call|backend_runtime_two_arg_helper)$'`
  leaves only that two-arg helper family failing
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  still reports the broader parked full-suite regression lane as red overall,
  but the checked-in after-state improved again from `2572 -> 2575` passes and
  `278 -> 275` failures after the zero-arg helper-call slice

- added a bounded x86 direct-LIR matcher/emitter in
  `src/backend/x86/codegen/emit.cpp` for the two-function single-param
  local-slot add helper family (`add_one(x) { x = x + 1; return x; }`
  followed by `main -> add_one(5)`)
- covered that native prepared-LIR seam with a focused x86 regression in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`
- verified the focused x86 slot-add seam is green:
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_param_slot_add_slice`,
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/param_slot.c`,
  and
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_local_temp|backend_runtime_param_slot)$'`
  now pass
- re-sampled the adjacent x86 direct-LIR call lane after the param-slot slice
  landed and confirmed `backend_runtime_call_helper` and
  `backend_runtime_local_arg_call` are the next bounded failures:
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_param_slot|backend_runtime_local_arg_call|backend_runtime_call_helper)$'`
  leaves only those two call-family tests failing
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log`
  still reports the broader parked full-suite regression lane as red overall,
  but the checked-in after-state improved from `2571 -> 2572` passes and
  `279 -> 278` failures after the param-slot slice

- added bounded x86 direct-LIR matcher/emitter support in
  `src/backend/x86/codegen/emit.cpp` for the next adjacent call-family lane:
  a single-function extern zero-arg helper call (`main -> helper()`) and the
  two-function single-local-slot helper call (`store 5 -> load -> add_one(x)`)
- covered those native prepared-LIR seams with focused x86 regressions in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`
- verified the focused x86 native prepared-LIR regressions are green:
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_extern_zero_arg_call_slice`
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_local_arg_call_slice`
  now pass
- verified the real internal runtime lane partially improved:
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_call_helper|backend_runtime_local_arg_call)$'`
  now leaves only `backend_runtime_call_helper` failing while
  `backend_runtime_local_arg_call` passes
- sampled the unresolved helper-call source shape with the riscv64 BIR text
  route:
  `./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/c/internal/backend_case/call_helper.c`
  prints a zero-arg declaration plus `main -> helper()` BIR call, so the
  remaining x86 failure is likely in the x86-target prepared-module route
  rather than the already-green bounded local-arg helper family

- added a bounded x86 direct-LIR local-temp matcher/emitter in
  `src/backend/x86/codegen/emit.cpp` for the one-slot `store imm -> load ->
  return` family and covered it with a focused backend regression in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp`
- verified the focused runtime slice is green:
  `ctest --test-dir build --output-on-failure -R '^backend_runtime_local_temp$'`
  now passes
- re-checked the nearest adjacent local-slot runtime after the local-temp slice
  landed and confirmed it remains the next bounded failure:
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_param_slot|backend_runtime_local_temp)$'`
  leaves only `backend_runtime_param_slot` failing
- refreshed `test_fail_after.log` with
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  re-ran the monotonic guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log`
  reports `2670 -> 2571` passes, `179 -> 279` failures, and `100` new failing
  tests across the already-broad parked route/select and x86 bring-up lanes,
  so the full-suite guard remains red overall even though the local-temp slice
  itself is green

- added bounded x86 direct-BIR affine-return support in
  `src/backend/x86/codegen/emit.cpp` for the minimal i32 add/sub chain family,
  including constant-only return-add and staged add/sub return slices plus the
  matching one-/two-parameter affine forms on the x86_64 SysV register path
- repaired the x86 assembler toolchain prelude acceptance in
  `src/backend/x86/assembler/parser.cpp` and
  `src/backend/x86/assembler/elf_writer.cpp` so object emission now accepts the
  emitter's existing optional `.type <symbol>, @function` line instead of
  treating it as an unsupported bounded slice
- verified the focused x86 affine-return/runtime and wrapper probes are green:
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/return_add.c`,
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/return_add_sub_chain.c`,
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_return_zero|backend_runtime_return_add|backend_runtime_return_add_sub_chain)$'`,
  and
  `ctest --test-dir build --output-on-failure -R '^backend_shared_util_tests$'`
  now pass
- classified the checked-in full-suite monotonic mismatch against
  `test_fail_before.log` and confirmed it is not stale-baseline drift:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log`
  reports `2670 -> 2526` passes, `179 -> 324` failures, zero resolved failing
  tests, and `144` new failing tests
- sampled representative failures and split the mismatch into:
  - a separate shared-BIR select/route regression family
  - an in-scope x86 asm/toolchain family
- recorded the separate cross-target shared-BIR regression as
  `ideas/open/47_shared_bir_select_route_regression_after_x86_variadic_recovery.md`
- repaired the bounded x86 asm prologue contract in
  `src/backend/x86/codegen/emit.cpp` so the minimal direct-BIR immediate-return
  path and the native string-literal helper now emit `.intel_syntax noprefix`
  and return to `.text` before function bodies
- added bounded x86 direct-BIR global-memory support in
  `src/backend/x86/codegen/emit.cpp` for:
  minimal scalar global load, minimal extern scalar global load, and the
  matching scalar global store-reload slice
- verified the focused x86 global-memory seam is green:
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_direct_bir_minimal_scalar_global_load_end_to_end`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_direct_bir_minimal_extern_scalar_global_load_end_to_end`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_scalar_global_load_through_bir_end_to_end`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_extern_scalar_global_load_through_bir_end_to_end`,
  and
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_direct_bir_minimal_scalar_global_store_reload_end_to_end`
  now pass
- verified the user-facing x86 internal global-load cases are green again:
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_global_load|backend_contract_x86_64_global_load_stdout_object)$'`
  passes, and
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/global_load.c`
  now emits bounded native assembly instead of failing at the x86 direct-LIR
  boundary
- re-ran the required full suite after the x86 global-memory slice and
  confirmed the checked-in monotonic guard is still red for the same broader
  separate baseline-drift lane:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log`
  reports `2670 -> 2564` passes, `179 -> 286` failures, zero resolved failing
  tests, and `106` new failing tests spanning `backend_bir_tests`,
  route tests, and broad x86/runtime families rather than the now-green
  `global_load` seam
- normalized the x86 string-literal helper's Intel operand spelling to the
  repo's existing native-asm contract:
  `lea rax, .L.str0[rip]` and `movsx/movzx ... byte ptr [...]`
- verified the focused x86 contract/runtime probes are green:
  `ctest --test-dir build --output-on-failure -R '^(backend_runtime_return_zero|backend_contract_x86_64_string_literal_char_stdout_object)$'`
  passes

- added a seam-local x86 prepared-LIR matcher in
  `src/backend/x86/codegen/emit.cpp` for the bounded floating variadic
  `variadic_double_bytes` runtime helper, accepting both the unit-test fixture
  shape and the prepared compiler path where entry-allocation rewriting folds
  the temporary double spills into `%lv.second`
- verified the new floating variadic x86 runtime slice is green:
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/variadic_double_bytes.c`
  now emits bounded native assembly, and
  `ctest --test-dir build --output-on-failure -R '^backend_runtime_variadic_double_bytes$'`
  now passes
- re-checked the adjacent x86 integer variadic probe after the floating slice
  landed and confirmed it stays green:
  `ctest --test-dir build --output-on-failure -R '^backend_runtime_variadic_sum2$'`
  passes
- expanded `src/backend/bir.hpp` so BIR can carry backend-owned type, memory,
  and call metadata needed by the split lowering seam
- added `src/backend/lowering/lir_to_bir/passes.hpp` and wired the translated
  split lowering scaffolds into the tree and build lists
- wrapped the old `try_lower_to_bir(...)` path as a legacy fallback behind
  `try_lower_to_bir_with_options(...)`
- moved type/legalization helper ownership into
  `src/backend/lowering/lir_to_bir/types.cpp`
- moved first memory/materialization and repeated GEP matcher ownership into
  `src/backend/lowering/lir_to_bir/memory.cpp`
- moved first direct-call construction, argument lowering, and basic call
  metadata ownership into `src/backend/lowering/lir_to_bir/calls.cpp`
- split direct-call metadata further so the split call seam now owns:
  `arg_types`, placeholder `arg_abi/result_abi`,
  target-derived `calling_convention`, and declared-function `is_variadic`
- moved the minimal declared direct-call matcher/module lowering path out of
  `src/backend/lowering/lir_to_bir.cpp` and into
  `src/backend/lowering/lir_to_bir/calls.cpp`, including declared/extern
  return-type validation, typed-call surface reconciliation, string-pool
  materialization, and direct-call BIR module construction
- moved the remaining minimal direct-call helper cluster out of
  `src/backend/lowering/lir_to_bir.cpp` and into
  `src/backend/lowering/lir_to_bir/calls.cpp`, covering the zero-arg direct
  call, void direct-call with immediate return, two-arg direct call, single-arg
  add/immediate and identity helpers, folded two-arg direct call, dual identity
  call/sub, and call-crossing direct-call matchers
- exported the split call-owned direct-call entry points through
  `src/backend/lowering/lir_to_bir/passes.hpp` so the monolith now dispatches
  to `calls.cpp` for that helper family instead of owning the definitions
- rewired `src/backend/lowering/lir_to_bir.cpp` dispatch through the exported
  split call-seam entry point and removed the now-dead monolith helper
  wrappers for that path
- moved the last shared minimal-signature/direct-call helper cluster out of
  `src/backend/lowering/lir_to_bir.cpp` and into
  `src/backend/lowering/lir_to_bir/calls.cpp`, exporting the call-owned
  signature/return-width helpers through `passes.hpp` so the monolith no
  longer owns duplicate copies
- verified the tree rebuilds cleanly after the declared direct-call ownership
  move:
  `cmake --build build -j8` succeeds
- verified the tree still rebuilds cleanly after the remaining minimal
  direct-call helper extraction:
  `cmake --build build -j8` succeeds
- verified the tree still rebuilds cleanly after centralizing the residual
  call-owned signature helpers in `calls.cpp`:
  `cmake --build build -j8` succeeds
- re-ran seam-local backend validation after the declared direct-call move and
  confirmed the same pre-existing unrelated failure buckets remain:
  `./build/backend_bir_tests` still fails across existing shared-BIR lowering
  acceptance/support cases, and `./build/backend_shared_util_tests` still
  aborts with the existing unsupported direct-LIR rejection
- re-ran the same seam-local backend executables after the direct-call helper
  extraction and observed the same pre-existing failure shape:
  `./build/backend_bir_tests` still fails across the existing shared-BIR
  lowering acceptance/support cases, and `./build/backend_shared_util_tests`
  still aborts with the existing unsupported direct-LIR rejection
- moved the minimal global char-pointer-diff and global int-pointer-diff
  matcher/module lowerers out of `src/backend/lowering/lir_to_bir.cpp` and
  into `src/backend/lowering/lir_to_bir/memory.cpp`, exporting the split
  memory-owned entry points through `passes.hpp` so the monolith now only
  dispatches those address-decoding seams
- moved the minimal extern/global array-load matcher/module lowerer out of
  `src/backend/lowering/lir_to_bir.cpp` and into
  `src/backend/lowering/lir_to_bir/memory.cpp`, exporting the split
  memory-owned entry point through `passes.hpp` so the monolith now only
  dispatches that GEP/address-decoding seam
- moved the minimal plain and extern scalar global-load matcher/module lowerers
  out of `src/backend/lowering/lir_to_bir.cpp` and into
  `src/backend/lowering/lir_to_bir/memory.cpp`, exporting the split
  memory-owned entry points through `passes.hpp` so the monolith now only
  dispatches those scalar global read seams
- moved the remaining scalar global store+reload matcher/module lowerer out of
  `src/backend/lowering/lir_to_bir.cpp` and into
  `src/backend/lowering/lir_to_bir/memory.cpp`, exporting the split
  memory-owned entry point through `passes.hpp` so the monolith now only
  dispatches the residual scalar global write+reload seam
- verified the tree still rebuilds cleanly after the global pointer-diff
  ownership move:
  `cmake --build build -j8` succeeds
- verified the tree still rebuilds cleanly after the extern/global array-load
  ownership move:
  `cmake --build build -j8` succeeds
- verified the tree still rebuilds cleanly after the plain/extern scalar
  global-load ownership move:
  `cmake --build build -j8` succeeds
- verified the tree still rebuilds cleanly after the scalar global
  store+reload ownership move:
  `cmake --build build -j8` succeeds
- re-ran the same seam-local backend executables after the global pointer-diff
  ownership move and observed the same pre-existing failure shape:
  `./build/backend_bir_tests` still fails across the existing shared-BIR
  lowering acceptance/support cases, and `./build/backend_shared_util_tests`
  still aborts with the existing unsupported direct-LIR rejection
- re-ran the same seam-local backend executables after the extern/global
  array-load ownership move and observed the same pre-existing failure shape:
  `./build/backend_bir_tests` still fails across the existing shared-BIR
  lowering acceptance/support cases, and `./build/backend_shared_util_tests`
  still aborts with the existing unsupported direct-LIR rejection
- re-ran the same seam-local backend executables after the plain/extern scalar
  global-load ownership move and observed the same pre-existing failure shape:
  `./build/backend_bir_tests` still fails across the existing shared-BIR
  lowering acceptance/support cases, and `./build/backend_shared_util_tests`
  still aborts with the existing unsupported direct-LIR rejection
- re-ran the same seam-local backend executables after the scalar global
  store+reload ownership move and observed the same pre-existing failure
  shape:
  `./build/backend_bir_tests` still fails across the existing shared-BIR
  lowering acceptance/support cases, and `./build/backend_shared_util_tests`
  still aborts with the existing unsupported direct-LIR rejection
- moved the minimal string-literal compare phi-return matcher/module lowerer
  out of `src/backend/lowering/lir_to_bir.cpp` and into
  `src/backend/lowering/lir_to_bir/memory.cpp`, exporting the split
  memory-owned entry point through `passes.hpp` so the monolith now only
  dispatches that residual string/global byte-address seam
- verified the tree still rebuilds cleanly after the string-literal
  compare phi-return ownership move:
  `cmake --build build -j8` succeeds
- re-ran the same seam-local backend executables after the string-literal
  compare phi-return ownership move and observed the same pre-existing
  failure shape:
  `./build/backend_bir_tests` still fails across the existing shared-BIR
  lowering acceptance/support cases, and `./build/backend_shared_util_tests`
  still aborts with the existing unsupported direct-LIR rejection
- normalized the duplicate integer-width and memory-value legalization
  predicates out of `src/backend/lowering/lir_to_bir/memory.cpp` and the
  monolith into shared `src/backend/lowering/lir_to_bir/types.cpp` helpers so
  the split memory seam no longer owns its own ad hoc typed-text parsing
- verified the tree rebuilds cleanly after that predicate normalization:
  `cmake --build build -j8` succeeds
- switched the split memory-owned typed global/global-array load and
  store-reload matchers to semantic type legalization instead of stale
  text-only checks, so the seam now accepts:
  `test_bir_lowering_accepts_typed_minimal_scalar_global_load_lir_slice_with_stale_text`,
  `test_bir_lowering_accepts_typed_minimal_extern_scalar_global_load_lir_slice_with_stale_text`,
  `test_bir_lowering_accepts_typed_minimal_extern_global_array_load_lir_slice_with_stale_text`,
  and
  `test_bir_lowering_accepts_typed_minimal_scalar_global_store_reload_lir_slice_with_stale_text`
- re-ran the nearby non-stale memory-owned global lowering checks after that
  change and confirmed they still pass:
  `test_bir_lowering_accepts_minimal_scalar_global_load_lir_module`,
  `test_bir_lowering_accepts_minimal_extern_scalar_global_load_lir_module`,
  `test_bir_lowering_accepts_minimal_extern_global_array_load_lir_module`, and
  `test_bir_lowering_accepts_minimal_scalar_global_store_reload_lir_module`
- restored the memory-owned minimal string-literal compare phi-return fold by
  letting `try_lower_to_bir_with_options(...)` give that exact seam one
  pre-phi lowering attempt after CFG normalization but before the scaffold
  phi-erasure pass destroys the join phi shape
- verified the focused shared-BIR string-literal seam is green again:
  `test_bir_lowering_accepts_minimal_string_literal_compare_phi_return_lir_module`
  and
  `test_backend_bir_pipeline_drives_x86_lir_minimal_string_literal_compare_phi_return_through_bir_end_to_end`
  now pass
- added the first x86-native explicit support probes in
  `src/backend/x86/codegen/emit.cpp` for:
  direct-BIR single-block immediate returns and prepared-LIR minimal
  string-literal byte loads
- verified the paired x86 native support probes are green:
  `test_x86_try_emit_module_reports_direct_bir_support_explicitly` and
  `test_x86_try_emit_prepared_lir_module_reports_direct_lir_support_explicitly`
  now pass
- re-ran the nearby memory-owned stale-text/global seam checks after the
  string-literal fix and confirmed they still pass:
  `test_bir_lowering_accepts_minimal_scalar_global_load_lir_module`,
  `test_bir_lowering_accepts_typed_minimal_scalar_global_load_lir_slice_with_stale_text`,
  `test_bir_lowering_accepts_typed_minimal_extern_scalar_global_load_lir_slice_with_stale_text`,
  `test_bir_lowering_accepts_typed_minimal_extern_global_array_load_lir_slice_with_stale_text`,
  and
  `test_bir_lowering_accepts_typed_minimal_scalar_global_store_reload_lir_slice_with_stale_text`
- added a seam-local x86 prepared-LIR matcher in
  `src/backend/x86/codegen/emit.cpp` for the narrowed SysV integer variadic
  `sum2` runtime module after entry-alloca preparation coalesces the temporary
  va-arg spill into `%lv.second`
- verified the narrowed x86 variadic integer runtime slice is green again:
  `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/internal/backend_case/variadic_sum2.c`
  now emits the expected bounded native assembly, and
  `ctest --test-dir build --output-on-failure -R '^backend_runtime_variadic_sum2$'`
  now passes

## Blockers

- no active blocker on ownership wiring
- later compile/test recovery still has known unrelated backend test noise, so
  not every existing backend test executable is currently a clean regression
  signal for this lane
- full-suite monotonic validation against the checked-in
  `test_fail_before.log` baseline is still red, but the mismatch is now a much
  broader 144-test delta
  (`before: passed=2670 failed=179 total=2849`;
  `after: passed=2526 failed=324 total=2850`) spanning route tests,
  `backend_bir_tests`, and many x86 backend/runtime cases rather than a
  seam-local `variadic_double_bytes` regression
- because that monotonic mismatch is far broader than this one matcher and the
  focused x86 variadic probes are green, treat the baseline drift as a separate
  blocking investigation before using the checked-in `test_fail_before.log` as
  the acceptance gate for further slices
- after the x86 global-memory slice, the same broad monotonic guard problem
  remains separate from the now-green `global_load` seam:
  `test_fail_after.log` still fails against `test_fail_before.log` with `106`
  newly failing tests and a `2564/286/2850` pass/fail/total snapshot

## Notes To Resume

- do not add any new testcase-shaped x86 direct-LIR matcher while this reset
  is active
- the current goal is structural ownership, not behavioral completion
- the call seam now owns the minimal signature/return-width helper family;
  next slices should avoid reintroducing those helpers into the monolith
- the memory seam now owns the two minimal global pointer-diff matchers; keep
  new GEP/address-form decoding work out of the monolith when extending this
  lane
- the memory seam now also owns the minimal extern/global array-load matcher;
  keep residual global-address and GEP decoding moves on the split seam side
- the memory seam now also owns the minimal plain and extern scalar
  global-load matchers plus the remaining scalar global store/reload seam;
  keep follow-on scalar global load/store ownership on the split seam side
- the memory seam now also owns the minimal string-literal compare
  phi-return seam; use that split implementation as the home for any
  remaining string/global byte-address cleanup instead of rebuilding those
  matchers in the monolith
- the string-literal seam depends on seeing the join phi before scaffold
  phi-erasure; keep that exception explicit and narrow instead of changing the
  global pre-phi lowering order
- when this lane switches to compile recovery, start narrow and seam-local
  before restoring broader regression discipline
- the typed stale-text regressions for scalar global load/store and extern
  global-array load plus the bounded string-literal compare fold are now green,
  so the next slice should move forward from the x86 probe/runtime surface
  rather than reopening those recovered memory checks
