# x86_64 Translated Codegen Integration

Status: Active
Source Idea: ideas/open/43_x86_64_peephole_pipeline_completion.md
Source Plan: plan.md

## Current Active Item

- Step 4 prepared-LIR sibling direct-call follow-on: keep the active x86
  ownership move on bounded direct-call route slices already living in
  `src/backend/x86/codegen/direct_calls.cpp` and the x86 backend tests instead
  of widening back into the parked branch/select or prologue/returns surfaces
- immediate target:
  add native public-pipeline regression coverage for the still-plain
  second-local two-arg sibling family so the typed-call args-spacing and
  suffix-spacing trimming seam is pinned above the prepared-LIR helper
  entrypoints too

## Next Slice

- continue auditing whether any remaining direct-call sibling family still
  lacks native public-pipeline spacing/suffix coverage after this
  second-local-arg route completion, without widening into parked matcher
  ownership
- likely next bounded slice:
  native public-pipeline spacing/suffix coverage for the still-plain
  second-local-rewrite sibling family, which already has prepared-LIR
  emitter coverage but not the public x86 route tests
- do not widen into new helper ownership or unrelated control-flow matchers in
  the same commit
- leave fused compare+branch, block branch lowering, select, and
  returns/prologue ownership parked for later explicit slices
- keep constant-backed long-double caller work and unrelated TLS/PIC work out
  of scope unless a later translated-owner cutover requires them directly

## Current Iteration Notes

- this iteration extends the active Step 4 direct-call ownership path with the
  missing native public-pipeline args-spacing and suffix-spacing regressions
  for the plain second-local two-arg helper family: the x86 route now pins
  typed-call trimming on that already-landed sibling direct-call family
  without widening matcher scope
- implementation note:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now adds dedicated
  native-path end-to-end regressions for
  `make_lir_minimal_two_arg_second_local_arg_direct_call_module_{with_spacing,with_suffix_spacing}()`
  so the public x86 backend route keeps the second-local two-arg helper
  family live when either args spacing or `callee_type_suffix` spacing drifts
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8` and
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_second_local_arg_direct_call_with_spacing_on_native_x86_path test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_second_local_arg_direct_call_with_suffix_spacing_on_native_x86_path`
- broad validation passed:
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration extends the active Step 4 direct-call ownership path with the
  missing native public-pipeline suffix-spacing regressions for the plain
  first-local and both-local two-arg helper families: the x86 route now pins
  suffix-spaced typed-call trimming on those already-landed sibling direct-call
  families without widening matcher scope
- implementation note:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now adds dedicated
  native-path end-to-end regressions for
  `make_lir_minimal_two_arg_{local_arg,both_local_arg}_direct_call_module_with_suffix_spacing()`
  so the public x86 backend route keeps the first-local and both-local plain
  two-arg helper families live even when `callee_type_suffix` spacing drifts
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8` and
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_local_arg_direct_call_with_suffix_spacing_on_native_x86_path test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_both_local_arg_direct_call_with_suffix_spacing_on_native_x86_path`
- broad validation note:
  the first matched current-tree rerun remained non-monotonic because unrelated
  full-suite cases drifted between `test_fail_after.log` and
  `test_fail_after_rerun.log`, temporarily swapping in
  `c_testsuite_src_00205_c` and `llvm_gcc_c_torture_src_20040313_1_c` while
  also resolving other unrelated failures
- broad validation passed on a stabilized current-tree rerun:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after_rerun.log`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after_rerun2.log`,
  and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_after_rerun.log --after test_fail_after_rerun2.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed after, zero newly failing tests, and zero
  new `>30s` tests

- this iteration extends the active Step 4 direct-call ownership path with the
  missing folded-two-arg and dual-identity typed-call spacing regressions:
  helper and native x86 prepared-LIR coverage now pin args-spacing and
  suffix-spacing drift on those already-landed sibling direct-call families
  without widening matcher scope
- implementation note:
  `tests/backend/backend_bir_pipeline_tests.cpp` now adds dedicated
  folded-two-arg and dual-identity helper fixtures plus matching helper
  assertions so the direct-call helper seam explicitly keeps those sibling
  families live for plain, args-spaced, and suffix-spaced typed calls
- implementation note:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now adds matching
  prepared-LIR folded-two-arg and dual-identity spacing fixtures plus native
  emitter assertions so the x86 path keeps helper/native parity on those
  sibling direct-call families
- implementation note:
  the existing direct-call parser/emitter path already accepted those spaced
  forms, so this slice lands as focused regression coverage rather than a code
  path rewrite
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_folded_two_arg_slice test_x86_direct_call_helper_accepts_folded_two_arg_slice_with_spacing test_x86_direct_call_helper_accepts_folded_two_arg_slice_with_suffix_spacing test_x86_direct_call_helper_accepts_dual_identity_sub_slice test_x86_direct_call_helper_accepts_dual_identity_sub_slice_with_spacing test_x86_direct_call_helper_accepts_dual_identity_sub_slice_with_suffix_spacing`, and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_folded_two_arg_direct_call_slice test_x86_direct_emitter_lowers_minimal_folded_two_arg_direct_call_slice_with_spacing test_x86_direct_emitter_lowers_minimal_folded_two_arg_direct_call_slice_with_suffix_spacing test_x86_direct_emitter_lowers_minimal_dual_identity_direct_call_sub_slice test_x86_direct_emitter_lowers_minimal_dual_identity_direct_call_sub_slice_with_spacing test_x86_direct_emitter_lowers_minimal_dual_identity_direct_call_sub_slice_with_suffix_spacing`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_before.log`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration extends the active Step 4 direct-call ownership path with the
  missing plain helper-side call-crossing regression: helper and native x86
  prepared-LIR coverage now pin the bounded call-crossing helper family on the
  native helper seam for plain, args-spaced, and suffix-spaced typed calls
  without widening matcher scope
- implementation note:
  `tests/backend/backend_bir_pipeline_tests.cpp` now adds
  `make_supported_x86_call_crossing_direct_call_lir_module()` plus
  `test_x86_direct_call_helper_accepts_call_crossing_slice()` and suite
  registration so the direct-call helper seam explicitly keeps the unspaced
  call-crossing form live next to the already-landed spacing and
  suffix-spacing variants
- implementation note:
  the existing direct-call parser/helper path already accepted that plain form,
  so this slice lands as focused regression coverage rather than a code path
  rewrite
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_call_crossing_slice test_x86_direct_call_helper_accepts_call_crossing_slice_with_spacing test_x86_direct_call_helper_accepts_call_crossing_slice_with_suffix_spacing test_x86_direct_emitter_lowers_minimal_call_crossing_direct_call_slice test_x86_direct_emitter_lowers_minimal_call_crossing_direct_call_slice_with_spacing test_x86_direct_emitter_lowers_minimal_call_crossing_direct_call_slice_with_suffix_spacing`
- broad validation note:
  comparing the fresh `test_fail_after.log` run against the older
  `test_fail_matched_before.log` baseline was non-monotonic because the current
  tree now reproducibly fails the unrelated x86 route cases
  `backend_codegen_route_x86_64_c_testsuite_00030_repeated_call_compare_zero_return_retries_after_direct_bir_rejection`,
  `backend_codegen_route_x86_64_c_testsuite_00031_local_i32_inc_dec_compare_retries_after_direct_bir_rejection`,
  `c_testsuite_x86_backend_src_00030_c`, and
  `c_testsuite_x86_backend_src_00031_c`
- broad validation passed on a matched current-tree rerun:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after_rerun.log`,
  and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_after.log --after test_fail_after_rerun.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration extends the active Step 4 direct-call ownership path with the
  missing helper-side base single-arg call regressions: direct x86 helper
  coverage now pins the unspaced add-immediate and identity single-arg helper
  families so the helper/native prepared-LIR matrix is no longer only covered
  on the native-emitter side
- implementation note:
  `tests/backend/backend_bir_pipeline_tests.cpp` now adds
  `test_x86_direct_call_helper_accepts_single_arg_{add_imm,identity}_call_slice()`
  plus suite registration so the direct-call helper seam explicitly keeps the
  base single-arg add-immediate and identity forms live alongside the already-
  landed spacing and suffix-spacing variants
- implementation note:
  the existing direct-call parser/helper path already accepted those base
  forms, so this slice lands as focused regression coverage rather than a code
  path rewrite
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8` and
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_single_arg_add_imm_call_slice test_x86_direct_call_helper_accepts_single_arg_identity_call_slice`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration extends the active Step 4 direct-call ownership path with the
  missing single-arg helper typed-call spacing regressions: helper and native
  x86 prepared-LIR coverage now pin args-spacing-only and suffix-spacing drift
  on the already-landed single-arg add-immediate and identity helper families
  without widening matcher scope
- implementation note:
  `tests/backend/backend_bir_pipeline_tests.cpp` now adds dedicated
  `make_supported_x86_single_arg_{add_imm,identity}_call_lir_module_{with_spacing,with_suffix_spacing}()`
  fixtures plus matching helper assertions so the direct-call parser keeps
  separately pinned spacing and suffix-spacing seams for the single-arg helper
  family
- implementation note:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now adds matching
  `make_lir_minimal_direct_call_{add_imm,identity_arg}_module_{with_spacing,with_suffix_spacing}()`
  fixtures plus native-emitter assertions so the prepared-LIR x86 path keeps
  helper/native parity on those single-arg typed-call spacing forms
- implementation note:
  the existing direct-call parser/emitter path already accepted those spaced
  forms, so this slice lands as focused regression coverage rather than a code
  path rewrite
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_single_arg_add_imm_call_slice_with_spacing test_x86_direct_call_helper_accepts_single_arg_add_imm_call_slice_with_suffix_spacing test_x86_direct_call_helper_accepts_single_arg_identity_call_slice_with_spacing test_x86_direct_call_helper_accepts_single_arg_identity_call_slice_with_suffix_spacing`, and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_single_arg_add_imm_helper_call_slice_with_spacing test_x86_direct_emitter_lowers_minimal_single_arg_add_imm_helper_call_slice_with_suffix_spacing test_x86_direct_emitter_lowers_minimal_single_arg_identity_helper_call_slice_with_spacing test_x86_direct_emitter_lowers_minimal_single_arg_identity_helper_call_slice_with_suffix_spacing`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration extends the active Step 4 direct-call ownership path with the
  missing single-local typed-call args-spacing regression on the native x86
  prepared-LIR seam: native emitter coverage now pins args-spacing-only drift
  on that already-landed helper family so it matches the existing helper-side
  spacing coverage without widening matcher scope
- implementation note:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now adds
  `make_x86_local_arg_call_lir_module_with_spacing()` plus
  `test_x86_direct_emitter_lowers_minimal_local_arg_call_slice_with_spacing()`
  so the native prepared-LIR direct emitter keeps a separately pinned
  args-spacing seam next to the existing single-local suffix-spacing coverage
- implementation note:
  the existing direct-call parser/emitter path already accepted that
  args-spaced form, so this slice lands as focused regression coverage rather
  than a code path rewrite
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_local_arg_call_slice_with_spacing`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_local_arg_call_slice_with_spacing`, and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_local_arg_call_slice_with_suffix_spacing`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration extends the active Step 4 direct-call ownership path with the
  missing plain immediate/immediate two-arg typed-call suffix-spacing
  regressions: helper and native x86 prepared-LIR coverage now pin
  suffix-spaced `callee_type_suffix` on that already-landed helper family
  without widening matcher scope
- implementation note:
  `tests/backend/backend_bir_pipeline_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now add explicit
  `with_suffix_spacing` fixtures and assertions for the immediate/immediate
  two-arg helper family so the existing typed-call parser keeps a separately
  pinned suffix-spacing seam adjacent to the already-covered args-spacing route
- implementation note:
  the existing direct-call parser/emitter path already accepted that
  suffix-spaced form, so this slice lands as focused regression coverage
  rather than a code path rewrite
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_call_slice_with_spacing`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_call_slice_with_suffix_spacing`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_helper_call_slice_with_spacing`, and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_helper_call_slice_with_suffix_spacing`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_before.log`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration extends the active Step 4 direct-call ownership path with the
  missing plain immediate/immediate two-arg typed-call spacing regressions:
  helper and native x86 prepared-LIR coverage now pin args-spacing-only drift
  on the already-landed immediate/immediate helper-call family without
  widening matcher scope
- implementation note:
  `tests/backend/backend_bir_pipeline_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now add explicit
  `with_spacing` fixtures and assertions for the immediate/immediate two-arg
  helper family so the existing typed-call parser keeps a separately pinned
  args-spacing seam adjacent to the plain route
- implementation note:
  the existing direct-call parser/emitter path already accepted that spaced
  form, so this slice lands as focused regression coverage rather than a code
  path rewrite
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_call_slice_with_spacing`, and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_helper_call_slice_with_spacing`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_before.log`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration extends the active Step 4 direct-call ownership path with the
  missing plain first-local two-arg typed-call spacing regression on the
  helper seam: helper coverage now pins args-spacing-only drift on the
  already-landed first-local two-arg direct-call family adjacent to the
  already-covered native x86 prepared-LIR spacing route, without widening
  matcher scope
- implementation note:
  `tests/backend/backend_bir_pipeline_tests.cpp` now adds explicit
  `make_supported_x86_two_arg_local_arg_call_lir_module_with_spacing()` and
  `test_x86_direct_call_helper_accepts_two_arg_local_arg_call_slice_with_spacing()`
  coverage so the helper-side direct-call parser keeps a separately pinned
  args-spacing seam next to the existing suffix-spacing variant
- implementation note:
  the existing direct-call parser/helper path already accepted that spaced
  form, so this slice lands as focused regression coverage rather than a code
  path rewrite
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_local_arg_call_slice_with_spacing`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_local_arg_call_slice_with_spacing`, and
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_local_arg_call_slice_with_suffix_spacing`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration extends the active Step 4 direct-call ownership path with the
  missing call-crossing typed-call suffix-spacing regressions: helper and
  native x86 prepared-LIR coverage now pin suffix-spaced
  `callee_type_suffix` on the already-landed call-crossing helper family
  without widening matcher scope
- implementation note:
  `tests/backend/backend_bir_pipeline_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now add explicit
  `with_suffix_spacing` fixtures and assertions for the call-crossing family so
  the existing typed-call parser keeps a separately pinned suffix-spacing seam
  adjacent to the already-covered plain spacing route
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_call_crossing_slice_with_spacing`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_call_crossing_slice_with_suffix_spacing`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_call_crossing_direct_call_slice_with_spacing`, and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_call_crossing_direct_call_slice_with_suffix_spacing`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration extends the active Step 4 direct-call ownership path with the
  missing second-local typed-call suffix-spacing regressions: helper and native
  x86 prepared-LIR coverage now pin suffix-spaced `callee_type_suffix` on the
  second-local arg and second-local rewrite helper families without widening
  matcher scope
- implementation note:
  `tests/backend/backend_bir_pipeline_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now add explicit
  `with_suffix_spacing` fixtures and assertions for those second-local
  families so the existing typed-call parser keeps a separately pinned
  suffix-spacing seam adjacent to the already-covered plain spacing routes
- implementation note:
  the existing direct-call parser/emitter path already accepted those
  suffix-spaced forms, so this slice lands as focused regression coverage
  rather than a code path rewrite
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_second_local_arg_call_slice_with_suffix_spacing`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_second_local_rewrite_call_slice_with_suffix_spacing`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_second_local_arg_call_slice_with_suffix_spacing`, and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_second_local_rewrite_call_slice_with_suffix_spacing`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and one existing `>30s` note on `backend_bir_tests`

- this iteration extends the active Step 4 direct-call ownership path with the
  missing plain second-local typed-call spacing regressions: helper and native
  x86 prepared-LIR coverage now pin args-spacing-only drift on the second-local
  arg and second-local rewrite helper families without widening matcher scope
- implementation note:
  `tests/backend/backend_bir_pipeline_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now add explicit
  `with_spacing` fixtures and assertions for those second-local families so the
  existing direct-call parser/emitter path keeps a separately pinned args-only
  spacing seam adjacent to the already-covered first-local and both-local
  routes
- implementation note:
  the existing direct-call parser/emitter path already accepted those spaced
  forms, so this slice lands as focused regression coverage rather than a code
  path rewrite
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_second_local_arg_call_slice_with_spacing`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_second_local_rewrite_call_slice_with_spacing`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_second_local_arg_call_slice_with_spacing`, and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_second_local_rewrite_call_slice_with_spacing`
- broad validation passed:
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and one existing `>30s` note on `backend_bir_tests`

- this iteration extends the active Step 4 direct-call ownership path with the
  missing plain both-local rewrite typed-call spacing regressions: helper and
  native x86 prepared-LIR coverage now pin args-spacing-only drift on the
  first-rewrite, second-rewrite, and double-rewrite helper families without
  widening matcher scope
- implementation note:
  `tests/backend/backend_bir_pipeline_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now split those
  both-local rewrite fixtures into explicit `with_spacing` and
  `with_suffix_spacing` variants so the existing suffix-spacing coverage stacks
  on top of a separately pinned args-spacing seam instead of silently covering
  both forms at once
- implementation note:
  the existing direct-call parser/emitter path already accepted those
  args-spaced forms, so this slice lands as focused regression coverage rather
  than a code path rewrite
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_both_local_first_rewrite_call_slice_with_spacing`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_both_local_second_rewrite_call_slice_with_spacing`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_both_local_double_rewrite_call_slice_with_spacing`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_first_rewrite_call_slice_with_spacing`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_second_rewrite_call_slice_with_spacing`, and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_double_rewrite_call_slice_with_spacing`
- broad validation passed:
  `ctest --test-dir build -j8 --output-on-failure > test_before.log`,
  `ctest --test-dir build -j8 --output-on-failure > test_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration extends the active Step 4 direct-call ownership path with the
  missing middle both-local typed-call parsing regressions: helper and native
  x86 prepared-LIR coverage now pin suffix-spaced `callee_type_suffix` and
  whitespace-tolerant `args_str` on the both-local first-rewrite and
  second-rewrite helper families without widening matcher scope
- implementation note:
  `tests/backend/backend_bir_pipeline_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now cover those
  suffix-spacing forms directly, proving the shared trimmed two-argument
  typed-call decoder continues to feed the existing both-local rewrite helper
  seams on both the helper-level and native x86 prepared-LIR paths
- implementation note:
  the existing direct-call parser/emitter path already accepted those spaced
  forms, so this slice lands as focused regression coverage rather than a code
  path rewrite
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_both_local_first_rewrite_call_slice_with_suffix_spacing`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_both_local_second_rewrite_call_slice_with_suffix_spacing`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_first_rewrite_call_slice_with_suffix_spacing`, and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_second_rewrite_call_slice_with_suffix_spacing`
- broad validation note:
  this slice intentionally stayed on focused `backend_bir_tests` coverage and
  did not widen back into the known unstable aggregate `ctest -R backend_bir_tests`
  lane

- this iteration extends the active Step 4 direct-call ownership path with a
  bounded both-local typed-call parsing regression slice: helper and native x86
  prepared-LIR coverage now pin whitespace-tolerant `callee_type_suffix` and
  `args_str` on the both-local reload route and the both-local double-rewrite
  route without widening the matcher surface
- implementation note:
  `tests/backend/backend_bir_pipeline_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now cover those
  spacing-drifted both-local forms directly, proving the shared trimmed
  two-argument typed-call decoder still feeds the existing direct-call helper
  seams for both the minimal reload family and the richer double-rewrite family
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_both_local_arg_call_slice_with_spacing`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_both_local_double_rewrite_call_slice_with_suffix_spacing`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_arg_call_slice_with_spacing`, and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_double_rewrite_call_slice_with_suffix_spacing`
- broad validation note:
  this slice intentionally stayed on focused `backend_bir_tests` coverage and
  did not widen back into the known unstable aggregate `ctest -R backend_bir_tests`
  lane

- this iteration extends the active Step 4 direct-call ownership path with
  another bounded typed-call parsing regression slice: helper and native x86
  prepared-LIR coverage now pin suffix-spaced `callee_type_suffix` and
  whitespace-tolerant `args_str` on the single-local, first-local-rewrite, and
  call-crossing helper-call families without widening the matcher surface
- implementation note:
  `tests/backend/backend_bir_pipeline_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now cover those
  suffix-spacing and call-crossing spacing forms directly, proving both the
  helper seam and the real `try_emit_prepared_lir_module(...)` x86 path keep
  accepting the shared trimmed typed-call syntax they already rely on
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_local_arg_call_slice_with_suffix_spacing`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_first_local_rewrite_call_slice_with_suffix_spacing`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_call_crossing_slice_with_spacing`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_local_arg_call_slice_with_suffix_spacing`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_first_local_rewrite_call_slice_with_suffix_spacing`, and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_call_crossing_direct_call_slice_with_spacing`
- broad validation note:
  this slice intentionally stayed on focused `backend_bir_tests` coverage and
  did not widen back into the known unstable aggregate `ctest -R backend_bir_tests`
  lane

- this iteration locks whitespace-tolerant typed-call parsing on the active
  Step 4 direct-call ownership path without widening the matcher surface:
  `tests/backend/backend_bir_pipeline_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now cover bounded
  local-arg and first-local-rewrite helper-call slices whose
  `callee_type_suffix` and `args_str` carry extra spacing, proving the native
  x86 call-owner seams continue to accept the shared trimmed call syntax they
  already rely on
- implementation note:
  the existing direct-call parser/emitter path already accepted those spaced
  forms, so this slice lands as focused regression coverage rather than a code
  path rewrite
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_local_arg_call_slice_with_spacing`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_first_local_rewrite_call_slice_with_spacing`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_local_arg_call_slice_with_spacing`, and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_first_local_rewrite_call_slice_with_spacing`
- broad validation note:
  `ctest --test-dir build -R backend_bir_tests --output-on-failure` still hits
  the existing `backend_bir_tests` crash path and exits with the same broad
  blocker surface rather than a whitespace-route regression; this slice did not
  attempt to widen into that separate failure lane

- this iteration wires the translated x86 comparison owner into both active
  x86 source lists in `CMakeLists.txt` and keeps the scope bounded to
  `src/backend/x86/codegen/comparison.cpp`: the live build now carries
  `emit_float_cmp_impl`, `emit_f128_cmp_impl`, and `emit_int_cmp_impl`,
  with constant-backed `F128` callers reusing the existing raw-byte reload
  bridge for x87 compare lowering instead of the parked compare placeholder
- implementation note:
  `src/backend/x86/codegen/comparison.cpp` now mirrors the translated
  value-producing compare sequences for scalar float, `F128`, and integer
  compares, while leaving fused compare+branch, block-branch, and select
  explicitly parked as placeholders for a later bounded owner slice
- implementation note:
  `src/backend/x86/codegen/x86_codegen.hpp` now defines the minimal
  `BlockId` carrier already implied by the public comparison-owner
  signatures so the newly linked comparison translation unit can compile
  without widening the current branch/select behavior
- implementation note:
  `tests/backend/backend_shared_util_tests.cpp` now pins the translated
  comparison-owner link surface and the constant-backed `F128` compare
  contract, including raw-byte stack staging, `fldt (%rsp)` reload,
  `fucomip`/`fstp` lowering, ordered-equality flag materialization, swapped
  ordered-less-than lowering, and destination-slot boolean stores
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_comparison_owner_symbols`,
  `./build/backend_shared_util_tests translated_comparison_owner_reuses_constant_f128_reload_bridge`, and
  `./build/backend_shared_util_tests`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and one new `>30s` note on `backend_bir_tests`

- this iteration wires the translated x86 cast owner into both active x86
  source lists in `CMakeLists.txt` and keeps the scope bounded to
  `src/backend/x86/codegen/cast_ops.cpp`: the live build now carries
  `emit_cast_instrs_impl` and `emit_cast_impl`, with constant-backed `F128`
  callers reusing the existing raw-byte reload bridge for `F128 -> F64` and
  `F128 -> integer` lowering instead of depending on the stale pre-wire
  operand-model assumptions in the parked translation unit
- implementation note:
  `src/backend/x86/codegen/cast_ops.cpp` now compiles against the current
  reduced `Operand`/`X86CodegenState` surface by replacing stale tuple/pointer
  access patterns with the live raw-id APIs, using local labels for the
  translated unsigned `u64 -> F128` recovery path, and falling back through
  `emit_cast_instrs_x86` for the remaining scalar helper-driven cast cases
- implementation note:
  `tests/backend/backend_shared_util_tests.cpp` now pins the translated cast
  owner link surface and the constant-backed `F128` narrowing contract,
  including shared raw-byte stack staging, `fldt (%rsp)` reload, x87
  `fstpl`/`fisttpq` lowering, narrow signed fixup, and destination-slot
  stores for both `F128 -> F64` and `F128 -> I8`
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_cast_owner_symbols`,
  `./build/backend_shared_util_tests translated_cast_owner_reuses_constant_f128_reload_bridge`, and
  `./build/backend_shared_util_tests`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration wires the translated x86 float owner into both active x86
  source lists in `CMakeLists.txt` and keeps the scope bounded to
  `src/backend/x86/codegen/float_ops.cpp`: the live build now carries
  `emit_float_binop_impl` and `emit_unaryop_impl`, with the F128 paths
  reusing `emit_f128_load_to_x87` and `emit_f128_load_finish` so constant
  long-double operands flow through the existing raw-byte reload bridge
  without widening the public operand/header model
- implementation note:
  `src/backend/x86/codegen/float_ops.cpp` now compiles against the real
  `x86_codegen.hpp` surface, keeps the translated x87 F128 binop/unary paths,
  and inlines the narrow non-F128 unary fallback instead of depending on the
  currently-unwired `emit_unaryop_default` surface
- implementation note:
  `tests/backend/backend_shared_util_tests.cpp` now pins the translated float
  owner link surface and the constant-backed F128 add/neg assembly contract,
  including raw-byte stack staging, x87 reload, result restaging through the
  destination slot, accumulator-cache refresh, and direct-slot tracking
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_float_owner_reuses_constant_f128_reload_bridge`,
  `./build/backend_shared_util_tests translated_float_owner_symbols`, and
  `./build/backend_shared_util_tests`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration lands the bounded translated constant long-double reload
  bridge inside `src/backend/x86/codegen/f128.cpp`: `emit_f128_load_to_x87`
  now recognizes the existing state-backed raw-byte payload map and stages
  constant `F128` operands through a temporary stack `fldt` path instead of
  misrouting them through the legacy `f64`-shaped fallback reload
- implementation note:
  `tests/backend/backend_shared_util_tests.cpp` now pins the non-zero-high and
  zero-high constant reload assembly contract directly, including the shared
  `fldt (%rsp)` path, stack cleanup, and zero-high fast path
- surfaced boundary note:
  the translated x86 cast/float/comparison owners that would consume this
  helper are still not wired into the active build/test source lists, so this
  slice intentionally lands the shared helper seam first and leaves the next
  caller-side constant long-double bridge to the first owner that gets wired
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_f128_helper_reloads_constant_raw_bytes`,
  `./build/backend_shared_util_tests translated_memory_owner`,
  `./build/backend_shared_util_tests translated_f128_store_raw_bytes`, and
  `./build/backend_shared_util_tests translated_f128_helpers`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and one new `>30s` note on `backend_bir_tests`

- this iteration lands the bounded translated constant long-double caller
  bridge for the active x86 memory owner: `src/backend/x86/codegen/memory.cpp`
  now routes `F128` constant stores through the real
  `emit_f128_store_raw_bytes` helper by consulting a minimal state-backed raw
  byte payload map keyed by the existing operand id surface instead of
  widening `Operand` into a richer enum
- implementation note:
  `src/backend/x86/codegen/x86_codegen.hpp` and
  `src/backend/x86/codegen/shared_call_support.cpp` now carry the narrow
  `f128_constant_words` state plus setter/getter helpers, and
  `tests/backend/backend_shared_util_tests.cpp` now pins the direct,
  over-aligned, and indirect constant-store assembly contract while asserting
  the old `f64`-via-`x87` fallback text is absent
- surfaced boundary note:
  the bridge is still state-backed rather than a full public operand-model
  expansion, so constant-backed cast/return callers remain the next bounded
  follow-on only if they can consume the same raw-id keyed payload cleanly
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_memory_owner`,
  `./build/backend_shared_util_tests translated_f128_store_raw_bytes`, and
  `./build/backend_shared_util_tests`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration ports the bounded translated raw-byte long-double helper still
  parked in `src/backend/x86/codegen/f128.cpp`: `emit_f128_store_raw_bytes`
  now matches the reference direct, over-aligned, and indirect resolved-address
  paths instead of leaving placeholder text, while keeping the scope limited to
  the helper surface rather than widening the public operand model or wiring
  new constant callers
- implementation note:
  `src/backend/x86/codegen/x86_codegen.hpp` now exposes the helper’s explicit
  `lo`/`hi` raw-byte payload parameters, and
  `tests/backend/backend_shared_util_tests.cpp` now pins the emitted assembly
  contract for direct-slot offset folding, over-aligned address resolution,
  indirect pointer loads, and zero/high-fragment materialization behavior
- surfaced boundary note:
  the helper body is now real, but the active x86 operand surface still does
  not expose a narrow constant long-double form for callers such as the memory
  owner path, so the next bounded move is caller-side constant plumbing rather
  than additional helper placeholder removal inside `f128.cpp`
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_f128_store_raw_bytes`,
  `./build/backend_shared_util_tests translated_memory_owner`, and
  `./build/backend_shared_util_tests`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_before.log`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and one new `>30s` note on `backend_bir_tests`

- this iteration lands the bounded translated unsigned cast-helper bridge
  already surfaced by the parked x86 cast-owner path:
  `src/backend/x86/codegen/f128.cpp` now replaces the
  `emit_f128_to_u64_cast`, `emit_float_to_unsigned`, and
  `emit_u64_to_float` placeholder bodies with the translated threshold,
  bias-recovery, and half-value SSE/x87 sequences while staying inside the
  reduced shared x86 surface by emitting local branch labels directly in
  `f128.cpp`
- implementation note:
  `tests/backend/backend_shared_util_tests.cpp` now pins the unsigned helper
  contract directly, covering the bounded `f128` to `u64` threshold compare
  path, `f64`/`f32` to unsigned integer truncation paths, and `u64` to
  `f64`/`f32` recovery paths while asserting the old placeholder text is gone
- surfaced boundary note:
  the reduced public x86 helper surface still does not expose the translated
  label-emitter helpers used by the fuller owner path, so this slice keeps the
  bridge bounded by generating local labels in `f128.cpp` instead of widening
  `x86_codegen.hpp`; constant-backed long-double operand staging remains the
  next explicit `f128.cpp` boundary
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_f128_unsigned`,
  `./build/backend_shared_util_tests translated_f128_cast_helpers`, and
  `./build/backend_shared_util_tests`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_before.log`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration lands the bounded translated x86 cast-helper bridge already
  surfaced by `src/backend/x86/codegen/cast_ops.cpp`: the parked
  `src/backend/x86/codegen/f128.cpp` helper layer now replaces the
  `emit_cast_instrs_x86`, `emit_int_to_f128_cast`, `emit_f128_to_int_cast`,
  `emit_f128_to_f32_cast`, stack-staged x87 helper, and signed/float generic
  cast placeholders with the translated signed/pointer/float paths that fit
  today’s reduced x86 utility/header surface
- implementation note:
  `tests/backend/backend_shared_util_tests.cpp` now pins the helper dispatch
  contract for narrow signed integer to `F128`, `F128` to `F32`, `F32` to
  `F128`, `I32` to `F64`, and `F64` to `I8` helper-backed casts
- surfaced boundary note:
  the unsigned-specific translated helper paths
  (`emit_f128_to_u64_cast`, `emit_u64_to_float`, and float-to-unsigned) remain
  parked because the current reduced x86 utility surface still does not expose
  unsigned `IrType` variants or the shared label helpers those branches use;
  constant-backed long-double cast inputs remain deferred behind the same
  public operand-surface boundary
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_f128_cast_helpers`,
  `./build/backend_shared_util_tests translated_f128_helpers`, and
  `./build/backend_shared_util_tests`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and one new `>30s` note on `backend_bir_tests`

- this iteration lands the next bounded translated `src/backend/x86/codegen/f128.cpp`
  helper bodies already surfaced by the parked x86 cast-owner path:
  `emit_f128_to_int_from_memory` now reloads direct, over-aligned, and
  indirect F128 sources into x87 before conversion, while
  `emit_f128_st0_to_int` now lowers signed/pointer integer conversion through
  the translated `fisttpq` stack path instead of throwing placeholder text
- implementation note:
  `tests/backend/backend_shared_util_tests.cpp` now pins the helper-backed
  F128 integer-conversion source reload contract across direct, over-aligned,
  and indirect addresses plus the bounded ST0 signed/pointer post-conversion
  fixup behavior
- bounded-scope note:
  this slice intentionally stays inside `f128.cpp`; unsigned-specific
  large-value cast handling and full `cast_ops.cpp` wiring remain deferred
  until the active public x86 `IrType` / operand surface is widened enough to
  carry that translated behavior cleanly
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_f128_helpers`,
  `./build/backend_shared_util_tests translated_memory_owner`, and
  `./build/backend_shared_util_tests`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and one new `>30s` note on `backend_bir_tests`

- this iteration ports the bounded translated F128 store-owner shape now
  surfaced by the active x86 memory path: `src/backend/x86/codegen/memory.cpp`
  now preserves the direct-slot `fldt`/`fstpt` fast path for tracked x87
  values and otherwise falls back through the translated
  `emit_f128_store_f64_via_x87` helper instead of routing every F128 store
  through the generic reload/store path
- implementation note:
  `src/backend/x86/codegen/f128.cpp` now carries the real translated
  `%rax`-via-x87 fallback store helper for direct, over-aligned, and indirect
  destinations, and `tests/backend/backend_shared_util_tests.cpp` now pins the
  direct-slot precision-preserving store path plus the bounded fallback-store
  text contract across direct, over-aligned, and indirect address forms
- surfaced boundary note:
  `emit_f128_store_raw_bytes` remains parked because the active public x86
  `Operand` surface still only exposes raw value ids, so constant-byte staging
  would require a wider ownership decision rather than another bounded helper
  port
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_memory_owner`, and
  `./build/backend_shared_util_tests`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and one new `>30s` note on `backend_bir_tests`

- this iteration wires the parked translated
  `src/backend/x86/codegen/f128.cpp` helper surface into both active x86 source
  lists in `CMakeLists.txt` and keeps the scope bounded to the helper methods
  already used by `memory.cpp` and the translated returns path: direct,
  indirect, and over-aligned F128 address resolution plus the linked
  `fldt`/`fstpt`, load-finish, and x87 reload helpers now live in `f128.cpp`
  instead of `shared_call_support.cpp`
- implementation note:
  `tests/backend/backend_shared_util_tests.cpp` now exercises the wired
  helper-backed F128 memory-owner path directly, pinning direct-slot x87
  reloads, resolved-address F128 stores/loads, destination-slot staging on
  load finish, accumulator-cache refresh, and the direct-slot/provenance state
  updates
- bounded-scope note:
  the rest of `src/backend/x86/codegen/f128.cpp` remains parked and stubbed;
  this slice only advances the helper bodies already exposed through the active
  public/shared x86 surface instead of reopening the full translated F128
  owner or the prologue/runtime dependency chain
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_memory_owner_surface`, and
  `./build/backend_shared_util_tests`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration exposes the bounded translated return-owner state needed by
  `src/backend/x86/codegen/returns.cpp` without widening into the parked
  prologue owner: `src/backend/x86/codegen/x86_codegen.hpp` now carries
  `current_return_type` and function-return eightbyte classes, while
  `src/backend/x86/codegen/returns.cpp` now uses that shared state to emit the
  direct-slot and tracked-source `f128` return reload path instead of throwing
- implementation note:
  `tests/backend/backend_shared_util_tests.cpp` now exercises
  `emit_return_impl` directly, pinning the bounded `f128` current-return-type
  surface plus both the direct-slot and tracked-indirect-source return reload
  paths; `emit_return_i128_to_regs_impl` now reads function-return classes
  rather than the call-result classification cache
- surfaced boundary note:
  the focused `backend_shared_util_tests` link initially failed because this
  target still does not link `src/backend/x86/codegen/prologue.cpp`, so this
  slice keeps the translated owner bounded by emitting the minimal epilogue
  text locally inside `returns.cpp` instead of widening into prologue-owner
  linkage or shared thunk state
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8` and
  `./build/backend_shared_util_tests`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_before.log`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration lands a bounded translated returns-helper slice inside
  `src/backend/x86/codegen/returns.cpp` without reopening the still-blocked
  prologue/runtime-owner state: the translated helper bodies for f32/f64/f128
  return register moves, mixed i128 lane reshuffles, and the second-lane
  f32/f64/f128 slot staging paths now emit real shared-owner-backed x86 text
  instead of unconditional throws
- implementation note:
  `tests/backend/backend_shared_util_tests.cpp` now exercises the linked
  translated returns-owner helper body directly, pinning the emitted text
  contract for the bounded return-register moves, i128 lane reshuffles, second
  return-lane slot staging, and the shared accumulator/f128-direct-slot state
  updates
- bounded-scope note:
  `emit_return_impl` and `current_return_type_impl` remain parked behind the
  existing logic-error guard because the exporter-backed translated return-owner
  state they need is still not exposed through the active public/shared x86
  surface; this slice only advances the already-linkable helper bodies
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_returns_owner`, and
  `./build/backend_shared_util_tests`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and one new `>30s` note on `backend_bir_tests`

- this iteration lands a bounded translated globals-owner behavior slice inside
  `src/backend/x86/codegen/globals.cpp` without widening into the still-blocked
  TLS/PIC or prologue/runtime-owner state: global-address materialization,
  absolute-address materialization, label-address materialization, scalar
  RIP-relative load/store emission, and the thin store/load delegation helpers
  now emit real shared helper-backed x86 text instead of throwing
- implementation note:
  `tests/backend/backend_shared_util_tests.cpp` now exercises the linked
  translated globals-owner body directly, pinning the emitted text contract for
  global-address, label-address, RIP-relative load/store, and the shared
  accumulator-cache invalidation behavior on the store path
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_globals_owner`, and
  `./build/backend_shared_util_tests`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration wires `src/backend/x86/codegen/memory.cpp` into both active
  x86 source lists in `CMakeLists.txt` and keeps the owner linkable by adding
  the missing bounded shared f128 helper definitions directly to
  `src/backend/x86/codegen/shared_call_support.cpp` instead of widening into
  the still-inconsistent parked `src/backend/x86/codegen/f128.cpp` owner
- implementation note:
  `tests/backend/backend_shared_util_tests.cpp` now exercises linked
  translated memory-owner helper behavior directly, covering segment-symbol
  stores, offset/GEP/memcpy helper emission, and one F128 load path that
  records translated load provenance through shared state
- concrete blocker result from the temporary wire-in:
  `memory.cpp` itself compiled cleanly with the active x86 include surface,
  but linking it immediately exposed four missing shared f128 helpers
  (`emit_f128_load_to_x87`, `emit_f128_fstpt`, `emit_f128_fldt`, and
  `emit_f128_load_finish`); the parked `f128.cpp` file remains out of build
  because it still does not match the public x86 header contract
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_memory_owner_surface`, and
  `./build/backend_shared_util_tests`
- broad validation passed:
  `cmake --build --preset default --target c4cll -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and one new `>30s` note on `backend_bir_tests`

- this iteration confirms the minimal `reg_assignments` exposure needed by the
  parked translated memory owner is still a bounded Step 3 move without
  widening into the parked translated prologue owner: the public x86 header
  now carries only shared register-assignment indices, not heavy regalloc type
  ownership, while `src/backend/x86/codegen/memory.cpp` now reads that bounded
  state through `X86CodegenState::assigned_reg_index`
- header-boundary note:
  an initial attempt to expose full `PhysReg` state through
  `src/backend/x86/codegen/x86_codegen.hpp` pulled in `regalloc.hpp` and broke
  the existing `backend_header_boundary_tests` completeness contract, so this
  slice intentionally narrowed back to raw register indices to keep the public
  x86 header lightweight
- direct compile-probe result after the reg-assignment slice:
  `clang++ -std=gnu++17 -fsyntax-only ... src/backend/x86/codegen/memory.cpp`
  now passes with the same include surface used by the real build, so the
  parked translated memory owner is past both the stale translated syntax tier
  and the first hidden register-assignment barrier
- implementation note:
  `src/backend/x86/codegen/shared_call_support.cpp` now preserves the shared
  translated register-assignment index map across state copies/moves, and
  `tests/backend/backend_shared_util_tests.cpp` now pins both the
  declaration-level memory owner surface and the new bounded register-index
  lookup contract
- focused validation passed:
  `cmake --preset default`,
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_memory_owner_surface`,
  `./build/backend_shared_util_tests translated_reg_assignments`, and
  `./build/backend_shared_util_tests translated_shared_call_support`
- broader build validation passed:
  `cmake --build --preset default -j8`
- broad validation note:
  full-suite monotonic guard remains deferred because this slice only advances
  shared header/state coverage plus a parked owner compile-probe boundary; it
  does not yet wire `memory.cpp` into the active runtime x86 path

- this iteration lands the bounded translated memory-owner normalization slice
  without wiring `src/backend/x86/codegen/memory.cpp` into the active x86
  build: `src/backend/x86/codegen/x86_codegen.hpp` now defines the
  `AddressSpace` enum values consumed by the memory owner,
  `src/backend/x86/codegen/memory.cpp` now uses the public `raw` field names
  consistently and replaces the missing top-level default load/store glue with
  bounded local forwarding to the existing const-offset helpers, and
  `tests/backend/backend_shared_util_tests.cpp` now pins the translated memory
  owner method surface through the public x86 header
- direct compile-probe result after the normalization slice:
  `clang++ -std=c++20 -fsyntax-only ... src/backend/x86/codegen/memory.cpp`
  no longer fails on stale translated `.0` field syntax, missing
  `AddressSpace` enumerators, or missing `emit_store_default` /
  `emit_load_default` glue; it now fails first on the next explicit hidden
  owner-state frontier, `X86Codegen::reg_assignments`
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_memory_owner_surface`, and
  `./build/backend_shared_util_tests translated_shared_call_support`
- broad validation note:
  full-suite monotonic guard remains deferred because this slice only advances
  header/test coverage plus a parked owner compile-probe boundary; it does not
  change the active runtime x86 path
- if future wiring exposes missing translated helper bodies shared with other
  parked owner files, treat that as a separate surfaced blocker tier rather
  than reverting this now-clean parser normalization work
- if a future x86 ABI policy change ever enables partial GP-register plus
  caller-stack aggregate splits, re-open `StructSplitRegStack` as a separate
  owner-path cutover item instead of silently folding it into the current
  parked-owner work
- keep the translated prologue owner parked out of build; a brief CMake
  wiring experiment confirmed `src/backend/x86/codegen/prologue.cpp` still
  depends on incomplete public x86 backend/type surface and is not ready for
  direct target inclusion
- treat the current shared backend-entry coverage for the bounded SysV helper
  families as complete enough for this lane unless a new owner-path regression
  shows a real gap
- broad monotonic guard has been rerun for the `calls.cpp` build-wiring slice;
  only rerun it again after the next real owner-path cutover or semantic
  shared-helper upgrade lands

## Current Iteration Notes

- this iteration upgrades the bounded shared x86 call-support layer from
  link-only placeholders into real transitional semantics: the shared state
  now records emitted assembly lines, slot metadata, alloca/over-alignment
  facts, bounded f128 load provenance, and accumulator-cache state while the
  shared output helpers now render actual x86 text forms
- implementation note:
  `src/backend/x86/codegen/x86_codegen.hpp` now carries concrete shared
  storage for the translated helper state/output/cache surface, and
  `src/backend/x86/codegen/shared_call_support.cpp` now binds that state to
  actual `emit`, `get_slot`, `resolve_slot_addr`, output-formatting, and
  reg-cache behavior instead of empty stubs
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_shared_call_support`, and
  `./build/backend_shared_util_tests translated_call_owner_surface`
- broad validation passed:
  `ctest --test-dir build -j8 --output-on-failure > test_fail_before.log`,
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`
  (still exits non-zero on the existing suite failure set), and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing
  tests, and zero new `>30s` tests

- this iteration lands the bounded shared-helper support slice that turns the
  previously declaration-only call-owner dependency cluster into real active
  x86 definitions and wires `src/backend/x86/codegen/calls.cpp` into both x86
  source lists in `CMakeLists.txt`
- implementation note:
  `src/backend/x86/codegen/shared_call_support.cpp` now provides transitional
  definitions for the missing shared call-owner helpers/state/output hooks
  (`operand_to_rax`, `store_rax_to`, `store_rax_rdx_to`, `X86CodegenState`
  emit/slot accessors, selected `X86CodegenOutput` emitters, and the
  `X86CodegenRegCache` hooks), while
  `tests/backend/backend_shared_util_tests.cpp` now pins those output/cache
  hooks alongside the existing translated call-owner surface check
- shippable-scope note:
  the new helper definitions are explicitly transitional build-wiring support;
  `emit.cpp` still owns the active runtime x86 path, so the next frontier is
  real shared state/output semantics rather than link reachability
- focused validation passed:
  `cmake --preset default`,
  `cmake --build --preset default --target backend_shared_util_tests -j8`, and
  `./build/backend_shared_util_tests translated_call_owner_surface`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`
  (still exits non-zero on the existing suite failure set), and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing tests,
  and the existing `backend_bir_tests` `>30s` timeout note still present

- this iteration targets the first parked translated-body syntax blocker inside
  `src/backend/x86/codegen/calls.cpp` after the earlier header-surfacing
  slice: the file still uses Rust-tuple-style field access (`dest.0`,
  `slot->0`) even though the public C++ owner contract now names those fields
  `raw`
- implementation note:
  `src/backend/x86/codegen/calls.cpp` now uses `Value::raw` and
  `StackSlot::raw` consistently for the translated call-result store paths,
  and `tests/backend/backend_shared_util_tests.cpp` now pins that raw-field
  contract through the shared translated call-owner header surface
- direct compile-probe result after the syntax-normalization slice:
  `clang++ -std=c++20 -fsyntax-only ... src/backend/x86/codegen/calls.cpp`
  now passes, so the parked translated call owner is past both the earlier
  incomplete header barrier and the first parser-level translated-body syntax
  tier
- real target-wire result after a temporary `CMakeLists.txt` experiment:
  adding `src/backend/x86/codegen/calls.cpp` to the active x86 source lists
  still fails at link time, not parse time; the file itself compiles, but the
  current public/transitional `x86_codegen.hpp` surface only declares shared
  helper/state methods that do not yet have active x86 definitions
- concrete unresolved shared-owner blockers from the failed link:
  `X86Codegen::operand_to_rax`, `X86Codegen::store_rax_to`,
  `X86Codegen::store_rax_rdx_to`, `X86CodegenState::emit`,
  `X86CodegenState::get_slot`, `X86CodegenOutput::emit_instr_imm_reg`,
  `X86CodegenOutput::emit_instr_rbp_reg`, `X86CodegenOutput::emit_instr_rbp`,
  `X86CodegenRegCache::invalidate_all`, and `X86CodegenRegCache::set_acc`
- shippable-scope note:
  the temporary target-wire change was reverted after the probe because it was
  only used to expose the next blocker tier; the tree remains on the prior
  shippable state while this runbook now records the exact shared-support
  frontier required before `calls.cpp` can enter the real build
- focused validation passed:
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_call_owner_surface`, and the
  direct standalone `clang++ -c` plus earlier `clang++ -fsyntax-only` probes
  for
  `src/backend/x86/codegen/calls.cpp`
- this iteration lands the planned bounded public-surface slice in
  `src/backend/x86/codegen/x86_codegen.hpp` for the translated call-owner
  dependency cluster instead of wiring another top-level owner file blindly
- implementation note:
  the header now exposes minimal compile-time definitions for
  `CallAbiConfig`, `CallArgClass`, `IrType`, `EightbyteClass`,
  `RiscvFloatClass`, `X86CodegenState`, and the shared `X86Codegen` helper
  declarations used by the parked translated call owner, plus focused
  `backend_shared_util_tests.cpp` coverage that pins those declarations through
  the public x86 codegen surface
- direct compile-probe result after the header slice:
  `calls.cpp` advances past the previous incomplete-type and missing-member
  barrier and now fails on the next translation tier inside the parked owner
  body itself, led by untranslated Rust-tuple syntax such as `dest.0`
  alongside other not-yet-normalized translated constructs
- shippable-scope note:
  this iteration intentionally keeps `calls.cpp` out of `CMakeLists.txt`; it
  only surfaces the shared header contract and records the new blocker
  boundary for the next owner-path slice
- focused validation target:
  passed `cmake --preset default`,
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_call_owner_surface`, and a
  direct `clang++ -fsyntax-only` probe for `src/backend/x86/codegen/calls.cpp`
  with the repo's current include surface; the probe now fails first on the
  parked translated body syntax (`dest.0`, `slot->0`) instead of incomplete
  public-surface declarations

- this iteration attempted the recorded next owner-build slice by probing
  `src/backend/x86/codegen/calls.cpp` with the same include surface used by the
  current x86 targets instead of wiring it into `CMakeLists.txt` blindly
- compile-probe result:
  `calls.cpp` is still blocked before link/build wiring because the
  transitional public `x86_codegen.hpp` surface only forward-declares key
  translated dependencies and does not expose the exporter-backed
  `X86Codegen` helpers/state the file uses
- concrete blocker samples from the probe:
  incomplete `CallAbiConfig`, `CallArgClass`, `Operand`, and `IrType`
  definitions plus missing `X86Codegen` members such as `state`,
  `operand_to_rax`, `store_rax_to`, and `store_rax_rdx_to`
- broader reassessment result:
  the same direct compile probe currently fails for every remaining top-level
  translated owner candidate (`alu.cpp`, `atomics.cpp`, `cast_ops.cpp`,
  `comparison.cpp`, `f128.cpp`, `float_ops.cpp`, `i128_ops.cpp`,
  `inline_asm.cpp`, `intrinsics.cpp`, `memory.cpp`, `prologue.cpp`, and
  `variadic.cpp`), so the real next shippable slice is public
  x86-codegen-header surfacing rather than another owner-file CMake wire-in
- focused validation passed:
  `cmake --preset default`,
  `cmake --build --preset default --target backend_shared_util_tests -j8`, and
  direct compile probes for `calls.cpp` plus the remaining untranslated
  top-level owner files using the current x86 target include surface
- broad validation note:
  full-suite monotonic guard remains deferred because no runtime/backend
  behavior changed; this iteration produced blocker inventory rather than a
  landed owner-path cutover

- this iteration wires `src/backend/x86/codegen/asm_emitter.cpp` into both x86
  source lists in `CMakeLists.txt` and adds focused
  `backend_shared_util_tests.cpp` coverage that pins the translated
  asm-emitter owner symbols through the public `x86_codegen.hpp` surface
- implementation note:
  `asm_emitter.cpp` is a bounded compile/link reachability slice only; its
  exported owner methods now carry explicit logic-error stubs so the unit can
  enter the build without widening the blocked exporter-backed `X86Codegen`
  inline-asm state surface yet
- focused validation target:
  passed `cmake --preset default`,
  `cmake --build --preset default --target backend_shared_util_tests -j8`, and
  `./build/backend_shared_util_tests translated_asm_emitter_owner_symbols`
- broader build validation passed:
  `cmake --build --preset default -j8` now compiles
  `src/backend/x86/codegen/asm_emitter.cpp` through both the `backend_bir_tests`
  and `c4cll` x86 source lists
- broad full-suite validation note:
  still deferred for this bounded symbol/link-coverage wiring slice per the
  active plan note to wait for a larger owner-path cutover before rerunning the
  monotonic full-suite guard

- this iteration corrects the stale execution note that still treated
  `globals.cpp` as the first untranslated top-level owner candidate even though
  both main x86 target lists in `CMakeLists.txt` already compile it; the next
  bounded build-wiring slice is `returns.cpp`
- this iteration wires `src/backend/x86/codegen/returns.cpp` into both x86
  source lists in `CMakeLists.txt` and adds focused
  `backend_shared_util_tests.cpp` coverage that pins the translated returns
  owner symbols through the public `x86_codegen.hpp` surface
- implementation note:
  `returns.cpp` cannot yet compile its real translated bodies against the
  current transitional header because the exporter-backed `X86Codegen` return
  state is still private; for now the file carries explicit
  symbol/link-coverage stubs with a targeted logic-error message instead of
  silently depending on hidden state
- focused validation passed:
  `cmake --preset default`,
  `cmake --build --preset default --target backend_shared_util_tests -j8`,
  `./build/backend_shared_util_tests translated_returns_owner_symbols`,
  `./build/backend_shared_util_tests translated_globals_owner`,
  and
  `./build/backend_shared_util_tests regalloc_pruning_helpers`
- broad validation passed:
  `cmake --build --preset default -j8`,
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`
  (still exits non-zero on the existing suite failure set), and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  with `3192` passed / `184` failed before and after, zero newly failing tests,
  and the existing `backend_bir_tests` `>30s` timeout note still present

- this iteration repairs the stale top-level execution state after the
  trailing small-aggregate `StructStack` reassessment: the bounded SysV
  argument-lowering helper families already tracked in this runbook all have
  shared backend-entry coverage now, so the next real Step 3 frontier is
  translated owner/build wiring rather than more helper-family parity work

- this iteration adds the missing shared backend entrypoint coverage for the
  bounded trailing small-aggregate-after-six-scalars `StructStack` helper
  family: `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins
  `make_x86_struct_stack_small_aggregate_param_slot_lir_module()` at
  `c4c::backend::emit_module(...)` so the public x86 backend entry surface,
  not just the direct-emitter seam, owns the SysV route that keeps the first
  six scalar arguments in registers while copying the trailing small aggregate
  through the caller stack area
- public-path behavior note:
  the shared BIR route preserves the same native x86 owner shape for this
  bounded aggregate family, including the helper-side copy from `[rbp + 16]`
  into `%lv.param.p` and the caller-side local-slot staging plus outgoing
  stack-area copy while the first six scalar arguments stay in the SysV
  integer argument registers
- focused validation:
  passed `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_small_struct_stack_param_slot_on_native_x86_path`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_small_struct_stack_param_slot_slice`
- broad validation note:
  still deferred for this bounded aggregate helper-family coverage slice per
  the active plan note to wait for a larger owner-path cutover before
  rerunning the monotonic full-suite guard

- this iteration adds the missing shared backend entrypoint coverage for the
  bounded caller-stack aggregate param-slot helper family:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins
  `make_x86_stack_aggregate_param_slot_lir_module()` at
  `c4c::backend::emit_module(...)` so the public x86 backend entry surface,
  not just the direct-emitter seam, owns the SysV route that copies a large
  by-value aggregate through the caller stack area
- public-path behavior note:
  the shared BIR route preserves the same native x86 owner shape for this
  bounded aggregate family, including the helper-side copy from `[rbp + 16]`,
  `[rbp + 24]`, and `[rbp + 32]` into the `%lv.param.*` slot and the
  caller-side local-slot staging plus outgoing stack-area copy before the
  helper call
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_stack_aggregate_param_slot_on_native_x86_path`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_stack_aggregate_param_slot_slice`
- broad validation note:
  still deferred for this bounded aggregate helper-family coverage slice per
  the active plan note to wait for a larger owner-path cutover before
  rerunning the monotonic full-suite guard

- this iteration adds the missing shared backend entrypoint coverage for the
  bounded register-backed aggregate param-slot helper family:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins
  `make_x86_register_aggregate_param_slot_lir_module()` at
  `c4c::backend::emit_module(...)` so the public x86 backend entry surface,
  not just the direct-emitter seam, owns the by-value register aggregate path
- public-path behavior note:
  the shared BIR route preserves the same native x86 owner shape for this
  bounded aggregate family, including the helper-side `mov QWORD PTR
  [rbp - 8], rdi` param-slot store and the caller-side local-slot aggregate
  staging plus `mov rdi, QWORD PTR [rbp - 8]` reload before the helper call
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_register_aggregate_param_slot_on_native_x86_path`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_register_aggregate_param_slot_slice`
- broad validation note:
  still deferred for this bounded aggregate helper-family coverage slice per
  the active plan note to wait for a larger owner-path cutover before
  rerunning the monotonic full-suite guard

- this iteration adds the missing shared backend entrypoint coverage for the
  bounded mixed register-plus-stack scalar helper family:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins both
  `make_x86_mixed_reg_stack_param_add_lir_module()` and
  `make_x86_mixed_reg_stack_param_add_i64_lir_module()` at
  `c4c::backend::emit_module(...)` so the public x86 backend entry surface,
  not just the direct-emitter seam, owns the SysV route that keeps the first
  argument in its incoming register while loading the seventh scalar argument
  from the caller stack slot
- public-path behavior note:
  the shared BIR route preserves the same native x86 owner shape for both
  bounded siblings, including the helper-side `edi`/`rdi` plus `[rbp + 16]`
  mix and the caller-side first-six-in-registers plus `push 7` / `add rsp, 8`
  sequence around the helper call
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_mixed_reg_stack_param_add_on_native_x86_path`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_mixed_reg_stack_param_add_i64_on_native_x86_path`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_mixed_reg_stack_param_add_slice`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_mixed_reg_stack_param_add_i64_slice`
- broad validation note:
  still deferred for this bounded prepared-LIR helper-family coverage slice per
  the active plan note to wait for a larger owner-path cutover before
  rerunning the monotonic full-suite guard

- this iteration adds the missing shared backend entrypoint coverage for the
  bounded seventh-parameter caller-stack scalar helper family:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins
  `make_x86_seventh_param_stack_add_lir_module()` at
  `c4c::backend::emit_module(...)` so the public x86 backend entry surface,
  not just the direct-emitter seam, owns the SysV route that keeps the first
  six integer arguments in registers and spills the seventh through the
  caller stack slot
- public-path behavior note:
  the shared BIR route preserves the same native x86 owner shape for this
  bounded helper family, including the helper-side `[rbp + 16]` stack load and
  the caller-side `push 7` / `add rsp, 8` sequence around `call add_stack_param`
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_seventh_param_stack_add_on_native_x86_path`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_seventh_param_stack_add_slice`
- broad validation note:
  still deferred for this bounded prepared-LIR helper-family coverage slice per
  the active plan note to wait for a larger owner-path cutover before
  rerunning the monotonic full-suite guard

- this iteration adds the missing shared backend entrypoint coverage for the
  still-direct-emitter-only single-parameter slot helper-call family:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins
  `make_x86_param_slot_add_lir_module()` at `c4c::backend::emit_module(...)`
  so the public x86 backend entry surface, not just the direct-emitter seam,
  owns the bounded helper-alloca/store/reload source shape
- public-path behavior note:
  the shared BIR route canonicalizes that helper body back to
  `mov eax, edi; add eax, 1; ret` before native x86 emission, so the new
  regression asserts native-asm ownership and helper/caller shape rather than
  the direct-emitter-only stack-slot sequence
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_param_slot_add_on_native_x86_path`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_param_slot_add_slice`
- broad validation note:
  still deferred for this bounded prepared-LIR helper-family coverage slice per
  the active plan note to wait for a larger owner-path cutover before
  rerunning the monotonic full-suite guard

- this iteration adds the missing shared backend entrypoint coverage for the
  helper-only `00080.c` `voidfn` body slice:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins
  `make_lir_source_00080_void_helper_only_module()` at
  `c4c::backend::emit_module(...)` so the public x86 backend entry surface,
  not just the direct-emitter seam, owns the standalone helper definition
- this iteration also aligns the nearby x86 public-entry `voidfn` assertions
  with the actual native assembler contract:
  the direct-BIR void-call, minimal void direct-call, and two-function
  `00080.c` backend-entry tests now expect `.type ... @function`, matching the
  already-emitted x86 asm and removing the stale `%function` expectation debt
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_source_00080_void_helper_only_on_native_x86_path`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_source_00080_void_direct_call_zero_return_through_bir_end_to_end`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_void_direct_call_imm_return_through_bir_end_to_end`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_direct_bir_void_direct_call_imm_return_on_native_x86_path`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_source_00080_main_only_void_call_zero_return_on_native_x86_path`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_source_00080_void_helper_only_slice`
- nearby validation note:
  `ctest --test-dir build -R backend_bir_tests --output-on-failure` still
  crashes in the pre-existing typed-LIR/BIR-lowering failure cluster and does
  not provide a clean owned signal for this bounded `voidfn` assertion slice
- broad validation note:
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`
  still exits non-zero on the existing suite failures, but
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  passed with `3192` passed / `184` failed before and after, zero newly
  failing tests, and only the recurring `backend_bir_tests` `>30s` timeout
  note remaining as pre-existing broad-validation noise

- this iteration adds the missing shared backend entrypoint coverage for the
  bounded zero-arg extern/declared helper-call family:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins
  `make_x86_declared_zero_arg_call_lir_module()` at
  `c4c::backend::emit_module(...)` so the public x86 backend entry surface,
  not just the direct-emitter seam, owns the minimal `call @helper_ext()`
  prepared-LIR route
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8` and
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_declared_zero_arg_direct_call_on_native_x86_path`
- nearby validation passed:
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_extern_zero_arg_call_slice`
- broad validation note:
  still deferred for this bounded prepared-LIR direct-call coverage slice per
  the active plan note to wait for a larger owner-path cutover before
  rerunning the monotonic full-suite guard

- this iteration adds the missing shared backend entrypoint coverage for the
  direct-emitter-only main-only `00080.c` prepared-LIR helper-call family:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins
  `make_lir_source_00080_main_only_module()` at
  `c4c::backend::emit_module(...)` so the public x86 backend entry surface,
  not just the direct-emitter seam, owns the bounded
  `voidfn(); return 0;` native prepared-LIR route
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_source_00080_main_only_void_call_zero_return_on_native_x86_path`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_source_00080_main_only_void_call_zero_return_slice`
- nearby validation note:
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_source_00080_void_direct_call_zero_return_through_bir_end_to_end`
  still fails on the pre-existing `.type ... %function` versus
  `.type ... @function` expectation mismatch and was not changed in this slice

- this iteration adds shared backend entrypoint coverage for the bounded
  single-local-arg prepared-LIR helper-call family:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins
  `make_x86_local_arg_call_lir_module()` at `c4c::backend::emit_module(...)`
  so the public x86 backend entry surface, not just the direct-emitter seam,
  owns the local-slot reload before the native helper call
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_local_arg_direct_call_on_native_x86_path`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_local_arg_call_slice`
- broad validation note:
  still deferred for this bounded prepared-LIR direct-call coverage slice per
  the active plan note to wait for a larger owner-path cutover before
  rerunning the monotonic full-suite guard

- this iteration adds the remaining shared backend entrypoint coverage for the
  bounded second-local-arg and both-local-arg two-argument prepared-LIR helper
  families: `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins
  both `add_pair(i32, i32)` shapes at `c4c::backend::emit_module(...)` so the
  public x86 backend entry surface, not just the direct-emitter seam, owns
  the cases where the second operand or both operands reload from local slots
- focused validation passed:
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_second_local_arg_direct_call_on_native_x86_path`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_both_local_arg_direct_call_on_native_x86_path`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_second_local_arg_call_slice`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_arg_call_slice`,
  and
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_local_arg_direct_call_on_native_x86_path`
- broad validation note:
  still deferred for this bounded prepared-LIR direct-call coverage slice per
  the active plan note to wait for a larger owner-path cutover before rerunning
  the monotonic full-suite guard

- this iteration adds the missing shared backend entrypoint coverage for the
  bounded first-local rewrite two-argument prepared-LIR helper family:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins that
  `add_pair(i32, i32)` shape at `c4c::backend::emit_module(...)` so the
  public x86 backend entry surface, not just the direct-emitter seam, now
  owns the case where the first operand round-trips through the trivial
  rewrite/store/reload path before the helper call while the second operand
  stays immediate
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_first_local_rewrite_direct_call_on_native_x86_path`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_first_local_rewrite_call_slice`
- broad validation note:
  still deferred for this bounded prepared-LIR direct-call coverage slice per
  the active plan note to wait for a larger owner-path cutover before rerunning
  the monotonic full-suite guard

- this iteration adds the missing shared backend entrypoint coverage for the
  bounded both-local second-rewrite two-argument prepared-LIR helper family:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins that
  `add_pair(i32, i32)` shape at `c4c::backend::emit_module(...)` so the
  public x86 backend entry surface, not just the direct-emitter seam, now
  owns the case where the first operand reloads from one local slot and the
  second operand round-trips through the trivial rewrite/store/reload path
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_both_local_second_rewrite_direct_call_on_native_x86_path`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_second_rewrite_call_slice`
- broad validation note:
  still deferred for this bounded prepared-LIR direct-call coverage slice per
  the active plan note to wait for a larger owner-path cutover before rerunning
  the monotonic full-suite guard

- this iteration adds the missing shared backend entrypoint coverage for the
  bounded both-local first-rewrite two-argument prepared-LIR helper family:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins that
  `add_pair(i32, i32)` shape at `c4c::backend::emit_module(...)` so the
  public x86 backend entry surface, not just the direct-emitter seam, now
  owns the case where the first operand round-trips through the trivial
  rewrite/store/reload path and the second operand reloads from a sibling
  local slot
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_both_local_first_rewrite_direct_call_on_native_x86_path`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_first_rewrite_call_slice`
- broad validation note:
  still deferred for this bounded prepared-LIR direct-call coverage slice per
  the active plan note to wait for a larger owner-path cutover before rerunning
  the monotonic full-suite guard

- this iteration adds shared backend entrypoint coverage for the bounded
  second-local rewrite two-argument prepared-LIR helper family:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins that
  `add_pair(i32, i32)` shape at `c4c::backend::emit_module(...)` so the
  public x86 backend entry surface, not just the direct-emitter seam, keeps
  the native owner path when the first operand stays immediate and the second
  operand round-trips through the trivial rewrite/store/reload local path
- focused validation passed for this slice:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_second_local_rewrite_direct_call_on_native_x86_path`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_second_local_rewrite_call_slice`
- broad validation note:
  skipped for this still-bounded prepared-LIR direct-call coverage slice per
  the active plan note to defer the monotonic full-suite guard until a larger
  owner-path cutover lands

- this iteration adds shared backend entrypoint coverage for the bounded
  both-local double-rewrite two-argument prepared-LIR helper family so the
  public `c4c::backend::emit_module(...)` x86 path, not just the direct
  emitter seam, now pins the native `add_pair(i32, i32)` ownership shape when
  both operands reload from locals after trivial rewrite/store round-trips
- focused validation passed for this slice:
  `cmake --build --preset default --target backend_bir_tests -j8` and
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_both_local_double_rewrite_direct_call_on_native_x86_path`

- this iteration extended the shared backend entrypoint coverage for the first
  local-arg two-argument prepared-LIR helper family:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins that
  `add_pair(i32, i32)` shape at `c4c::backend::emit_module(...)` so the
  first-operand local reload still reaches native x86 asm emission through the
  public backend entry surface instead of only through the direct-emitter-only
  slice test
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_bir_tests`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_local_arg_direct_call_on_native_x86_path`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_local_arg_call_slice`
- broad validation note:
  skipped for this still-bounded prepared-LIR direct-call coverage slice per
  the active plan note to defer the monotonic full-suite guard until a larger
  owner-path cutover lands

- this iteration moved the bounded folded two-arg `fold_pair(i32, i32)`
  helper family onto the native prepared-LIR x86 emitter path:
  `src/backend/x86/codegen/direct_calls.cpp`,
  `src/backend/x86/codegen/direct_dispatch.cpp`, and
  `src/backend/x86/codegen/x86_codegen.hpp` now recognize the helper body
  `10 + %lhs - %rhs` and emit the helper plus main call path natively instead
  of leaving that prepared-LIR family outside the direct x86 owner matrix
- `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins the native
  prepared-LIR folded-helper slice directly while keeping the existing BIR-
  owned end-to-end folded-call regression in place
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_bir_tests`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_folded_two_arg_direct_call_slice`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_folded_two_arg_direct_call_through_bir_end_to_end`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_helper_call_slice`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_call_crossing_direct_call_slice`
- broad validation note:
  skipped for this still-bounded direct-call owner slice per the active plan
  note to defer the monotonic full-suite guard until a larger owner-path
  cutover lands

- this iteration moved the bounded call-crossing prepared-LIR helper family
  into `src/backend/x86/codegen/direct_calls.cpp`: the native x86 path now
  accepts the `2 + 3`, `call add_one(%t0)`, `%t0 + %t1` shape directly,
  preserving the pre-call source value in `rbx` across the helper call
  instead of throwing before the prepared-LIR seam can own it
- `src/backend/x86/codegen/direct_dispatch.cpp` now routes both the prepared-
  LIR and direct-BIR call-crossing helper family through the same native x86
  asm owner, and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins the native
  prepared-LIR slice plus the aligned direct-BIR/native-prelude expectations
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_bir_tests`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_call_crossing_direct_call_slice`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_call_crossing_direct_call_through_bir_end_to_end`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_direct_bir_minimal_call_crossing_direct_call_end_to_end`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_dual_identity_direct_call_sub_slice`
- broad validation note:
  skipped for this still-bounded direct-call owner slice per the active plan
  note to defer the monotonic full-suite guard until a larger owner-path
  cutover lands

- this iteration moved the bounded dual-helper subtraction family into
  `src/backend/x86/codegen/direct_calls.cpp`: the native prepared-LIR path now
  accepts the minimal `f(i32)` / `g(i32)` identity-helper pair followed by a
  caller-side subtraction instead of forcing that three-function slice through
  shared BIR lowering first
- `src/backend/x86/codegen/direct_dispatch.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pin that ownership
  change directly, including the prepared-LIR direct-emitter regression and
  the shared entrypoint assertion updates that now expect the native x86
  prepared-LIR path to own this slice
- focused validation passed for this slice:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_dual_identity_direct_call_sub_slice`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_dual_identity_direct_call_sub_through_bir_end_to_end`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_helper_call_slice`
- nearby-validation note:
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_call_crossing_direct_call_through_bir_end_to_end`
  still fails with
  `x86 backend emitter does not support this direct LIR module...`; treat that
  as the next prepared-LIR ownership gap rather than a regression in the new
  dual-helper subtraction slice
- broad validation note:
  `ctest --test-dir build -R backend_bir_tests --output-on-failure` is still
  not an owned signal for this bounded direct-call slice because the monolithic
  `backend_bir_tests` binary carries existing unrelated failures and a crash
  outside the new dual-helper subtraction path; monotonic full-suite guard
  remains deferred per the active plan note until a larger owner-path cutover
  lands

- this iteration moved the bounded one-arg helper-call family into
  `src/backend/x86/codegen/direct_calls.cpp`: the native prepared-LIR path now
  accepts both the immediate single-arg `add_one(i32)` helper and the minimal
  single-arg identity helper instead of forcing those shapes through shared
  BIR lowering first
- `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now pins both native
  prepared-LIR slices directly and adjusts the shared-entrypoint add-imm and
  identity assertions to reflect that these modules now stay on the native x86
  path while separate lowering coverage continues to own the shared-BIR checks
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_bir_tests`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_single_arg_add_imm_helper_call_slice`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_single_arg_identity_helper_call_slice`,
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_direct_call_add_imm_on_native_x86_path`,
  and
  `./build/backend_bir_tests test_backend_bir_pipeline_drives_x86_lir_minimal_direct_call_identity_arg_on_native_x86_path`
- broad validation note:
  after rebuilding the full tree with `cmake --build build -j8`, a refreshed
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log`
  stayed monotonic against the current matched rerun baseline
  `test_fail_after_rerun.log`; the regression guard reported
  `3190` passed / `186` failed before and after with no newly failing tests
  in non-decreasing mode
- timeout note:
  the stricter timeout-enforced guard still reports the recurring
  `backend_bir_tests` `>30s` warning as a new suspicious timeout relative to
  `test_fail_after_rerun.log`, so this slice records the monotonic non-
  decreasing pass/fail result while leaving the broad backend-binary timeout
  noise as pre-existing validation debt

- this iteration added the first bounded prepared-LIR direct-emitter coverage
  for a true SysV `ParamClass::StructStack` case:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now includes a helper
  with six leading integer params followed by a small `%struct.Pair` by-value
  aggregate so the trailing aggregate is forced onto the caller stack while
  still flowing through the native direct x86 emitter seam
- `src/backend/x86/codegen/direct_calls.cpp`, `direct_dispatch.cpp`, and
  `x86_codegen.hpp` now recognize and emit that bounded helper/main shape,
  copying one incoming qword from `[rbp + 16]` into `%lv.param.p` on the
  callee side and staging one outgoing qword into a 16-byte caller stack area
  after the first six SysV integer arg registers are filled
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_bir_tests`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_small_struct_stack_param_slot_slice`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_register_aggregate_param_slot_slice`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_stack_aggregate_param_slot_slice`
- broad validation note:
  `ctest --test-dir build -R backend_bir_tests --output-on-failure` is not a
  clean owned signal here because the single `backend_bir_tests` binary still
  times out around existing unrelated failures already outside this bounded
  x86 direct-emitter aggregate slice; monotonic full-suite guard remains
  deferred per the active plan note until a larger owner-path cutover lands

- this iteration added the first backend-facing x86 caller-stack aggregate
  param-slot regression through the native prepared-LIR emitter seam:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now includes a bounded
  `%struct.Big` by-value helper that forces the `ParamClass::LargeStructStack`
  path end to end and asserts on the emitted helper/main assembly
- `src/backend/x86/codegen/direct_calls.cpp`, `direct_dispatch.cpp`, and
  `x86_codegen.hpp` now recognize and emit that bounded caller-stack aggregate
  param-slot slice so the direct x86 emitter accepts the same slot-owner shape
  already mirrored by the parked translated prologue helpers
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_bir_tests`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_stack_aggregate_param_slot_slice`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_register_aggregate_param_slot_slice`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_mixed_reg_stack_param_add_i64_slice`
- broad validation note:
  skipped for this still-bounded owner-path aggregate slice per the current
  plan note to defer the monotonic full-suite guard until a larger owner-path
  cutover lands

- this iteration added the first backend-facing x86 aggregate param-slot
  regression through the native prepared-LIR emitter seam:
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now includes a bounded
  `%struct.Pair` by-value helper that forces the register-backed aggregate
  parameter path end to end and asserts on the emitted caller/prologue assembly
- `src/backend/x86/codegen/direct_calls.cpp`, `direct_dispatch.cpp`, and
  `x86_codegen.hpp` now recognize and emit that bounded register-backed
  aggregate param-slot slice so the direct x86 emitter accepts the same slot
  owner path already covered by the helper-only x86 shared util contracts
- focused validation passed for this slice:
  `cmake --build build -j8`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_register_aggregate_param_slot_slice`,
  and `./build/backend_shared_util_tests`
- broader validation note:
  a broad `./build/backend_bir_tests` run surfaced existing unrelated
  typed-LIR/BIR-lowering failures already outside this x86 aggregate slice and
  did not provide a clean full-binary pass signal for this change

- this iteration resolved the remaining `StructSplitRegStack` decision for the
  current x86 aggregate param-slot owner slice without reopening the broader
  ABI policy: the shared x86 helper surface now exposes the translated
  `allow_struct_split_reg_stack = false` contract directly, the live x86
  `CallAbiConfig` consumes that helper, and focused helper coverage locks the
  ref rule that x86_64 does not permit partial GP-register plus caller-stack
  aggregate splits
- implication:
  because the active x86 ABI config still forbids partial reg/stack aggregate
  splits, `ParamClass::StructSplitRegStack` remains deferred with any future
  ABI-policy change rather than joining the current parked translated prologue
  owner path
- focused validation target for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests` and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation note:
  skipped for this helper-only ABI-policy bookkeeping slice per the current
  plan note to defer the monotonic full-suite guard until a larger owner-path
  cutover lands

- this iteration widened the parked translated aggregate param-slot consumer
  seam in `src/backend/x86/codegen/prologue.cpp` to cover the reference-backed
  by-value and caller-stack copy branches:
  `ParamClass::StructByValReg`, `ParamClass::StructStack`, and
  `ParamClass::LargeStructStack` now route through the previously landed
  shared helper contracts instead of staying outside the parked owner wiring
- `tests/backend/backend_shared_util_tests.cpp` now tightens the helper
  contract used by those branches, including the two-qword by-value size
  boundary, the trailing `r9` register mapping for the second aggregate qword,
  and the third-qword caller-stack copy source offset progression
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests test_x86_translated_asm_emitter_helpers_match_shared_contract`,
  and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- validation note:
  a brief attempt to add `src/backend/x86/codegen/prologue.cpp` to the active
  CMake source lists confirmed the existing plan note: the translated prologue
  owner still depends on incomplete public x86 backend/type surface, so the
  file remains parked out of build for now and this slice is still validated
  through helper-backed coverage rather than live end-to-end owner execution
- broad validation note:
  skipped for this still-bounded parked-owner aggregate slice per the current
  plan note to defer the monotonic full-suite guard until a larger owner-path
  cutover lands

- this iteration consumed the landed x86 param-slot naming bridge in the live
  translated prologue entry path:
  `src/backend/x86/codegen/prologue.cpp` now resolves `%lv.param.*` slots from
  ABI parameter names and wires the first active slot-backed aggregate store
  branches for `ParamClass::StructSseReg`,
  `ParamClass::StructMixedIntSseReg`, and
  `ParamClass::StructMixedSseIntReg` instead of leaving those shapes parked
  behind helper-only consumers
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests backend_bir_tests`,
  `./build/backend_shared_util_tests test_x86_translated_asm_emitter_helpers_match_shared_contract`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_param_slot_add_slice`
- validation note:
  a new end-to-end x86 aggregate regression was attempted but the current x86
  backend entry still rejects those aggregate LIR modules before the translated
  prologue path is exercised, so this slice is currently locked by compile
  coverage plus the focused shared-helper and existing backend-slice tests
- broad validation note:
  skipped for this still-bounded owner-wiring slice per the current plan note
  to defer the monotonic full-suite guard until a larger owner-path cutover
  lands

- this iteration reconstructed the parked translated x86 param-slot naming
  bridge the aggregate prologue store helpers will consume next:
  `src/backend/x86/codegen/mod.cpp` and `x86_codegen.hpp` now expose
  `x86_param_slot_name(...)` and `x86_param_slot_matches(...)` so the
  future slot-backed aggregate storage branch can derive `%lv.param.*` local
  slot names directly from `%p.*` ABI parameter names without reintroducing
  ad hoc `emit.cpp`-local naming logic
- `tests/backend/backend_shared_util_tests.cpp` now locks that contract,
  including valid `%p.arg` / `%p.aggregate.0` mapping and rejection of
  malformed or mismatched names
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests` and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation note:
  skipped for this helper-only parked-owner slice per the current plan note to
  defer the monotonic full-suite guard until a larger owner-path cutover lands

- this iteration landed the first parked translated consumer-side wiring for
  the helper-locked SSE and mixed aggregate storage seams:
  `src/backend/x86/codegen/prologue.cpp` now carries helper-backed parked
  store routines for `StructSseReg`, `StructMixedIntSseReg`, and
  `StructMixedSseIntReg`, mirroring the reference slot-write ordering while
  keeping the translated prologue owner out of the active build
- `tests/backend/backend_shared_util_tests.cpp` now tightens the active helper
  contract around that parked consumer wiring, including the third pure-SSE
  destination offset progression and the empty-register bounds for the mixed
  GP/SSE register lookup helpers
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests` and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation note:
  skipped for this parked-owner helper/consumer slice per the current plan
  note to defer the monotonic full-suite guard until a larger owner-path
  cutover lands

- this iteration added the next shared parked-prologue aggregate storage helper
  seam for the translated x86 owner path: `src/backend/x86/codegen/mod.cpp`
  and `x86_codegen.hpp` now expose the SSE-register aggregate and mixed
  GP/SSE aggregate source-register plus destination-slot offset helpers used by
  the parked `StructSseReg`, `StructMixedIntSseReg`, and
  `StructMixedSseIntReg` storage branches in the reference prologue
- `tests/backend/backend_shared_util_tests.cpp` now locks that contract,
  including `xmm0` / `xmm7`, `rdi` / `r9`, and the slot offset ordering for
  the pure-SSE and mixed aggregate store shapes
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation note:
  skipped for this helper-only parked-owner slice per the current plan note to
  defer the monotonic full-suite guard until a larger owner-path cutover lands

- this iteration added a shared register-backed aggregate storage helper seam
  for the parked translated x86 prologue owner path:
  `src/backend/x86/codegen/mod.cpp` and `x86_codegen.hpp` now expose the
  bounded `ParamClass::StructByValReg` qword-count, incoming GP register, and
  destination-slot offset helpers mirrored from the reference prologue copy
  shape
- `tests/backend/backend_shared_util_tests.cpp` now locks that contract,
  including the one- and two-qword aggregate sizes, `rdi` / `rsi` and `r9`
  register mapping, and destination-slot offset progression for the parked
  by-value aggregate store path
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation note:
  skipped for this helper-only parked-owner slice per the current plan note to
  defer the monotonic full-suite guard until a larger owner-path cutover lands

- this iteration added a shared split register-plus-stack aggregate helper seam
  for the parked translated x86 prologue owner path: `src/backend/x86/codegen/mod.cpp`
  and `x86_codegen.hpp` now expose the incoming GP register name plus the
  qword-copy count and source/destination offset helpers for the caller-stack
  tail used by `ParamClass::StructSplitRegStack`
- `tests/backend/backend_shared_util_tests.cpp` now locks that contract,
  including the first-register `rdi` / `r9` mapping, the ceil-to-qword count
  for the caller-stack remainder beyond the leading register, and the offset
  progression from caller stack to destination slot tail
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation note:
  defer the monotonic full-suite guard for this helper-only parked-owner slice
  per the current plan note until a larger owner-path cutover lands

- this iteration added a shared aggregate stack-copy helper seam for the
  parked translated x86 prologue owner path: `src/backend/x86/codegen/mod.cpp`
  and `x86_codegen.hpp` now expose qword-copy count plus source/destination
  offset helpers for aggregate parameter classes that arrive from the caller
  stack
- `tests/backend/backend_shared_util_tests.cpp` now locks that contract,
  including ceil-to-qword copy counts and the caller-stack/base-slot offset
  progression used by the translated `StructStack` / `LargeStructStack` copy
  shape
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation passed with the regression guard:
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log 2>&1`
  plus
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  reported `3177` passed / `199` failed before and `3190` passed / `186`
  failed after, with no newly failing tests; the checker did note one new
  `>30s` case `backend_bir_tests`

- this iteration added a shared parked-prologue bookkeeping seam for
  non-alloca-backed parameter pre-store tracking:
  `src/backend/x86/codegen/mod.cpp` and `x86_codegen.hpp` now expose
  `x86_mark_param_prestored(...)` and `x86_param_is_prestored(...)` so the
  parked translated prologue owner can record which parameter indices were
  already preserved into their assigned callee-saved location
- `src/backend/x86/codegen/prologue.cpp` now uses that helper seam when
  emitting the landed integer-register and float-register pre-store moves, so
  parked `emit_param_ref_impl(...)` skips duplicate reload emission for those
  already-preserved parameter indices instead of relying on an untracked local
  set mutation
- `tests/backend/backend_shared_util_tests.cpp` now locks the new bookkeeping
  helper contract alongside the existing pre-store helper coverage
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests` and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation note:
  skipped for this helper-only parked-owner slice per the current plan note to
  defer the monotonic full-suite guard until a larger owner-path cutover lands

- this iteration widened the parked translated x86 prologue helper seam from
  the earlier integer-only non-alloca-backed pre-store contract into the first
  float-register owner slice: `src/backend/x86/codegen/mod.cpp` and
  `x86_codegen.hpp` now expose typed float pre-store move/source/destination
  helpers for `f32` and `f64`, including `movd %xmmN, %r32` and
  `movq %xmmN, %r64`
- `src/backend/x86/codegen/prologue.cpp` now routes parked translated
  `ParamClass::FloatReg` non-alloca-backed pre-store emission through that
  shared helper seam, keeping the owner-path logic aligned with the reference
  typed XMM-to-callee-saved register move shape without pulling the full
  translated prologue owner into build
- `tests/backend/backend_shared_util_tests.cpp` now locks the float pre-store
  helper contract alongside the earlier integer pre-store coverage, including
  `xmm0` / `xmm7`, `movd` / `movq`, and typed callee-saved destinations
  `ebx` / `r15`
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation note:
  skipped for this helper-only slice per the current plan note to defer the
  monotonic full-suite guard until a larger owner-path cutover lands

- this iteration widened the parked translated x86 prologue seam into the
  first non-alloca-backed integer pre-store owner slice: `mod.cpp` and
  `x86_codegen.hpp` now expose shared helper accessors for the full-width
  direct pre-store move mnemonic plus ABI-source and callee-saved destination
  register names
- `src/backend/x86/codegen/prologue.cpp` now uses that helper surface to
  preserve uniquely assigned integer-register parameters in their callee-saved
  destination register at function entry, matching the intended translated
  direct pre-store shape without pulling the full prologue owner into build
- `tests/backend/backend_shared_util_tests.cpp` now locks the shared helper
  contract for that pre-store slice, including `movq`, `rdi` / `r9`, and
  `rbx` / `r15`
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `cmake --build build -j8 --target backend_bir_tests`,
  `./build/backend_shared_util_tests test_x86_translated_asm_emitter_helpers_match_shared_contract`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_arg_call_slice`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation note:
  skipped for this helper-only slice per the current plan note to defer the
  monotonic full-suite guard until a larger owner-path cutover lands

- this iteration widened the shared translated `emit_param_ref_impl(...)`
  helper seam from integer-like scalar cases into the first float-register
  owner slice: `src/backend/x86/codegen/mod.cpp` and
  `src/backend/x86/codegen/x86_codegen.hpp` now expose the XMM argument-
  register map plus the typed `movd` / `movq` helper contract for `f32` /
  `f64` `ParamClass::FloatReg` loads
- `src/backend/x86/codegen/prologue.cpp` now routes parked translated
  `emit_param_ref_impl(...)` float-register scalar handling through that
  shared helper seam, keeping the owner-path logic aligned with the reference
  `movd %xmmN, %eax` / `movq %xmmN, %rax` behavior without pulling the full
  translated prologue owner into the active build
- `tests/backend/backend_shared_util_tests.cpp` now locks the float-register
  helper contract alongside the existing integer-like and stack-scalar
  `ParamRef` helper coverage, including `xmm0`..`xmm7`, `movd` for `f32`, and
  `movq` for `f64`
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation passed for monotonicity on this slice:
  `ctest --test-dir build -j8 --output-on-failure > test_before.log 2>&1`,
  `ctest --test-dir build -j8 --output-on-failure > test_after.log 2>&1`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
  reported `3190` passed / `186` failed before and after, with no newly
  failing tests and no new `>30s` tests

- this iteration widened the shared translated `emit_param_ref_impl(...)`
  helper contract coverage from the earlier signed integer-only slice into the
  remaining integer-like scalar owner cases `u32`, `u64`, and `ptr`
- `tests/backend/backend_shared_util_tests.cpp` now locks the full helper-side
  load mnemonic, destination-register, incoming argument-register, and incoming
  stack-operand contract for `u32`, `u64`, and `ptr`, matching the already
  wired helper behavior in `src/backend/x86/codegen/mod.cpp` and the parked
  translated consumer in `src/backend/x86/codegen/prologue.cpp`
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation passed against the stored current-tree rerun baseline:
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log 2>&1`
  plus
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_after_rerun.log --after test_fail_after.log --allow-non-decreasing-passed`
  reported `3190` passed / `186` failed before and after, with no newly
  failing tests and no new `>30s` tests
- next owner-path note:
  keep the next widening point on float-register scalar `ParamRef` handling or
  a deliberate non-alloca-backed pre-store slice before revisiting aggregate
  parameter classes

- this iteration added the first reusable shared helper seam for the parked
  translated x86 `emit_param_ref_impl(...)` owner path: `mod.cpp` and
  `x86_codegen.hpp` now expose typed scalar load helpers for the initial
  integer register and stack-scalar cases, including the `i32` `movslq` /
  `edi` rule and the `i64` `movq` / `rdi` plus
  `QWORD PTR [rbp + 16 + offset]` rule mirrored from the build-active direct
  emitter slices
- `tests/backend/backend_shared_util_tests.cpp` now locks that helper contract,
  proving the shared seam matches the already-covered direct-emitter ABI
  behavior for the first translated `ParamRef` integer scalar register and
  stack loads
- `src/backend/x86/codegen/prologue.cpp` was updated in the parked translated
  owner path to route those first integer scalar `ParamRef` loads through the
  new helper seam instead of the literal `<load-parameter>` placeholder
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation passed for monotonicity on this slice:
  `ctest --test-dir build -j8 --output-on-failure > test_before.log 2>&1`,
  `ctest --test-dir build -j8 --output-on-failure > test_after.log 2>&1`, and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
  reported `3190` passed / `186` failed before and after, with no newly
  failing tests; the checker did note one new `>30s` case `backend_bir_tests`

- this iteration widened the build-active x86 direct-emitter mixed
  register-plus-stack parameter coverage from the prior `i32` helper into an
  `i64` slice: `src/backend/x86/codegen/direct_calls.cpp` now also recognizes a
  bounded `add_first_and_stack_param_i64(i64 %p.a, ..., i64 %p.g)` helper and
  emits the typed SysV sequence with `mov rax, rdi` plus
  `add rax, QWORD PTR [rbp + 16]`
- added focused x86 backend coverage in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` that locks the helper-
  side `rax`/`rdi` plus `QWORD PTR [rbp + 16]` path and the caller-side
  six-register-plus-one-stack `i64` argument setup for the same bounded slice
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_bir_tests`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_mixed_reg_stack_param_add_i64_slice`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_mixed_reg_stack_param_add_slice`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_seventh_param_stack_add_slice`
- next owner-path note:
  use the paired `i32`/`i64` direct-emitter slices as the typed scalar ABI
  reference when extracting the first reusable helper seam for the parked
  translated `emit_param_ref_impl(...)` owner path

- this iteration widened the build-active x86 direct-emitter parameter coverage
  from the prior stack-only seventh-argument helper into a mixed
  register-plus-stack slice: `src/backend/x86/codegen/direct_calls.cpp` and
  `direct_dispatch.cpp` now recognize a bounded
  `add_first_and_stack_param(i32 %p.a, ..., i32 %p.g)` helper and emit the
  real SysV calling sequence with `%edi` preserved for the first parameter plus
  `%rbp+16` loading for the seventh incoming stack parameter in the same
  helper body
- added focused x86 backend coverage in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` that locks both the
  helper-side `mov eax, edi` plus `add eax, DWORD PTR [rbp + 16]` sequence and
  the caller-side six-register-plus-one-stack argument setup for the mixed
  parameter path
- next owner-path note:
  keep using these build-active direct-emitter parameter slices to pin the
  intended ABI behavior while the translated `prologue.cpp` owner stays out of
  build, then resume the helper-surface work needed for the first real
  translated `emit_param_ref_impl(...)` cutover

- this iteration added a build-active x86 direct-emitter stack-scalar slice
  for the first incoming stack argument beyond the six SysV integer arg
  registers: `src/backend/x86/codegen/direct_calls.cpp` now recognizes a
  bounded seven-parameter helper shape and emits the helper with an explicit
  `%rbp+16` incoming stack load plus caller-side seventh-arg stack staging and
  cleanup
- `src/backend/x86/codegen/direct_dispatch.cpp` and
  `src/backend/x86/codegen/x86_codegen.hpp` now expose that new
  `try_emit_minimal_seventh_param_stack_add_module(...)` seam so the active
  build can exercise a concrete stack-scalar parameter path even while the
  translated `prologue.cpp` owner remains out of build
- added focused x86 backend coverage in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` that locks the new
  seventh-parameter helper route, including the helper-side
  `mov eax, DWORD PTR [rbp + 16]` load and the caller-side `push 7` /
  `add rsp, 8` stack-arg discipline
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_bir_tests`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_seventh_param_stack_add_slice`,
  and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_param_slot_add_slice`
- investigation note:
  `src/backend/x86/codegen/prologue.cpp` is still intentionally out of the
  active build and does not compile standalone against the current public
  `x86_codegen.hpp` surface because that header only exposes forward
  declarations for much of the translated owner state; keep the next owner-path
  slice scoped to helper-surface expansion or a deliberate header-wiring
  change, not a silent CMake hookup

- this iteration removed the invalid placeholder emission from
  `src/backend/x86/codegen/prologue.cpp` `emit_store_params_impl(...)` and
  replaced it with real x86 SysV parameter-classification caching via
  `classify_params_full(...)`, so the provisional owner path now records the
  translated parameter classes instead of emitting a literal
  `<store-parameters>` asm stub
- exposed `x86_param_stack_offset(...)` through
  `src/backend/x86/codegen/mod.cpp` and `x86_codegen.hpp` so the incoming
  `%rbp+16+offset` stack-parameter address rule is shared and test-locked for
  the next `emit_param_ref_impl` stack-scalar slice
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation passed against the stored matched rerun baseline:
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log 2>&1`
  plus
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_after_rerun.log --after test_fail_after.log --allow-non-decreasing-passed`
  reported `3190` passed / `186` failed before and after, with no newly
  failing tests; the checker did note one new `>30s` case
  `backend_bir_tests`, but timeout enforcement was not enabled for this slice

- this iteration exposed the translated x86 parameter-storage / `ParamRef`
  helper seam through `src/backend/x86/codegen/mod.cpp` and
  `x86_codegen.hpp`: `x86_arg_reg_name(...)`,
  `x86_param_stack_base_offset()`, `x86_phys_reg_is_callee_saved(...)`, and
  `x86_param_can_prestore_direct_to_reg(...)`
- `src/backend/x86/codegen/calls.cpp` now routes outgoing integer argument
  register selection through that shared helper surface instead of a local
  file-only register-name table, keeping call-side ABI register naming aligned
  with the future translated prologue parameter-owner path
- added focused shared-util coverage that locks the translated parameter ABI
  register order, the `%rbp+16` incoming stack-parameter base, and the
  dead-param-alloca pre-store eligibility rule that only allows direct
  pre-store to unshared callee-saved destinations
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests` and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`

- this iteration exposed the translated x86 variadic register-save-area layout
  helpers through `src/backend/x86/codegen/mod.cpp` and `x86_codegen.hpp`:
  `x86_variadic_gp_save_offset(...)` and
  `x86_variadic_sse_save_offset(...)`
- `src/backend/x86/codegen/prologue.cpp` now routes its variadic prologue
  register-save-area emission through that shared helper surface, including the
  six GPR home slots and the eight 16-byte XMM save slots when SSE is enabled,
  instead of leaving the variadic save area disconnected from the provisional
  translated prologue path
- added focused shared-util coverage that locks the translated variadic
  register-save-area offset contract, including the `base + gp_index*8`
  mapping for `%rdi`..`%r9` and the `base + 48 + xmm_index*16` mapping for
  `%xmm0`..`%xmm7`
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests test_x86_translated_asm_emitter_helpers_match_shared_contract`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation note:
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log 2>&1`
  and a second rerun to `test_fail_after_rerun.log` both settled at the same
  current-tree result; the rerun-to-rerun guard passed via
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_after.log --after test_fail_after_rerun.log --allow-non-decreasing-passed`
  with `3190` passed / `186` failed before and after
- blocker note:
  the stored matched full-suite baseline still diverges from the current tree
  by the same four x86 route/c-testsuite regressions already tracked below:
  `backend_codegen_route_x86_64_c_testsuite_00030_repeated_call_compare_zero_return_retries_after_direct_bir_rejection`,
  `backend_codegen_route_x86_64_c_testsuite_00031_local_i32_inc_dec_compare_retries_after_direct_bir_rejection`,
  `c_testsuite_x86_backend_src_00030_c`, and
  `c_testsuite_x86_backend_src_00031_c`

- this iteration exposed the translated x86 prologue stack-probe and
  callee-saved spill-slot helpers through `src/backend/x86/codegen/mod.cpp`
  and `x86_codegen.hpp`:
  `x86_stack_probe_page_size()`, `x86_needs_stack_probe(...)`, and
  `x86_callee_saved_slot_offset(...)`
- `src/backend/x86/codegen/prologue.cpp` now routes its frame-growth and
  callee-saved save/restore offset decisions through that shared helper surface,
  including the translated page-probe loop for frames larger than 4096 bytes,
  instead of keeping placeholder local ownership for those rules
- added focused shared-util coverage that locks the translated prologue
  stack-probe threshold and callee-saved spill-offset contract, including the
  4096-byte page threshold and `-frame_size + slot*8` offset rule
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests test_x86_translated_asm_emitter_helpers_match_shared_contract`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation note:
  `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log 2>&1`
  plus
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_matched_before.log --after test_fail_after.log --allow-non-decreasing-passed`
  still reported `3190` passed / `186` failed versus the stored matched
  baseline `3194` passed / `182` failed, with the same four x86 route/c-testsuite
  regressions already tracked below:
  `backend_codegen_route_x86_64_c_testsuite_00030_repeated_call_compare_zero_return_retries_after_direct_bir_rejection`,
  `backend_codegen_route_x86_64_c_testsuite_00031_local_i32_inc_dec_compare_retries_after_direct_bir_rejection`,
  `c_testsuite_x86_backend_src_00030_c`, and
  `c_testsuite_x86_backend_src_00031_c`
- blocker note:
  the stored matched full-suite baseline still diverges from the current tree
  by those same four x86 route/c-testsuite failures, so this helper slice is
  validated by focused backend coverage plus confirmation that no additional
  full-suite failures appeared beyond the already-tracked mismatch

- this iteration exposed the translated x86 caller-saved pruning helper
  through `src/backend/x86/codegen/mod.cpp` and `x86_codegen.hpp`:
  `x86_prune_caller_saved_regs(...)`
- `src/backend/x86/codegen/prologue.cpp` now routes its current
  indirect-call-sensitive caller-saved register-pool selection through that
  shared helper instead of clearing the entire caller-saved pool ad hoc
- added focused shared-util coverage that locks the translated caller-saved
  pruning contract for the prologue/regalloc path, including the combined
  indirect-call + i128 + atomic narrowing case
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests test_x86_translated_regalloc_pruning_helpers_match_shared_contract`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation note:
  `ctest --test-dir build -j8 --output-on-failure > test_after.log 2>&1`,
  `ctest --test-dir build -j8 --output-on-failure > test_after_rerun.log 2>&1`,
  and
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_matched_before.log --after test_after_rerun.log --allow-non-decreasing-passed`
  still reported the same `3190` passed / `186` failed current-tree result
  against the stored matched baseline `3194` passed / `182` failed, with the
  same four x86 route/c-testsuite regressions already tracked below:
  `backend_codegen_route_x86_64_c_testsuite_00030_repeated_call_compare_zero_return_retries_after_direct_bir_rejection`,
  `backend_codegen_route_x86_64_c_testsuite_00031_local_i32_inc_dec_compare_retries_after_direct_bir_rejection`,
  `c_testsuite_x86_backend_src_00030_c`, and
  `c_testsuite_x86_backend_src_00031_c`

- this iteration implemented the already-declared shared x86 regalloc seam in
  `src/backend/x86/codegen/mod.cpp`:
  `run_shared_x86_regalloc(...)` now builds its config from the translated
  `x86_callee_saved_regs()` and `x86_caller_saved_regs()` helper surface and
  routes through the shared backend regalloc handoff
- added focused shared-util coverage that locks the new x86 regalloc seam to
  the translated register-pool contract, proving call-spanning values come from
  the translated callee-saved pool, non-call-spanning values come from the
  translated caller-saved pool, and cached liveness stays visible through the
  shared handoff
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests test_x86_shared_regalloc_helper_uses_translated_register_pools`,
  `./build/backend_shared_util_tests`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation note:
  the first post-build full-suite rerun changed 12
  `llvm_gcc_c_torture_*` failures at the same `3190` passed / `186` failed
  count, but a second rerun matched the original baseline exactly
- broad validation passed on the matched rerun:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after_rerun.log --allow-non-decreasing-passed`
  reported `3190` passed / `186` failed before and after, with no newly
  failing tests and no new `>30s` cases

- this iteration exposed the translated x86 prologue-side frame helpers
  through `src/backend/x86/codegen/mod.cpp` and `x86_codegen.hpp`:
  `x86_variadic_reg_save_area_size(...)` and
  `x86_aligned_frame_size(...)`
- `src/backend/x86/codegen/prologue.cpp` now reuses that shared helper surface
  for variadic register-save-area sizing and 16-byte frame alignment instead
  of keeping those ref-owned constants/rules local to the provisional
  prologue owner file
- added focused shared-util coverage that locks the translated prologue-side
  frame-sizing contract, including the `176`/`48` variadic save-area split and
  positive-only 16-byte frame rounding behavior
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests test_x86_translated_asm_emitter_helpers_match_shared_contract`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation note:
  after `ctest --test-dir build -j8 --output-on-failure > test_after.log 2>&1`,
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_matched_before.log --after test_after.log --allow-non-decreasing-passed`
  still reported `3190` passed / `186` failed versus the stored matched
  baseline `3194` passed / `182` failed, with the same four x86 route/c-testsuite
  regressions already tracked below:
  `backend_codegen_route_x86_64_c_testsuite_00030_repeated_call_compare_zero_return_retries_after_direct_bir_rejection`,
  `backend_codegen_route_x86_64_c_testsuite_00031_local_i32_inc_dec_compare_retries_after_direct_bir_rejection`,
  `c_testsuite_x86_backend_src_00030_c`, and
  `c_testsuite_x86_backend_src_00031_c`
- blocker note:
  the stored matched full-suite baseline still diverges from the current tree
  by those four x86 route/c-testsuite failures, so the helper-extraction lane
  remains focused on targeted backend validation until a later cutover or
  dedicated regression-triage slice resolves that baseline mismatch

- this iteration exposed translated x86 inline-asm register-width, register
  formatting, and GCC condition-code helpers through
  `src/backend/x86/codegen/mod.cpp` and `x86_codegen.hpp`:
  `x86_reg_name_to_64(...)`, `x86_reg_name_to_16(...)`,
  `x86_reg_name_to_8l(...)`, `x86_reg_name_to_8h(...)`,
  `x86_format_reg(...)`, and `x86_gcc_cc_to_x86(...)`
- `src/backend/x86/codegen/inline_asm.cpp` now reuses that shared helper
  surface for `format_x86_reg(...)`, `reg_to_32(...)`, `reg_to_64(...)`,
  `reg_to_16(...)`, `reg_to_8l(...)`, and `gcc_cc_to_x86(...)` instead of
  keeping placeholder local passthroughs
- added focused shared-util coverage that locks the translated inline-asm
  register-width and condition-code mapping contract, including xmm/x87
  passthrough behavior for width formatting
- focused validation passed for this slice:
  `cmake --build build -j8 --target backend_shared_util_tests`,
  `./build/backend_shared_util_tests`, and
  `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- broad validation note:
  after `cmake --build --preset default -j8` plus
  `ctest --test-dir build -j8 --output-on-failure > test_after.log 2>&1`,
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_matched_before.log --after test_after.log --allow-non-decreasing-passed`
  reported `3190` passed / `186` failed versus the stored matched baseline
  `3194` passed / `182` failed, with four newly failing x86 route/c-testsuite
  cases:
  `backend_codegen_route_x86_64_c_testsuite_00030_repeated_call_compare_zero_return_retries_after_direct_bir_rejection`,
  `backend_codegen_route_x86_64_c_testsuite_00031_local_i32_inc_dec_compare_retries_after_direct_bir_rejection`,
  `c_testsuite_x86_backend_src_00030_c`, and
  `c_testsuite_x86_backend_src_00031_c`
- blocker note:
  those four broad-suite regressions exercise direct-LIR/direct-BIR fallback
  coverage for simple compare/inc-dec programs and do not involve inline asm;
  they remain unresolved and should be triaged separately before relying on the
  stored matched full-suite baseline as a hard gate for the next x86 helper
  slice

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

- moved the bounded one-arg prepared-LIR helper-call family out of
  `emit.cpp` into `src/backend/x86/codegen/direct_calls.cpp`
- added direct x86 regressions that pin the native single-arg add-immediate
  and single-arg identity helper-call slices
- updated the shared x86 LIR add-immediate and identity route checks to assert
  the native x86 ownership path explicitly instead of implying a still-active
  shared-BIR emission route for those shapes
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
- this iteration extends the active Step 4 direct-call ownership path with the
  missing first-local and both-local plain two-arg typed-call suffix-spacing
  regressions: helper and native x86 prepared-LIR coverage now pin
  suffix-spaced `callee_type_suffix` on those two already-landed helper
  families without widening matcher scope
- implementation note:
  `tests/backend/backend_bir_pipeline_tests.cpp` and
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` now add explicit
  `with_suffix_spacing` fixtures and assertions for the first-local plain
  two-arg and both-local plain two-arg helper-call families so the existing
  typed-call parser keeps separately pinned suffix-spacing seams adjacent to
  the already-covered generic spacing routes
- implementation note:
  the existing direct-call parser/emitter path already accepted those
  suffix-spaced forms, so this slice lands as focused regression coverage
  rather than a code path rewrite
- focused validation passed:
  `cmake --build --preset default --target backend_bir_tests -j8`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_local_arg_call_slice_with_suffix_spacing`,
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_both_local_arg_call_slice_with_suffix_spacing`,
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_local_arg_call_slice_with_suffix_spacing`, and
  `./build/backend_bir_tests test_x86_direct_emitter_lowers_minimal_two_arg_both_local_arg_call_slice_with_suffix_spacing`
