# x86_64 Translated Codegen Integration

Status: Active
Source Idea: ideas/open/43_x86_64_peephole_pipeline_completion.md
Source Plan: plan.md

## Current Active Item

- Step 3 translated-owner cutover for the remaining direct dispatcher surface
  around the x86 entry/support helper surface after the legacy matcher body
  removal in `src/backend/x86/codegen/emit.cpp`
- immediate target:
  continue the translated support dependency inventory in the already-built x86
  support unit by exposing the next ref-owned helper family through
  `src/backend/x86/codegen/mod.cpp` and `x86_codegen.hpp`
  - keep focused shared-util coverage on the translated helper contract
  - keep validation centered on backend-only targets while this remains a
    support-surface slice, not a full translated prologue compile-in

## Next Slice

- continue the translated dependency inventory with the next already-built
  helper family from `ref/.../emit.rs`, likely the remaining inline-asm /
  prologue support mappings that can land without pulling `prologue.cpp` into
  the build yet
- only rerun the broad monotonic guard after a larger owner-path cutover lands

## Current Iteration Notes

- this iteration exposed the ref-owned x86 ALU/shift mnemonic helpers through
  `src/backend/x86/codegen/mod.cpp` and `x86_codegen.hpp`:
  `x86_alu_mnemonic(...)` and `x86_shift_mnemonic(...)`
- `src/backend/x86/codegen/alu.cpp` now consumes that shared helper surface
  instead of keeping private local shift-mnemonic helpers, keeping the future
  translated ALU owner contract anchored in the already-built support unit
- added focused shared-util coverage that locks the translated add/sub/and/or/xor
  plus 32-bit/64-bit shift mnemonic mapping contract to the ref emitter shape
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests test_x86_translated_asm_emitter_helpers_match_shared_contract`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`

- this iteration exposed the translated x86 register-pool and inline-asm
  callee-saved mapping helpers through `src/backend/x86/codegen/mod.cpp` and
  `x86_codegen.hpp`:
  `x86_callee_saved_regs()`, `x86_caller_saved_regs()`,
  `x86_constraint_to_callee_saved(...)`, and
  `x86_clobber_name_to_callee_saved(...)`
- added focused shared-util coverage that locks the ref callee-saved /
  caller-saved pool order plus the explicit-register and `'b'`-constraint
  mapping contract for future translated prologue/regalloc ownership work
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests test_x86_translated_asm_emitter_helpers_match_shared_contract`,
  `./build/backend_shared_util_tests test_x86_direct_dispatch_owner_handles_helper_backed_prepared_lir_slice`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`

- this iteration exposed the ref-owned x86 physreg/register-name helpers
  through `src/backend/x86/codegen/mod.cpp` and `x86_codegen.hpp`:
  `reg_name_to_32(...)`, `phys_reg_name(...)`, and
  `phys_reg_name_32(...)`
- added focused shared-util coverage that locks the translated physreg and
  sub-register naming contract to the ref emitter shape without widening the
  public x86 header dependency surface
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests backend_bir_tests`,
  `./build/backend_shared_util_tests test_x86_translated_asm_emitter_helpers_match_shared_contract`

- this iteration moved the shared x86 asm string/symbol/prelude helpers out of
  `src/backend/x86/codegen/emit.cpp` into the already-built support unit
  `src/backend/x86/codegen/mod.cpp`, and the direct helper owners now reuse
  that shared surface instead of carrying duplicate local helper definitions
- touched direct helper owners:
  `src/backend/x86/codegen/direct_calls.cpp`,
  `src/backend/x86/codegen/direct_globals.cpp`,
  `src/backend/x86/codegen/direct_locals.cpp`,
  `src/backend/x86/codegen/direct_loops.cpp`,
  `src/backend/x86/codegen/direct_printf.cpp`,
  `src/backend/x86/codegen/direct_returns.cpp`,
  `src/backend/x86/codegen/direct_variadic.cpp`, and
  `src/backend/x86/codegen/direct_void.cpp`
- added focused shared-util coverage for the shared asm helper contract through
  `test_x86_translated_asm_emitter_helpers_match_shared_contract`
- focused validation passed:
  `cmake --build build -j8 --target backend_shared_util_tests backend_bir_tests`,
  `./build/backend_shared_util_tests test_x86_translated_asm_emitter_helpers_match_shared_contract`,
  `./build/backend_shared_util_tests test_x86_direct_dispatch_owner_handles_helper_backed_prepared_lir_slice`,
  `./build/backend_bir_tests test_x86_direct_printf_helper_accepts_repeated_printf_immediates_slice`,
  `./build/backend_bir_tests test_x86_direct_local_helper_accepts_local_temp_slice`,
  `./build/backend_bir_tests test_x86_direct_variadic_helper_accepts_variadic_sum2_runtime_slice`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_param_slot_add_slice`,
  `./build/backend_bir_tests test_x86_direct_void_helper_accepts_void_direct_call_return_slice`, and
  `./build/backend_bir_tests test_x86_direct_bir_return_helper_accepts_affine_return_slice`
- broad validation rerun stayed matched against the current-tree baseline:
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after_rerun.log 2>&1`
  plus
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_matched_before.log --after test_fail_after_rerun.log --allow-non-decreasing-passed`
  reported `3194` passed / `182` failed before and after, with no newly
  failing tests and no new `>30s` cases
- this iteration moved the remaining direct-BIR immediate/affine-return
  parse/asm owner out of `src/backend/x86/codegen/emit.cpp` into the new
  sibling owner file `src/backend/x86/codegen/direct_returns.cpp` through
  `try_emit_minimal_affine_return_module(...)`
- the direct-BIR helper dispatcher in
  `src/backend/x86/codegen/direct_dispatch.cpp` now routes the bounded
  immediate/affine-return slice through that sibling owner file, and
  `emit.cpp` no longer keeps the local return matcher ladder for either direct
  BIR or prepared-LIR helper-first probing
- added focused backend coverage that calls the moved direct-BIR return helper
  explicitly, plus a dispatcher-visible regression so the Step 3 ownership move
  stays observable outside the public `try_emit_module(...)` surface
- focused validation passed:
  `cmake --build build -j8 --target backend_shared_util_tests backend_bir_tests`,
  `./build/backend_shared_util_tests test_x86_direct_dispatch_owner_handles_affine_return_bir_slice`,
  `./build/backend_bir_tests test_x86_direct_bir_return_helper_accepts_affine_return_slice`,
  `./build/backend_bir_tests test_x86_try_emit_module_reports_direct_bir_support_explicitly`,
  `./build/backend_shared_util_tests test_x86_direct_dispatch_owner_handles_helper_backed_bir_slice`,
  `./build/backend_bir_tests test_x86_public_bir_emitter_delegates_direct_bir_route_to_shared_backend`,
  `./build/backend_bir_tests test_x86_try_emit_prepared_lir_module_reports_direct_lir_support_explicitly`, and
  `./build/backend_shared_util_tests test_x86_direct_dispatch_owner_handles_local_temp_prepared_lir_slice`
- this iteration moved the bounded direct-BIR countdown-loop parse/asm owner
  out of `src/backend/x86/codegen/emit.cpp` into the new sibling owner file
  `src/backend/x86/codegen/direct_loops.cpp` through
  `try_emit_minimal_countdown_loop_module(...)`
- the direct-BIR helper dispatcher in
  `src/backend/x86/codegen/direct_dispatch.cpp` now routes the countdown-loop
  slice through that sibling owner file, and `emit.cpp` no longer keeps the
  local countdown-loop matcher or asm emitter
- added focused backend coverage that calls the moved countdown-loop helper
  explicitly so the Step 3 BIR ownership move stays observable outside the
  end-to-end x86 pipeline tests
- focused validation passed:
  `cmake --build build -j8 --target backend_shared_util_tests backend_bir_tests`,
  `./build/backend_shared_util_tests`,
  `./build/backend_bir_tests test_x86_direct_bir_helper_accepts_minimal_countdown_loop_slice`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_direct_bir_minimal_countdown_loop_end_to_end`, and
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_countdown_loop_through_bir_end_to_end`
- this iteration moved the prepared-LIR `constant_branch` and `local_temp`
  parse/asm helpers out of `src/backend/x86/codegen/emit.cpp` into the new
  sibling owner file `src/backend/x86/codegen/direct_locals.cpp` through
  `try_emit_minimal_constant_branch_return_module(...)` and
  `try_emit_minimal_local_temp_module(...)`
- `emit.cpp` now delegates those two prepared-LIR slices through sibling owner
  exports instead of keeping their parser and asm emission logic locally
- the prepared-LIR helper dispatcher in
  `src/backend/x86/codegen/direct_dispatch.cpp` now routes those bounded local
  slices through the new sibling owner file as well
- added focused backend coverage that calls both moved local helpers
  explicitly plus a shared-util dispatcher regression that proves the prepared
  helper ladder still reaches the new local owner path
- focused validation passed:
  `cmake --build build -j8 --target backend_shared_util_tests backend_bir_tests`,
  `./build/backend_shared_util_tests`,
  `./build/backend_bir_tests test_x86_direct_local_helper_accepts_local_temp_slice`,
  `./build/backend_bir_tests test_x86_direct_local_helper_accepts_constant_branch_return_slice`, and
  `./build/backend_bir_tests test_x86_try_emit_prepared_lir_module_reports_direct_lir_support_explicitly`
- broad validation rerun stayed matched against the current-tree baseline:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_matched_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  reported `3194` passed / `182` failed before and after, with no newly
  failing tests and no new `>30s` cases
- this iteration moved the remaining helper-route dispatch ladders out of
  `src/backend/x86/codegen/emit.cpp` into the new sibling owner file
  `src/backend/x86/codegen/direct_dispatch.cpp` through
  `try_emit_direct_bir_helper_module(...)` and
  `try_emit_direct_prepared_lir_helper_module(...)`
- `emit.cpp` now keeps the public x86 entrypoints plus the still-local
  parse/asm helpers, while the helper-backed BIR and prepared-LIR route
  ladders no longer live in that legacy owner file
- added focused shared-util coverage that calls the new dispatcher owner
  helpers explicitly for one helper-backed BIR route and one helper-backed
  prepared-LIR route so the ownership move stays observable outside the
  end-to-end backend tests
- focused validation passed:
  `cmake --build build -j8 --target backend_shared_util_tests backend_bir_tests`,
  `./build/backend_shared_util_tests`,
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`,
  `./build/backend_bir_tests test_x86_try_emit_prepared_lir_module_reports_direct_lir_support_explicitly`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_direct_bir_minimal_scalar_global_store_reload_end_to_end`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_direct_bir_global_store_return_and_entry_return_end_to_end`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_param_slot_add_slice`, and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_source_00080_void_helper_call_slice`
- focused validation note:
  `./build/backend_bir_tests test_x86_try_emit_module_reports_direct_bir_support_explicitly`
  still fails on the current tree because its "unsupported" fixture is the
  two-function immediate-return module from `make_unsupported_multi_function_bir_module()`,
  which the long-standing local parser in `emit.cpp` already accepts; this run
  did not modify that parse-backed path
- this iteration moved the remaining bounded scalar-global BIR owner routes
  out of `src/backend/x86/codegen/emit.cpp` into
  `src/backend/x86/codegen/direct_globals.cpp` through the new
  `try_emit_minimal_scalar_global_store_reload_module(...)` and
  `try_emit_minimal_global_store_return_and_entry_return_module(...)` helpers
- added focused shared-util coverage that calls those new direct-globals
  helpers explicitly so the ownership move stays observable outside the
  end-to-end backend pipeline tests
- focused validation passed:
  `cmake --build build -j8 --target backend_shared_util_tests backend_bir_tests`,
  `./build/backend_shared_util_tests`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_direct_bir_minimal_scalar_global_store_reload_end_to_end`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_direct_bir_global_store_return_and_entry_return_end_to_end`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation rerun stayed matched against the current-tree baseline:
  `test_fail_matched_before.log` versus `test_fail_after_rerun.log` both report
  `95% tests passed, 182 tests failed out of 3376`, with no newly failing
  tests and no fixed failures in the failure-set diff
- this plan was activated by explicit user priority override
- idea 44 remains open as the parked shared-BIR cleanup and legacy-matcher
  consolidation lane
- user redirected the execution strategy on 2026-04-10:
  stop optimizing for slow `emit.cpp` seam-by-seam extraction and instead aim
  to delete the remaining x86 `emit.cpp` legacy ownership by porting from
  `ref/claudes-c-compiler/src/backend/x86/codegen/emit.rs`
- user also clarified the validation boundary on 2026-04-10:
  backend-only emitter work should be gated primarily by backend regression
  coverage because frontend validation runs through LLVM IR and is not part of
  this backend ownership switch
- user further clarified on 2026-04-10 that the regression-guard rule should
  not block the legacy-removal phase itself; finish the cutover first, then
  evaluate broad regression fallout afterward
- this iteration moved the bounded direct-BIR minimal scalar global-load
  matcher/dispatch out of `src/backend/x86/codegen/emit.cpp` into
  `src/backend/x86/codegen/globals.cpp` through the new
  `try_emit_minimal_scalar_global_load_module(...)` helper
- added focused shared-util coverage that calls the new globals-owner helper
  directly and checks that it emits the expected native global definition plus
  RIP-relative load sequence
- focused validation passed:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- this iteration moved the bounded direct-BIR extern scalar global-load
  matcher/dispatch out of `src/backend/x86/codegen/emit.cpp` into
  `src/backend/x86/codegen/globals.cpp` through the new
  `try_emit_minimal_extern_scalar_global_load_module(...)` helper
- added focused shared-util coverage that calls the new globals-owner helper
  directly and checks that it keeps the extern unresolved while emitting the
  expected RIP-relative load sequence
- focused checks passed:
  `cmake --build build -j8 --target backend_shared_util_tests backend_bir_tests`,
  `./build/backend_shared_util_tests`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_direct_bir_minimal_extern_scalar_global_load_end_to_end`, and
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_direct_bir_minimal_scalar_global_load_end_to_end`
- broad validation note:
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log 2>&1`
  completed, but the stored `test_fail_before.log` baseline is stale for the
  current tree shape (`1398` tests before versus `3376` after), so the
  monotonic guard reported one apparent new failure
- verified the reported delta is pre-existing and unrelated to this x86 globals
  slice:
  `cpp_eastl_memory_uses_allocator_parse_recipe` fails on both the patched
  tree and a clean `HEAD` worktree because
  `ref/EASTL/include/EASTL/internal/memory_uses_allocator.h` is absent from
  the current workspace inventory
- follow-up validation debt:
  regenerate a matched current-tree full-suite baseline before using the
  monotonic regression guard as a hard gate again
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests and the existing `backend_bir_tests`
  `>30s` timeout warning unchanged
- this iteration extends Step 4 with another bounded prepared-LIR sibling seam:
  the x86 `void` helper/call direct-LIR routes now live in
  `src/backend/x86/codegen/direct_void.cpp` instead of `emit.cpp`
- this iteration starts the neighboring integer helper-call cluster with a new
  bounded sibling seam: the x86 prepared-LIR `param_slot_add` route plus the
  zero-arg extern/declared-call route now live in
  `src/backend/x86/codegen/direct_calls.cpp` instead of `emit.cpp`
- this iteration extends that direct-calls sibling seam one bounded step
  further: the x86 prepared-LIR single-local helper-call route now lives in
  `src/backend/x86/codegen/direct_calls.cpp` instead of `emit.cpp`
- this iteration continues that direct-calls sibling seam with the next
  bounded ownership move: the x86 prepared-LIR immediate/immediate
  two-argument helper-call route now lives in
  `src/backend/x86/codegen/direct_calls.cpp` instead of staying in `emit.cpp`
- this iteration continues that direct-calls sibling seam with the next
  bounded ownership move: the x86 prepared-LIR first-local two-argument
  helper-call route now lives in `src/backend/x86/codegen/direct_calls.cpp`
  instead of staying in `emit.cpp`
- this iteration continues that direct-calls sibling seam with the next
  bounded ownership move: the x86 prepared-LIR second-local two-argument
  helper-call route now lives in `src/backend/x86/codegen/direct_calls.cpp`
  instead of staying in `emit.cpp`
- this iteration continues that direct-calls sibling seam with the next
  bounded ownership move: the x86 prepared-LIR second-local rewrite
  two-argument helper-call route now lives in
  `src/backend/x86/codegen/direct_calls.cpp` instead of staying in `emit.cpp`
- this iteration continues that direct-calls sibling seam with the next
  bounded ownership move: the x86 prepared-LIR first-local rewrite
  two-argument helper-call route now lives in
  `src/backend/x86/codegen/direct_calls.cpp` instead of staying in `emit.cpp`
- added a focused backend regression that calls the moved first-local rewrite
  two-arg helper seam explicitly so the Step 4 ownership move stays observable
  apart from the broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_first_local_rewrite_call_slice`
  plus `test_x86_direct_emitter_lowers_minimal_two_arg_first_local_rewrite_call_slice`
  and `test_x86_direct_call_helper_accepts_two_arg_second_local_rewrite_call_slice`
- added a focused backend regression that calls the moved second-local rewrite
  two-arg helper seam explicitly so the Step 4 ownership move stays observable
  apart from the broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_second_local_rewrite_call_slice`
  plus `test_x86_direct_emitter_lowers_minimal_two_arg_second_local_rewrite_call_slice`
  and `test_x86_direct_call_helper_accepts_two_arg_second_local_arg_call_slice`
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests and no new `>30s` cases
- added a focused backend regression that calls the moved second-local two-arg
  helper seam explicitly so the Step 4 ownership move stays observable apart
  from the broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_second_local_arg_call_slice`
  plus `test_x86_direct_call_helper_accepts_two_arg_local_arg_call_slice`,
  `test_x86_direct_emitter_lowers_minimal_two_arg_local_arg_call_slice`, and
  `test_x86_direct_emitter_lowers_minimal_two_arg_second_local_arg_call_slice`
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests and no new `>30s` cases
- added a focused backend regression that calls the moved first-local two-arg
  helper seam explicitly so the Step 4 ownership move stays observable apart
  from the broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_local_arg_call_slice`
  plus `test_x86_direct_emitter_lowers_minimal_two_arg_local_arg_call_slice`
  and `test_x86_direct_call_helper_accepts_two_arg_call_slice`
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests and the existing `backend_bir_tests`
  `>30s` warning unchanged
- added a focused backend regression that calls the moved two-argument helper
  seam explicitly so the Step 4 ownership move stays observable apart from the
  broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_call_slice`
  plus `test_x86_direct_emitter_lowers_minimal_two_arg_helper_call_slice` and
  `test_x86_direct_call_helper_accepts_local_arg_call_slice`
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests
- added a focused backend regression that calls the moved single-local helper
  seam explicitly so the Step 4 ownership move stays observable apart from the
  broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_local_arg_call_slice`
  plus `test_x86_direct_call_helper_accepts_param_slot_add_slice` and
  `test_x86_try_emit_prepared_lir_module_reports_direct_lir_support_explicitly`
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests
- added a focused backend regression that calls the new direct call-helper seam
  explicitly so the Step 4 ownership move stays observable apart from the
  broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_param_slot_add_slice`
  plus `test_x86_direct_emitter_lowers_minimal_param_slot_add_slice` and
  `test_x86_direct_emitter_lowers_minimal_extern_zero_arg_call_slice`
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests and the existing `backend_bir_tests`
  `>30s` warning unchanged
- the moved seam covers the helper-only `voidfn` body, the two-function
  `voidfn(); return imm;` slice, and the main-only extern-void call plus
  immediate return shape without widening the surrounding integer helper-call
  ownership surface yet
- added a focused backend regression that calls the new direct `void` helper
  seam explicitly so the Step 4 ownership move stays observable apart from the
  broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_void_helper_accepts_void_direct_call_return_slice`
  plus the existing `00080` helper-only, helper-call, and main-only direct
  x86 emitter regressions
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests
- this iteration extends the direct-printf sibling seam one bounded step
  further: the `string_literal_char` direct-LIR route now lives in
  `src/backend/x86/codegen/direct_printf.cpp` instead of `emit.cpp`
- added a focused backend regression that calls the moved direct
  `string_literal_char` helper seam explicitly so the Step 4 ownership move
  stays observable apart from the broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_printf_helper_accepts_string_literal_char_slice`
  plus `test_x86_try_emit_prepared_lir_module_reports_direct_lir_support_explicitly`
  and the existing repeated/local-buffer/counting direct-printf helper guards
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests
- this iteration extends the direct-printf sibling seam one bounded step
  further: the counted ternary-loop `printf` direct-LIR route now lives in
  `src/backend/x86/codegen/direct_printf.cpp` instead of `emit.cpp`
- added a focused backend regression that calls the new direct `printf` helper
  seam explicitly so the Step 4 ownership move stays observable apart from the
  broader native-path dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_printf_helper_accepts_counted_printf_ternary_loop_slice`
  plus the existing local-buffer direct-printf helper guard and the native-path
  counted-ternary backend filter
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against a matched `HEAD` baseline after restoring the same external
  `c-testsuite` inventory in the clean worktree; the regression guard reported
  `181` failed before and after with no newly failing tests
- this iteration extends the direct-printf sibling seam one bounded step
  further: the local-buffer `strcpy` plus one-byte-offset `printf` direct-LIR
  route now lives in `src/backend/x86/codegen/direct_printf.cpp` instead of
  `emit.cpp`
- added a focused backend regression that calls the new direct `printf` helper
  seam explicitly so the Step 4 ownership move stays observable apart from the
  broader native-path dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_printf_helper_accepts_local_buffer_string_copy_printf_slice`
  plus the existing minimal/source-like local-buffer native-path regressions
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `2723` passed / `181` failed before and after with no newly failing tests
  and the existing `backend_bir_tests` `>30s` timeout warning unchanged
- 2026-04-10 Step 1 audit refresh:
  the source idea's older integration summary was stale
  - CMake already builds `emit.cpp`, the `direct_*.cpp` seam files, and the
    full translated `peephole/` subtree
  - CMake still excludes the translated top-level owner units:
    `alu.cpp`, `asm_emitter.cpp`, `atomics.cpp`, `calls.cpp`, `cast_ops.cpp`,
    `comparison.cpp`, `f128.cpp`, `float_ops.cpp`, `globals.cpp`,
    `i128_ops.cpp`, `inline_asm.cpp`, `intrinsics.cpp`, `memory.cpp`,
    `prologue.cpp`, `returns.cpp`, and `variadic.cpp`
  - `emit.cpp` still owns the direct-BIR and prepared-LIR dispatcher surface,
    while successful emission already flows through the translated
    `peephole_optimize(...)` entry
  - the immediate owner-switch blocker is therefore top-level build wiring and
    translated support-surface cleanup, not peephole activation itself
- first compile-oriented migration cluster chosen on 2026-04-10:
  start by wiring a leaf-like translated owner unit into CMake before touching
  the larger dispatcher cutover
  - preferred first candidate: `src/backend/x86/codegen/globals.cpp`
  - near-neighbor candidates after that: `comparison.cpp` and `returns.cpp`
  - expected blockers before broader cluster wiring:
    placeholder-heavy `inline_asm.cpp` / `asm_emitter.cpp` and likely
    signature/helper drift in some excluded top-level units
- this iteration extends Step 4 with the first dedicated direct-printf sibling
  seam: the bounded repeated `printf` immediate direct-LIR route now lives in
  `src/backend/x86/codegen/direct_printf.cpp` instead of `emit.cpp`
- added a focused backend regression that calls the new direct `printf` helper
  seam explicitly so the Step 4 ownership move stays observable apart from the
  broader x86 pipeline path
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_printf_helper_accepts_repeated_printf_immediates_slice`
  plus the existing source-like repeated-`printf` native-path regression and
  the direct variadic helper guard
- this iteration extends that direct-printf sibling seam one bounded step
  further on the BIR-owned path: the repeated local-i32 `printf` route now
  lives in `src/backend/x86/codegen/direct_printf.cpp` instead of `emit.cpp`
- added a focused backend regression that lowers the existing source-like
  repeated local-i32 `printf` fixture to BIR first and then calls the new
  direct helper seam explicitly so the Step 3 ownership move stays observable
  apart from the broader x86 pipeline path
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_printf_helper_accepts_repeated_printf_local_i32_bir_slice`
  plus `test_backend_bir_pipeline_drives_x86_lir_repeated_printf_local_i32_calls_through_bir_end_to_end`
  and `test_x86_direct_printf_helper_accepts_repeated_printf_immediates_slice`
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun against the
  stale `test_fail_before.log` baseline reported one apparent new failure:
  `cpp_eastl_memory_uses_allocator_parse_recipe`; a direct rerun confirmed that
  failure is already present in the matched `HEAD` baseline captured in
  `test_fail_matched_before.log`
- the regression guard therefore passed against the matched `HEAD` baseline:
  `3194` passed / `182` failed before versus `3194` passed / `182` failed
  after, with no newly failing tests and no new `>30s` cases
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `2723` passed / `181` failed before and after with no newly failing tests
- this iteration extends Step 4 with another bounded sibling seam: the
  prepared direct-LIR variadic sum2 and variadic double-byte runtime slices now
  live in `src/backend/x86/codegen/direct_variadic.cpp` instead of `emit.cpp`
- the translated `src/backend/x86/codegen/variadic.cpp` unit stays parked:
  once it entered the build it immediately proved it still depends on
  non-exported `X86Codegen` state/method members, so the new direct sibling
  file narrows ownership transfer without widening that class surface
- added a focused backend regression that calls the new direct variadic helper
  seam explicitly so the Step 4 ownership move stays observable apart from the
  broader dispatcher path
- 2026-04-10 Step 2 compile-cluster slice:
  a direct standalone compile probe on `src/backend/x86/codegen/globals.cpp`
  confirmed the first blocker is still exported-surface drift, not CMake wiring
- 2026-04-10 Step 2 blocker refinement:
  a follow-up attempt to let `globals.cpp` reuse the existing
  `emit_seg_{load,store}_symbol_impl(...)` path failed at link time even after
  the new unit compiled
  - `x86_codegen.hpp` currently exposes declarations that do not link cleanly
    against the existing `memory.cpp` symbol-helper definitions from this unit
  - the smallest real behavior lift for `globals.cpp` therefore still requires
    promoting a shared `X86Codegen` helper/state surface out of `emit.cpp`
    instead of trying to tunnel through the current public declarations
- added a second focused backend shared-util smoke test that keeps the
  translated globals-owner helper entrypoints (`emit_global_load_rip_rel_impl`,
  `emit_global_store_rip_rel_impl`, `emit_store_result_impl`,
  `emit_load_operand_impl`) compile/link reachable while the real owner wiring
  remains blocked on the shared-surface export
- focused checks passed:
  `./build/backend_shared_util_tests test_x86_codegen_header_exports_translated_globals_owner_helper_symbols`
  plus `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun matched the
  current 2904-test baseline in `test_after_rerun.log`; the regression guard
  reported `2723` passed / `181` failed before and after, with no newly failing
  tests and no new `>30s` cases
  - the translated unit could not compile because `X86Codegen` still hides
    `state`, `store_rax_to`, and `operand_to_rax` behind the legacy
    `emit.cpp` owner surface
  - added a narrow backend shared-util smoke check that takes the addresses of
    the translated globals-owner methods so link coverage fails if
    `globals.cpp` drops out of either backend test target again
  - wired `src/backend/x86/codegen/globals.cpp` into both backend source lists
    and kept the methods as explicit transitional throws so accidental runtime
    use fails loudly instead of silently faking behavior before the owner state
    is exported
- focused checks passed:
  `./build/backend_shared_util_tests`
  plus `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_direct_bir_minimal_scalar_global_load_end_to_end`
  and `test_backend_bir_pipeline_drives_x86_direct_bir_minimal_scalar_global_store_reload_end_to_end`
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests and no new `>30s` cases
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_variadic_helper_accepts_variadic_sum2_runtime_slice`
  plus the existing prepared variadic sum2/double-byte filters
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `2723` passed / `181` failed before and after with no newly failing tests
- the current question is not "what more should be translated"
- the current question is "which already-translated x86 codegen pieces can be
  made real and reachable first"
- Step 3 is now effectively complete apart from the intentionally parked
  `frame_compact.cpp` placeholder, so this iteration moves back to Step 4
  ownership transfer instead of widening the peephole matcher surface again
- this iteration extends the direct-global sibling seam one bounded step
  further: the direct-BIR global two-field struct store/sub/sub route now
  lives in `src/backend/x86/codegen/direct_globals.cpp` instead of `emit.cpp`
- added a new direct x86 BIR regression that pins the two-field global store,
  byte-offset reload, and final subtract chain so this Step 4 ownership move
  stays observable on the native path
- the focused `./build/backend_bir_tests global_two_field_struct_store_sub_sub`
  regression passed after the ownership move
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `2723` passed / `181` failed before and after with no newly failing tests
- this iteration extends the direct-global sibling seam one bounded step
  further: the two-function `store global in helper; return immediate in both
  helper and entry` route now lives in `src/backend/x86/codegen/direct_globals.cpp`
  instead of `emit.cpp`
- added a new direct x86 BIR regression that pins the helper store plus the
  independent entry return so this Step 4 ownership move stays observable on
  the native path
- the focused `./build/backend_bir_tests global_store_return_and_entry_return`
  regression passed after the ownership move
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `2723` passed / `181` failed before and after with no newly failing tests
- this iteration moves the bounded scalar-global direct-emission seam out of
  `emit.cpp` into `src/backend/x86/codegen/direct_globals.cpp`, covering the
  native scalar global load, extern scalar global load, and scalar global
  store-reload paths
- the original translated `globals.cpp` unit still does not compile against the
  current transitional `X86Codegen` header because it references state/method
  members that are not live yet, so the new sibling file intentionally narrows
  ownership transfer without widening class-surface work
- added a new direct regression for the zero-initialized scalar global-load
  path so the new sibling seam is pinned for both `.data` and `.bss` routes
- targeted backend filters for the direct BIR scalar-global load/store paths
  and the shared-BIR scalar-global store-reload routes passed on the new seam
- the broad `ctest --test-dir build -j8 --output-on-failure` sweep still fails
  in the same pre-existing aggregate areas, starting with the existing
  `backend_bir_tests` segfault and the wider known x86/backend failure set
- this iteration activates the parked translated callee-save cleanup slice in
  the real x86 peephole build and post-pass orchestration without enabling the
  still-placeholder frame-compaction pass
- `peephole/passes/callee_saves.cpp` now compiles as part of the real build,
  runs after the existing optimization rounds, and removes an unused callee-
  saved register save even when an earlier live pass already deleted the
  matching restore
- the new direct regression keeps the current frame-size boundary explicit: the
  callee-save save/restore pair is removed, but `subq $N, %rsp` remains parked
  until a future frame-compaction slice owns stack-slot rewriting
- the full-suite regression guard stayed monotonic for this slice:
  `2723` passed / `181` failed before and after, with no newly failing tests;
  the guard again reported one `>30s` test (`backend_bir_tests`), matching the
  existing harness instability rather than a new regression
- current evidence shows most of `src/backend/x86/codegen/*.cpp` is still not
  in the build, while `emit.cpp` remains the practical owner of x86 emission
- the active inventory for this plan is now explicit inside idea 43:
  16 translated top-level codegen units plus the translated peephole subtree
- the translated peephole subtree is the first bounded reachable slice because
  it has a self-contained string-in/string-out seam even though the current
  translated parser helpers still need a shared header and the emitted x86 asm
  uses Intel syntax rather than the ref tree's AT&T-shaped patterns
- the active x86 route now passes through the translated peephole entrypoint in
  both the direct x86 wrapper and the shared backend assembly handoff path
- the first live optimization is syntax-agnostic redundant jump-to-immediately-
  following-label elimination; the rest of the translated local-pass inventory
  remains parked until its assumptions are reconciled with the current emitter
- the translated compare-and-branch pass needed one shared classifier seam
  before it became reachable: `types.cpp` now classifies `cmp*` lines as
  `LineKind::Cmp`, which lets the pass fuse cmp/setcc/test/jcc sequences on the
  real x86 peephole path
- the loop-trampoline coalescing slice exposed another operand-classification
  seam: many comma-separated AT&T instructions still leave `LineInfo.dest_reg`
  unset because the trailing operand is not trimmed before family lookup, so
  the live trampoline pass currently uses a local trimmed-destination fallback
  instead of broadening that shared parser mid-slice
- this iteration promotes that trailing-operand trim into the shared
  classifier/helper path because the next translated trampoline case
  (`movslq` copy-back) would otherwise duplicate the same local fallback again
- the aggregate `backend_bir_tests` runner still crashes in pre-existing typed
  LIR validation coverage before it can serve as a narrow peephole harness, so
  this iteration verified the new trampoline form with a focused standalone
  peephole harness while the full-suite regression guard remained monotonic
- the next bounded gap against the translated loop-trampoline behavior is
  mixed safe/unsafe shuffle handling: the current C++ pass still bails out of
  the whole trampoline when one sibling move cannot be coalesced, while the
  reference keeps the unsafe sibling parked and still applies the safe rewrite
- this iteration intentionally pivots away from that matcher growth lane:
  `dead_code.cpp` is already translated but still disconnected from the real
  orchestration/build, so Step 3 can make progress by activating another
  existing pass file with focused peephole-only coverage
- the translated dead-code slice landed as a bounded orchestration/build step:
  `peephole/passes/dead_code.cpp` now compiles into the real x86 backend and
  runs inside the live peephole round without widening unrelated matcher logic
- the aggregate `backend_bir_tests` runner still times out on pre-existing
  typed-LIR validation failures, so this iteration validated the new pass with
  direct filtered backend test invocations plus the full-suite regression guard
- the full-suite regression guard stayed monotonic for this slice:
  `2723` passed / `181` failed before and after, with no newly failing tests;
  the guard reported one `>30s` test (`backend_bir_tests`), matching the
  existing harness-timeout situation rather than a new regression
- this iteration closes the simplest parked stack-load gap: the live pass now
  recognizes a single-slot `%rax` spill in the predecessor plus an immediate
  trampoline reload/copy-back pair and rewrites that shape onto the
  loop-carried register without widening into generic spill analysis
- the next bounded gap after this iteration is the remaining stack-load family:
  wider spill forms should stay parked until their predecessor-store matching
  and fallthrough safety rules are explicit instead of inferred ad hoc
- this iteration closes one more bounded stack-load shape without widening the
  shared parser or the predecessor scan into generic spill analysis: the live
  pass now accepts a single-slot `%eax` spill plus trampoline reload and
  `movslq` copy-back onto the loop-carried register
- the aggregate `backend_bir_tests` runner still aborts in the same
  pre-existing typed LIR validation coverage before it can serve as a narrow
  harness for the new regression, so this slice again used a focused
  standalone peephole harness while the full-suite regression guard stayed
  monotonic
- this iteration closes the next bounded stack-load family member without
  widening into generic spill analysis: the live pass now accepts a single-slot
  `%rax` spill whose trampoline reload writes the loop-carried register
  directly via `movq -N(%rbp), %loop_reg`
- the aggregate `backend_bir_tests` runner still aborts in the same
  pre-existing typed LIR validation coverage before it can serve as a narrow
  harness for the new direct-reload regression, so this slice again used the
  standalone backend test filter plus the full-suite regression guard
- this iteration closes the direct sign-extending stack-reload variant without
  widening predecessor analysis: the live pass now accepts a single-slot
  `%eax` spill whose trampoline reload writes the loop-carried register
  directly via `movslq -N(%rbp), %loop_reg`
- the aggregate `backend_bir_tests` runner still aborts in the same
  pre-existing typed LIR validation coverage before it can serve as a narrow
  harness for the new direct-`movslq` regression, so this slice again uses the
  standalone backend test filter/subset plus the full-suite regression guard
- this iteration deliberately adds a negative regression instead of widening
  stack-load matching again: the live x86 peephole must keep the spill,
  reload, and trampoline branch parked when the branch fallthrough still
  consumes `%rax` after the bounded predecessor spill
- the new stack-load safety regression passed on the existing implementation,
  confirming the current fallthrough guard already enforces that `%rax`/`%eax`
  boundary
- the first full-suite rerun picked up one extra unrelated failure in
  `cpp_typedef_owned_functional_cast_perf`; rerunning that test in isolation
  passed, and a second full-suite rerun returned to the monotonic baseline
- this iteration promotes copy propagation because the translated pass already
  exists, is more substantive than the parked stub passes, and composes
  directly with the live dead-move cleanup without crossing into epilogue or
  frame-layout ownership
- the translated copy-propagation slice landed as another bounded
  orchestration/build step: `peephole/passes/copy_propagation.cpp` now
  compiles into the real x86 backend and runs inside the live peephole round
  without widening into ABI-sensitive cleanup passes
- the new direct regression deliberately gives the existing dead-move cleanup
  explicit overwrite proof after the propagated use so this slice stays about
  activating translated copy propagation rather than broadening dead-code
  semantics
- the first full-suite rerun picked up one extra unrelated failure in
  `cpp_qualified_template_call_template_arg_perf`; rerunning that test in
  isolation passed, and a second full-suite rerun returned to the same
  `2723` passed / `181` failed count as the baseline
- the aggregate `backend_bir_tests` entry still does not serve as a narrow
  peephole harness: on the rerun it reached the same pre-existing
  `test_bir_lowering_accepts_local_two_field_struct_sub_sub_two_return_module`
  crash path quickly enough to surface as `SEGFAULT` instead of the earlier
  `Timeout`, so the unstable aggregate runner remains a known blocker rather
  than a new peephole regression
- this iteration activates the translated store-forwarding slice in the real
  x86 peephole build and optimization round with a focused regression proving a
  same-register `(%rbp)` reload is removed after a matching bounded stack
  store
- the store-forwarding regression initially exposed a shared classifier seam:
  `types.cpp` left stack-store offsets contaminated by the source operand and
  left stack-load destination registers with leading whitespace, so matching
  `StoreRbp`/`LoadRbp` pairs never surfaced to the live pass until that parse
  cleanup landed
- this slice intentionally keeps dead-store semantics parked: the new
  regression only claims bounded reload elimination, while the surviving stack
  store remains acceptable because the current dead-store pass still treats
  function exit as a barrier
- the full-suite regression guard stayed monotonic for this slice:
  `2723` passed / `181` failed before and after, with no newly failing tests
  and no new `>30s` tests
- this iteration activates the parked translated tail-call slice in the real
  x86 peephole build and optimization round without widening into callee-save
  or frame-compaction work
- `peephole/passes/tail_call.cpp` now compiles as part of the real build,
  runs inside the live optimization loop, and rewrites a bounded
  `call; pure epilogue; ret` sequence into the same epilogue followed by
  `jmp target`
- the first full-suite rerun picked up the same intermittent
  `cpp_qualified_template_call_template_arg_perf` outlier already noted in
  earlier slices; rerunning that test in isolation passed, and a second
  full-suite rerun returned to the baseline `2723` passed / `181` failed
  counts with no newly failing tests
- this slice intentionally keeps the shared classifier boundary parked:
  focused coverage uses the currently classified `call ...` spelling rather
  than widening `types.cpp` to classify `callq ...` in the same iteration
- this iteration activates the translated memory-fold slice in the real x86
  peephole build and optimization round with focused regressions around a
  bounded stack-load plus ALU-use pattern
- `peephole/passes/memory_fold.cpp` now compiles as part of the real build,
  runs inside the live optimization loop, and folds a safe `movq`/`movl`
  reload from `(%rbp)` into the following ALU source operand when the loaded
  register is a scratch register and is not also the ALU destination
- the direct regression keeps one safety boundary explicit: the load remains
  parked when folding would turn the loaded register into the ALU destination,
  which would change the instruction into an invalid memory-destination shape
- the aggregate `backend_bir_tests` entry remains an unstable broad harness:
  this iteration's focused filters passed, the full-suite fail set stayed at
  `2723` passed / `181` failed, and the monotonic guard passed only in
  non-decreasing mode because the existing aggregate backend runner surfaced as
  `Timeout` instead of the baseline run's `SEGFAULT`

## Recently Completed

- moved the bounded direct-BIR global two-field struct store/sub/sub route out
  of `emit.cpp` into `src/backend/x86/codegen/direct_globals.cpp`
- added a direct x86 BIR regression that pins the native two-field global
  store/sub/sub route without relying on the LIR-to-BIR lowering hop
- extended `src/backend/x86/codegen/direct_globals.cpp` to own the bounded
  two-function global store plus independent entry-return direct-BIR route
- moved `MinimalGlobalStoreReturnAndEntryReturnSlice` ownership out of
  `emit.cpp` into `x86_codegen.hpp` so the sibling direct-global unit can own
  that emitter seam cleanly
- added a direct x86 BIR regression for the helper-global-store plus entry-
  return route and wired it into `backend_bir_tests`
- started Step 4 ownership transfer after the translated peephole subtree
  reached the real x86 path
- moved the bounded scalar-global direct-emission cluster out of `emit.cpp`
  into `src/backend/x86/codegen/direct_globals.cpp`
- wired `direct_globals.cpp` into both the main x86 build and the backend test
  build
- added a direct regression that pins the zero-initialized scalar global-load
  path on the native x86 route
- created idea 43 to own x86 peephole pipeline completion
- parked idea 44 as a separate shared-BIR cleanup lane
- switched the active runbook and execution state to idea 43
- reprioritized idea 43 so integrating already-translated x86 codegen units is
  now ahead of peephole-only work
- added the first bounded translated x86 peephole compile-in cluster to the
  build: `peephole/mod.cpp`, `peephole/types.cpp`,
  `peephole/passes/helpers.cpp`, `peephole/passes/local_patterns.cpp`, and
  `peephole/passes/mod.cpp`
- added a shared peephole header so that cluster now compiles as a real unit
  instead of parked source inventory
- routed x86 emitted asm through `peephole_optimize(...)` on both the direct
  x86 wrapper path and the shared backend assembly handoff path
- added targeted backend tests that pin the live redundant-jump cleanup and
  confirm emitted countdown-loop asm reaches the peephole stage
- compiled the translated `peephole/passes/push_pop.cpp` unit into the real
  x86 backend and test builds
- wired the safe redundant `pushq`/`popq` elimination pass into the live x86
  peephole optimization loop
- added a direct regression test that proves the translated push/pop pass now
  removes a redundant pair while preserving the surrounding label and return
- compiled the translated `peephole/passes/compare_branch.cpp` unit into the
  real x86 backend and test builds
- wired the translated compare-and-branch fusion pass into the live x86
  peephole optimization loop
- added a direct regression test that proves the live x86 peephole now fuses a
  cmp/setcc/test/jne boolean-materialization sequence into a direct conditional
  jump
- compiled the translated `peephole/passes/loop_trampoline.cpp` unit into the
  real x86 backend and test builds
- wired a conservative single-entry trampoline rewrite into the live x86
  peephole optimization loop before the generic adjacent-jump cleanup
- added a direct regression test that proves the live x86 peephole now
  rewrites a sole incoming branch away from a label-only trampoline block onto
  the real loop header while preserving the current emitted label syntax
- widened the live translated loop-trampoline pass to coalesce a bounded
  register-register `movq` trampoline copy back into the predecessor update
  chain before redirecting the branch onto the real loop header
- added a direct regression test that proves the live x86 peephole now rewrites
  `movq %loop_reg, %tmp` / mutate `%tmp` / trampoline `movq %tmp, %loop_reg`
  into a direct mutation of the loop-carried register
- promoted trailing-operand trimming into the shared x86 peephole
  classifier/helper path so comma-delimited destinations no longer leave
  `LineInfo.dest_reg` unset just because the trailing operand still carries
  whitespace
- widened the live translated loop-trampoline pass to recognize and coalesce a
  bounded `movslq %tmpd, %loop_reg` trampoline copy-back onto the real
  loop-carried register
- added a direct regression test that proves the live x86 peephole now rewrites
  a 32-bit update feeding a `movslq` trampoline copy-back into a direct update
  of the loop-carried register
- widened the live translated loop-trampoline pass to coalesce a proven-safe
  register shuffle inside a mixed trampoline block even when a sibling move
  still has to stay parked
- taught the loop-trampoline predecessor scan to reject rewrites that would
  read the loop-carried destination while also writing the temporary, avoiding
  bogus self-referential rewrites such as `addq %r10, %r10`
- added a direct regression test that proves the live x86 peephole now removes
  only the safe `%r9/%r14` shuffle from a mixed trampoline while leaving the
  unsafe `%r10/%r15` pair and trampoline branch in place
- widened the live translated loop-trampoline pass to coalesce a conservative
  single-slot stack-spill trampoline when the predecessor computes into `%rax`,
  spills to one `(%rbp)` slot, and the trampoline immediately reloads `%rax`
  before copying back into the loop-carried register
- added a direct regression test that proves the live x86 peephole now rewrites
  that bounded stack-spill trampoline onto the loop-carried register and drops
  the now-dead spill, reload, and trampoline copy-back sequence
- widened the live translated loop-trampoline pass to coalesce a bounded
  32-bit single-slot stack spill when the predecessor computes through `%eax`,
  stores to one `(%rbp)` slot, and the trampoline reloads `%eax` before
  `movslq` copy-back into the loop-carried register
- added a direct regression test that proves the live x86 peephole now rewrites
  that bounded `movl`/`movslq` stack-spill trampoline onto the loop-carried
  register and drops the now-dead spill, reload, and trampoline copy-back
  sequence
- widened the live translated loop-trampoline pass to coalesce a bounded
  64-bit single-slot stack spill when the trampoline reloads the loop-carried
  register directly from `(%rbp)` instead of bouncing through `%rax`
- added a direct regression test that proves the live x86 peephole now rewrites
  that bounded direct-reload stack-spill trampoline onto the loop-carried
  register and drops the now-dead spill and direct trampoline reload
- widened the live translated loop-trampoline pass to coalesce a bounded
  32-bit single-slot stack spill when the trampoline sign-extends directly from
  `(%rbp)` into the loop-carried register instead of reloading `%eax` first
- added a direct regression test that proves the live x86 peephole now rewrites
  that bounded direct `movslq` stack-spill trampoline onto the loop-carried
  register and drops the now-dead spill and direct trampoline reload
- added a direct regression test that proves the live x86 peephole keeps a
  bounded stack-spill trampoline parked when the branch fallthrough still
  consumes `%rax`, locking in the current fallthrough-safety boundary instead
  of widening the matcher
- rebuilt and reran the full ctest suite with monotonic results:
  `183` failures before, `181` failures after, no newly failing tests
- rebuilt and reran the full ctest suite for this test-only safety slice with
  monotonic results: `181` failures before, `181` failures after, no newly
  failing tests
- compiled the translated `peephole/passes/dead_code.cpp` unit into the real
  x86 backend and test builds
- wired the translated dead register-move and overwritten stack-store cleanup
  passes into the live x86 peephole optimization loop
- added direct regression tests that prove the live x86 peephole now drops a
  dead `movq` whose destination is overwritten before any read and removes an
  earlier `(%rbp)` store when a later store overwrites the same slot before any
  read
- rebuilt, ran the filtered `test_x86_peephole_` backend subset plus the new
  direct dead-code test filters, and reran the full ctest suite with monotonic
  results: `181` failures before, `181` failures after, no newly failing tests
- compiled the translated `peephole/passes/copy_propagation.cpp` unit into the
  real x86 backend and test builds
- compiled the translated `peephole/passes/store_forwarding.cpp` unit into the
  real x86 backend and test builds
- wired the translated store-forwarding pass into the live x86 peephole
  optimization loop
- fixed the shared x86 peephole stack-memory classifier so `StoreRbp` and
  `LoadRbp` lines trim register/offset operands correctly before family and
  slot matching
- added a direct regression test that proves the live x86 peephole now drops a
  bounded same-register `(%rbp)` reload after a matching stack store while
  preserving the surrounding control flow
- rebuilt, ran the focused `test_x86_peephole_` backend filter plus the direct
  countdown-loop peephole route test, and reran the full ctest suite with
  monotonic results: `181` failures before, `181` failures after, no newly
  failing tests
- wired the translated register copy-propagation pass into the live x86
  peephole optimization loop ahead of the existing dead-code cleanup
- added a direct regression test that proves the live x86 peephole now
  propagates a transitive register-copy chain into a downstream ALU use and
  lets the existing overwrite-based dead-move cleanup drop the superseded
  copies
- rebuilt, ran the filtered `test_x86_peephole_` backend subset plus the new
  direct copy-propagation test filter, and reran the full ctest suite to the
  same `181`-failure count as the baseline; the stable rerun surfaced the
  existing aggregate `backend_bir_tests` crash path as `SEGFAULT` instead of
  the earlier timeout while keeping the failure set otherwise unchanged
- compiled the translated `peephole/passes/tail_call.cpp` unit into the real
  x86 backend and test builds
- wired the translated tail-call pass into the live x86 peephole optimization
  loop
- added a direct regression test that proves the live x86 peephole now
  rewrites a bounded `call` plus pure epilogue plus `ret` sequence into the
  same epilogue followed by `jmp target`
- rebuilt, ran the focused tail-call regression plus the surrounding
  `test_x86_peephole_` backend subset, reran the intermittent
  `cpp_qualified_template_call_template_arg_perf` case in isolation after a
  transient full-suite failure, and confirmed a second full ctest rerun
  returned to the baseline `181`-failure count with no newly failing tests
- compiled the translated `peephole/passes/frame_compact.cpp` unit into the
  real x86 backend and test builds
- wired the translated frame-compaction pass into the live x86 peephole
  pipeline only for the bounded zero-byte frame-adjustment case
- added a direct regression test that proves the live x86 peephole now drops
  `subq $0, %rsp` while preserving the surrounding prologue/body shape
- focused checks passed:
  `./build/backend_bir_tests test_x86_peephole_eliminates_zero_byte_frame_adjustment`
  plus the surrounding `test_x86_peephole_` subset and the existing direct
  countdown-loop peephole route regression
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `2723` passed / `181` failed after with no newly failing tests, and the
  existing `backend_bir_tests` `>30s` warning remained the only timeout note
- 2026-04-10 verification refresh:
  the current tree already matches the first `globals.cpp` compile-cluster goal
  that had still been listed as active work above
  - `src/backend/x86/codegen/globals.cpp` is already in the real build and
    remains parked behind transitional unwired-owner stubs
  - `tests/backend/backend_shared_util_tests.cpp` already carries the narrow
    translated-globals symbol/link smoke coverage
  - focused validation passed:
    `./build/backend_shared_util_tests x86_codegen_header_exports_translated_globals_owner`
    and `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
  - broad validation remained monotonic against `test_fail_before.log`; the
    matched rerun in `test_fail_after.log` kept the same `181` failing tests
    with no newly failing cases
- this iteration extends the direct-calls sibling seam with the remaining
  bounded two-arg prepared-LIR local/local variants: the both-local, both-local
  first-rewrite, both-local second-rewrite, and both-local double-rewrite
  helper-call routes now live in `src/backend/x86/codegen/direct_calls.cpp`
  instead of `emit.cpp`
- added focused backend regressions that call each new direct call-helper seam
  explicitly so the Step 3 ownership move stays observable apart from the
  broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests
  test_x86_direct_call_helper_accepts_two_arg_both_local_arg_call_slice`,
  `test_x86_direct_call_helper_accepts_two_arg_both_local_first_rewrite_call_slice`,
  `test_x86_direct_call_helper_accepts_two_arg_both_local_second_rewrite_call_slice`,
  `test_x86_direct_call_helper_accepts_two_arg_both_local_double_rewrite_call_slice`,
  plus the matching direct-emitter regressions
- the stored `test_fail_before.log` baseline is stale relative to the current
  3376-test inventory, so the direct regression guard falsely reported one new
  unrelated failure (`cpp_eastl_memory_uses_allocator_parse_recipe`) while the
  workspace was also missing `ref/EASTL`
- a matched rerun against the current tree stayed stable: comparing
  `test_fail_matched_before.log` to `test_fail_after_rerun.log` with
  `--allow-non-decreasing-passed` reported `3194` passed / `182` failed before
  and after, with no newly failing tests and no new `>30s` cases
