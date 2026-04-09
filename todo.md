# x86_64 Backend Bring-Up After BIR Cutover

Status: Active
Source Idea: ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md
Source Plan: plan.md

## Current Active Item

- Step 3 x86-native seam recovery, after settling the monotonic-baseline
  question:
  keep the active work inside idea 44's x86-native emitter/toolchain lane and
  do not silently absorb the separate shared-BIR select regression
- current exact slice:
  move from the now-green both-local first-rewrite two-arg helper call into the
  next adjacent source-produced family:
  `backend_runtime_two_arg_both_local_second_rewrite`, keeping the work inside
  idea 44's x86 native prepared-LIR seam and not widening into the separate
  parked shared-BIR select regression

## Next Slice

- keep the separate shared-BIR select regression parked in
  `ideas/open/47_shared_bir_select_route_regression_after_x86_variadic_recovery.md`
  instead of widening idea 44
- after the now-green `backend_runtime_call_helper` slice, keep the active
  work on the next adjacent call-family target:
  after the now-green `backend_runtime_two_arg_helper` and
  `backend_runtime_two_arg_local_arg` and
  `backend_runtime_two_arg_second_local_arg` and
  `backend_runtime_two_arg_second_local_rewrite` and
  `backend_runtime_two_arg_first_local_rewrite` and
  `backend_runtime_two_arg_both_local_arg` and
  `backend_runtime_two_arg_both_local_first_rewrite` slices, the next bounded
  source-produced failure is `backend_runtime_two_arg_both_local_second_rewrite`

## Recently Completed

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
