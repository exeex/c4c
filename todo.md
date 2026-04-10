# Frontend Compile-Time Hotspots And Extraction Candidates

Status: Active
Source Idea: ideas/open/02_frontend_compile_time_hotspots_and_extraction_candidates.md
Source Plan: plan.md

## Active Item

- Step 5: refresh the optimized hotspot snapshot after the
  `stmt_emitter_call.cpp` builtin split, then pick the lower-risk next slice
  between the remaining HIR leaders (`hir_templates.cpp` and `hir_stmt.cpp`).

## Completed

- Activated the lowest-numbered eligible idea from `ideas/open/` into
  `plan.md`.
- Created `todo.md` aligned to the same source idea.
- Recorded the `ctest --test-dir build -j --output-on-failure` baseline in
  `test_fail_before.log`.
- Reconfigured `build` with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` so Step 1
  measurements use the generated CMake compile commands.
- Measured the initial parser, HIR, and LIR hotspot list and wrote the ranked
  inventory into `docs/frontend_compile_time_hotspot_inventory.md`.
- Copied the durable Step 1 ranking and first-pass interpretation back into
  `ideas/open/02_frontend_compile_time_hotspots_and_extraction_candidates.md`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3319/3319 tests passing before and after and no new failures.
- Measured `-fsyntax-only`, `-O0 -c`, and optimized `-O2 -c` for
  `stmt_emitter_expr.cpp`, `hir_expr.cpp`, `hir_stmt.cpp`,
  `hir_templates.cpp`, and `stmt_emitter_call.cpp`.
- Classified the full top-five HIR/LIR hotspot tier as optimizer heavy and
  recorded the evidence in
  `docs/frontend_compile_time_hotspot_inventory.md` and the source idea.
- Sampled `-ftime-report` on `stmt_emitter_expr.cpp` and `hir_templates.cpp`;
  both spend most total wall time in `phase opt and generate`, with
  `callgraph functions expansion` as the largest reported pass family.
- Ranked the first concrete Step 3 extraction seams and recorded them in
  `docs/frontend_compile_time_hotspot_inventory.md` and the source idea.
- Executed the first Step 4 slice by moving the AArch64 `va_arg` lowering
  cluster out of `src/codegen/lir/stmt_emitter_expr.cpp` into
  `src/codegen/lir/stmt_emitter_vaarg.cpp`.
- Rebuilt after the split and re-ran focused variadic coverage:
  `backend_lir_aarch64_variadic_*`, `backend_runtime_variadic_*`,
  `abi_abi_variadic_*`, and the `llvm_gcc_c_torture` `stdarg`/`va_arg` subset.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3319/3319 tests passing before and after and no new failures.
- Recorded the first before/after extraction measurement: the optimized
  single-TU compile for `src/codegen/lir/stmt_emitter_expr.cpp` improved from
  6.712s to 5.493s after the split, and the new
  `src/codegen/lir/stmt_emitter_vaarg.cpp` compiles in 2.799s.
- Added `tests/c/internal/positive_case/ok_expr_unary_binary_runtime.c` as a
  focused runtime check for scalar unary/binary, pointer arithmetic, logical
  short-circuiting, compound assignment, and simple complex arithmetic.
- Executed the second Step 4 slice by moving the binary-expression lowering
  cluster (`emit_complex_binary_arith`, `emit_rval_payload(..., BinaryExpr,
  ...)`, and `emit_logical`) out of
  `src/codegen/lir/stmt_emitter_expr.cpp` into the new
  `src/codegen/lir/stmt_emitter_expr_binary.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `positive_sema_ok_expr_unary_binary_runtime_c`,
  `positive_sema_ok_expr_canonical_cast_index_c`,
  `positive_sema_ok_expr_canonical_types_c`,
  `smoke_aggregate_access.c`, and `smoke_call_lowering.c` in compare mode.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3320/3320 tests passing after the new focused test was added and no new
  failures.
- Recorded the second before/after extraction measurement: the optimized
  single-TU compile for `src/codegen/lir/stmt_emitter_expr.cpp` improved from
  6.021s before the split to 3.401s after it, and the new
  `src/codegen/lir/stmt_emitter_expr_binary.cpp` compiles in 2.679s.
- Re-measured the current hotspot tier after the two
  `stmt_emitter_expr.cpp` splits: `hir_expr.cpp` is now 4.933s,
  `hir_templates.cpp` 4.295s, `hir_stmt.cpp` 4.275s,
  `stmt_emitter_call.cpp` 3.547s, and `stmt_emitter_expr.cpp` 2.986s on the
  optimized single-TU compile commands from `build/compile_commands.json`.
- Chose `src/frontend/hir/hir_templates.cpp` as the next Step 4 slice because
  it remains in the leading hotspot tier while `stmt_emitter_expr.cpp` has
  dropped below both `hir_templates.cpp` and `stmt_emitter_call.cpp`.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/template_member_owner_signature_local_hir.cpp`
  and wired the new `cpp_hir_template_member_owner_signature_local` test into
  `tests/cpp/internal/InternalTests.cmake`.
- Executed the third Step 4 slice by moving the deferred
  template-owner/member-typedef helper cluster out of
  `src/frontend/hir/hir_templates.cpp` into the new
  `src/frontend/hir/hir_templates_member_typedef.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_template_member_owner_chain`,
  `cpp_hir_template_member_owner_field_and_local`,
  `cpp_hir_template_member_owner_signature_local`,
  `cpp_positive_sema_template_alias_member_typedef_dependent_ref_runtime_cpp`,
  `cpp_positive_sema_template_alias_member_typedef_reordered_owner_runtime_cpp`,
  `cpp_positive_sema_template_member_owner_resolution_cpp`, and
  `cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3321/3321 tests passing after the new focused HIR test was added and no
  new failures.
- Recorded the third before/after extraction measurement: the optimized
  single-TU compile for `src/frontend/hir/hir_templates.cpp` was 4.295s before
  the split and ranged from 4.342s to 4.527s across three post-split reruns;
  the new `src/frontend/hir/hir_templates_member_typedef.cpp` ranged from
  1.443s to 1.487s.
- Recreated `build/compile_commands.json`, rebuilt the tree, and re-ran the
  active hotspot ranking: `hir_expr.cpp` 9.281s, `hir_stmt.cpp` 8.841s,
  `hir_templates.cpp` 8.780s, `stmt_emitter_call.cpp` 6.327s, and
  `stmt_emitter_expr.cpp` 3.770s.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/hir_expr_call_member_helper_hir.cpp` and wired
  the new `cpp_hir_expr_call_member_helper` test into
  `tests/cpp/internal/InternalTests.cmake`.
- Executed the fourth Step 4 slice by moving the `hir_expr.cpp` call-lowering
  helper cluster (`try_lower_template_struct_call`, `lower_call_arg`,
  `try_expand_pack_call_arg`, `try_lower_member_call_expr`,
  `lower_call_expr`, and `try_lower_consteval_call_expr`) into the new
  `src/frontend/hir/hir_expr_call.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_positive_sema_hir_expr_call_member_helper_runtime_cpp`,
  `cpp_positive_sema_operator_call_rvalue_ref_runtime_cpp`,
  `cpp_positive_sema_template_operator_call_rvalue_ref_runtime_cpp`, and
  `cpp_hir_expr_call_member_helper`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3322/3322 tests passing after the new focused HIR test was added and no
  new failures.
- Recorded the fourth before/after extraction measurement: the optimized
  single-TU compile for `src/frontend/hir/hir_expr.cpp` improved from 9.281s
  to 3.549s after the split, and the new `src/frontend/hir/hir_expr_call.cpp`
  compiles in 2.884s.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/hir_stmt_range_for_helper_hir.cpp` and wired
  the new `cpp_hir_stmt_range_for_helper` test into
  `tests/cpp/internal/InternalTests.cmake`.
- Executed the fifth Step 4 slice by moving the `NK_RANGE_FOR` lowering branch
  out of `src/frontend/hir/hir_stmt.cpp` into the new
  `src/frontend/hir/hir_stmt_range_for.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_stmt_range_for_helper` and
  `cpp_positive_sema_range_for_const_cpp`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3323/3323 tests passing after the new focused HIR test was added and no
  new failures.
- Recorded the fifth before/after extraction measurement: compiling the
  pre-split `src/frontend/hir/hir_stmt.cpp` from `HEAD` on the generated
  optimized command took 5.231s, the post-split
  `src/frontend/hir/hir_stmt.cpp` took 10.111s, and the new
  `src/frontend/hir/hir_stmt_range_for.cpp` took 1.958s.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/template_struct_body_instantiation_hir.cpp`
  and wired the new `cpp_hir_template_struct_body_instantiation` test into
  `tests/cpp/internal/InternalTests.cmake`.
- Executed the sixth Step 4 slice by moving the template-struct
  instantiation/body helper cluster
  (`apply_template_typedef_bindings`, `materialize_template_array_extent`,
  `append_instantiated_template_struct_bases`,
  `register_instantiated_template_struct_methods`,
  `record_instantiated_template_struct_field_metadata`,
  `instantiate_template_struct_field`,
  `append_instantiated_template_struct_fields`, and
  `instantiate_template_struct_body`) out of
  `src/frontend/hir/hir_templates.cpp` into the new
  `src/frontend/hir/hir_templates_struct_instantiation.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_template_struct_body_instantiation`,
  `cpp_hir_template_struct_inherited_method_binding`,
  `cpp_hir_template_struct_field_array_extent`,
  `cpp_hir_template_method_param_reentrant_lowering`,
  `cpp_positive_sema_template_member_owner_resolution_cpp`, and
  `cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3324/3324 tests passing after the new focused HIR test was added and no
  new failures.
- Recorded the sixth before/after extraction measurement: compiling the
  pre-split `src/frontend/hir/hir_templates.cpp` from `HEAD` on the generated
  optimized command took 5.087s, the post-split
  `src/frontend/hir/hir_templates.cpp` took 4.241s, and the new
  `src/frontend/hir/hir_templates_struct_instantiation.cpp` took 1.384s.
- Refreshed the optimized hotspot ranking after the struct-instantiation split:
  `hir_templates.cpp` is now 4.237s, `stmt_emitter_call.cpp` 3.996s,
  `hir_stmt.cpp` 3.901s, `hir_expr.cpp` 3.834s, and
  `stmt_emitter_expr.cpp` 3.261s.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/template_inherited_member_typedef_trait_hir.cpp`
  and wired the new `cpp_hir_template_inherited_member_typedef_trait` test
  into `tests/cpp/internal/InternalTests.cmake`.
- Executed the seventh Step 4 slice by moving the template member-typedef type
  resolution cluster (`resolve_struct_member_typedef_type` and
  `resolve_struct_member_typedef_if_ready`) out of
  `src/frontend/hir/hir_templates.cpp` into the new
  `src/frontend/hir/hir_templates_type_resolution.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_template_inherited_member_typedef_trait`,
  `cpp_hir_template_member_owner_chain`,
  `cpp_hir_template_member_owner_field_and_local`,
  `cpp_hir_template_member_owner_signature_local`,
  `cpp_positive_sema_template_variable_alias_inherited_member_typedef_runtime_cpp`,
  `cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`,
  `cpp_positive_sema_template_alias_member_typedef_dependent_ref_runtime_cpp`,
  `cpp_positive_sema_template_alias_member_typedef_reordered_owner_runtime_cpp`,
  and `cpp_positive_sema_template_member_owner_resolution_cpp`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3325/3325 tests passing after the new focused HIR test was added and no
  new failures.
- Recorded the seventh before/after extraction measurement: compiling the
  pre-split `src/frontend/hir/hir_templates.cpp` from `HEAD` on the generated
  optimized command took 4.283s, the post-split
  `src/frontend/hir/hir_templates.cpp` took 4.021s, and the new
  `src/frontend/hir/hir_templates_type_resolution.cpp` took 1.581s.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/template_value_arg_static_member_trait_hir.cpp`
  and wired the new `cpp_hir_template_value_arg_static_member_trait` test into
  `tests/cpp/internal/InternalTests.cmake`.
- Executed the eighth Step 4 slice by moving the template value-argument and
  static-member-const helper family
  (`assign_template_arg_refs_from_ast_args`,
  `resolve_ast_template_value_arg`,
  `try_eval_template_static_member_const`, and
  `try_eval_instantiated_struct_static_member_const`) out of
  `src/frontend/hir/hir_templates.cpp` into the new
  `src/frontend/hir/hir_templates_value_args.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_template_value_arg_static_member_trait`,
  `cpp_hir_template_alias_deferred_nttp_static_member`,
  `cpp_hir_template_deferred_nttp_static_member_expr`, and
  `cpp_hir_template_inherited_member_typedef_trait`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3326/3326 tests passing after the new focused HIR test was added and no
  new failures.
- Recorded the eighth before/after extraction measurement: compiling the
  pre-split `src/frontend/hir/hir_templates.cpp` from `HEAD` on the generated
  optimized command took 4.468s, the post-split
  `src/frontend/hir/hir_templates.cpp` took 3.681s, and the new
  `src/frontend/hir/hir_templates_value_args.cpp` compiled in 1.229s.
- Refreshed the optimized hotspot ranking after the value-arg split:
  `hir_templates.cpp` measured 4.279s, `stmt_emitter_call.cpp` 4.086s,
  `hir_stmt.cpp` 3.923s, `hir_expr.cpp` 3.615s, and
  `stmt_emitter_expr.cpp` 3.270s.
- Added `tests/c/internal/positive_case/ok_call_builtin_runtime.c` as focused
  runtime coverage for the builtin-call helper family split from
  `stmt_emitter_call.cpp`.
- Executed the ninth Step 4 slice by moving the builtin-call helper family
  (`prepare_builtin_int_arg`, `narrow_builtin_int_result`, the
  `emit_builtin_*` / `promote_builtin_*` methods, `emit_post_builtin_call`,
  and the final builtin dispatch inside `emit_rval_payload`) out of
  `src/codegen/lir/stmt_emitter_call.cpp` into the new
  `src/codegen/lir/stmt_emitter_call_builtin.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `positive_sema_ok_call_builtin_runtime_c` and
  `tests/c/internal/compare_case/smoke_call_lowering.c` in compare mode.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3327/3327 tests passing after the new focused runtime test was added and
  no new failures.
- Recorded the ninth before/after extraction measurement: compiling the
  pre-split `src/codegen/lir/stmt_emitter_call.cpp` from `HEAD^` on the
  generated optimized command took 4.463s, the post-split
  `src/codegen/lir/stmt_emitter_call.cpp` took 4.007s, and the new
  `src/codegen/lir/stmt_emitter_call_builtin.cpp` compiled in 2.718s.

## Next Slice

- `stmt_emitter_call.cpp` has now dropped after the builtin split, so the next
  iteration should re-measure the active hotspot tier before picking another
  extraction target.
- Unless a refreshed ranking shows `stmt_emitter_call.cpp` still leading, the
  next measured slice should likely return to the remaining HIR leaders,
  starting with the lower-risk seam between `hir_templates.cpp` and
  `hir_stmt.cpp`.

## Blockers

- None recorded.

## Notes

- Follow numeric ordering from `ideas/open/`; the active source idea is `02`.
- Step 1 is complete for the initial target set.
- The original hotspot inventory has been superseded by the refreshed HIR-led
  rankings recorded during the current Step 4 iterations.
- The first-pass top five hotspot units all preprocess to roughly 84k to 86k
  lines and have 345 to 384 include-tree entries.
- Step 2 is complete: the top-five hotspot tier is optimizer heavy rather than
  parse-heavy, though all five keep a meaningful `-fsyntax-only` floor.
- The latest `ctest --test-dir build -j --output-on-failure` rerun passes
  3327/3327 tests, and the monotonic regression guard remains green.
- The first executed extraction slice reduced the hottest TU,
  `src/codegen/lir/stmt_emitter_expr.cpp`, by 1.219s on the optimized
  single-TU compile command.
- The second extraction slice reduced `src/codegen/lir/stmt_emitter_expr.cpp`
  by another 2.620s versus the immediate pre-split baseline used this
  iteration.
- The earlier refreshed ranking that led with `src/frontend/hir/hir_expr.cpp`
  at 4.933s has now itself been superseded by the post-split order led by
  `src/frontend/hir/hir_stmt.cpp` and `src/frontend/hir/hir_templates.cpp`.
- The third extraction slice preserved behavior but did not show a clear
  compile-time improvement on `src/frontend/hir/hir_templates.cpp`, so no
  compile-time win should be claimed from this helper split in isolation.
- The refreshed post-split hotspot order now leads with
  `src/frontend/hir/hir_stmt.cpp` at 4.710s and
  `src/frontend/hir/hir_templates.cpp` at 4.634s, while
  `src/frontend/hir/hir_expr.cpp` has dropped to 3.549s after the call helper
  extraction.
- The fifth extraction slice preserved behavior and added focused range-for HIR
  coverage, but the compile-time measurement moved in the wrong direction, so
  no compile-time win should be claimed from the `hir_stmt.cpp` range-for
  split.
- The sixth extraction slice improved `src/frontend/hir/hir_templates.cpp`
  from 5.087s to 4.241s on the generated optimized single-TU compile command,
  so unlike the earlier member-typedef split it does count as a measured
  hotspot reduction for that TU.
- The refreshed post-seventh-slice hotspot order is now led by
  `src/frontend/hir/hir_templates.cpp` at 4.247s, followed by
  `src/codegen/lir/stmt_emitter_call.cpp` at 4.097s,
  `src/frontend/hir/hir_stmt.cpp` at 4.063s,
  `src/frontend/hir/hir_expr.cpp` at 3.641s, and
  `src/codegen/lir/stmt_emitter_expr.cpp` at 3.258s.
- The seventh extraction slice reduced `src/frontend/hir/hir_templates.cpp`
  from 4.283s to 4.021s on the generated optimized single-TU compile command,
  so this type-resolution split counts as another measured hotspot reduction
  for that TU.
- The latest full-suite rerun passes 3325/3325 tests, and the monotonic
  regression guard remains green.
- The eighth extraction slice reduced `src/frontend/hir/hir_templates.cpp`
  from 4.468s to 3.681s on the generated optimized single-TU compile command,
  so this value-argument/static-member split also counts as a measured hotspot
  reduction for that TU.
- The latest full-suite rerun passes 3326/3326 tests, and the monotonic
  regression guard remains green.
