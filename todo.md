# Frontend Compile-Time Hotspots And Extraction Candidates

Status: Active
Source Idea: ideas/open/02_frontend_compile_time_hotspots_and_extraction_candidates.md
Source Plan: plan.md

## Active Item

- Step 4: extract the next measured `hir_stmt.cpp` helper family, because the
  refreshed direct-TU comparison after the scalar/control split now reads
  `hir_stmt.cpp` at `2.612s`, `stmt_emitter_call_vaarg_amd64.cpp` at `2.291s`,
  `hir_templates.cpp` at `2.002s`, and the reduced `hir_expr.cpp` at
  `1.123s`.

## Completed

- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/hir_expr_scalar_control_helper_hir.cpp` and
  wired the new `cpp_hir_expr_scalar_control_helper` test into
  `tests/cpp/internal/InternalTests.cmake`.
- Executed the twenty-fourth Step 4 slice by moving the scalar/control
  expression helper family
  (`lower_var_expr`, `lower_unary_expr`, `lower_postfix_expr`,
  `lower_addr_expr`, `lower_deref_expr`, `lower_comma_expr`,
  `lower_binary_expr`, `lower_assign_expr`,
  `lower_compound_assign_expr`, `lower_cast_expr`, `lower_va_arg_expr`,
  `lower_index_expr`, `lower_ternary_expr`, `lower_generic_expr`,
  `lower_stmt_expr`, `lower_complex_part_expr`, `lower_sizeof_expr`, and
  `lower_sizeof_pack_expr`) out of `src/frontend/hir/hir_expr.cpp` into the
  new `src/frontend/hir/hir_expr_scalar_control.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_expr_scalar_control_helper`,
  `cpp_hir_expr_operator_member_helper`,
  `cpp_hir_expr_call_member_helper`, and
  `cpp_hir_expr_object_materialization_helper`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard
  passed with `3330/3330` tests passing before and `3342/3342` after, with no
  new failures.
- Recorded the twenty-fourth before/after extraction measurement: compiling
  the pre-split `src/frontend/hir/hir_expr.cpp` from `HEAD` on the generated
  optimized command took `2.556s`, the reduced
  `src/frontend/hir/hir_expr.cpp` took `1.470s`, and the new
  `src/frontend/hir/hir_expr_scalar_control.cpp` compiled in `2.356s` on the
  direct comparison rerun.
- Refreshed the direct hotspot comparison after the scalar/control split:
  `hir_stmt.cpp` measured `2.612s`,
  `stmt_emitter_call_vaarg_amd64.cpp` `2.291s`,
  `hir_templates.cpp` `2.002s`, and the reduced `hir_expr.cpp` `1.123s`.

- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/hir_expr_operator_member_helper_hir.cpp` and
  wired the new `cpp_hir_expr_operator_member_helper` test into
  `tests/cpp/internal/InternalTests.cmake`.
- Executed the twenty-third Step 4 slice by moving the overloaded
  operator/member-expression helper family
  (`try_lower_rvalue_ref_storage_addr`, `resolve_ref_overload`,
  `find_pending_method_by_mangled`, `try_lower_operator_call`,
  `lower_member_expr`, and `maybe_bool_convert`) out of
  `src/frontend/hir/hir_expr.cpp` into the new
  `src/frontend/hir/hir_expr_operator.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_expr_operator_member_helper`,
  `cpp_hir_expr_call_member_helper`,
  `cpp_hir_expr_object_materialization_helper`,
  `cpp_positive_sema_operator_arrow_member_basic_cpp`,
  `cpp_positive_sema_operator_bool_member_basic_cpp`, and
  `cpp_positive_sema_operator_subscript_member_basic_cpp`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard
  passed with `3330/3330` tests passing before and `3341/3341` after, with no
  new failures.
- Recorded the twenty-third before/after extraction measurement: compiling the
  pre-split `src/frontend/hir/hir_expr.cpp` from `HEAD` on the generated
  optimized command took `2.960s`, the reduced
  `src/frontend/hir/hir_expr.cpp` took `2.230s`, and the new
  `src/frontend/hir/hir_expr_operator.cpp` compiled in `1.690s` on the direct
  comparison rerun.
- Refreshed the direct hotspot comparison after the operator/member split:
  `hir_expr.cpp` measured `2.480s`, `hir_stmt.cpp` `2.450s`,
  `stmt_emitter_call_vaarg_amd64.cpp` `2.370s`, and
  `hir_templates.cpp` `2.100s`.

- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/template_function_deduction_binding_hir.cpp`
  and wired the new `cpp_hir_template_function_deduction_binding` test into
  `tests/cpp/internal/InternalTests.cmake`.
- Executed the twenty-second Step 4 slice by moving the template
  call-binding and deduction helper family
  (`build_call_bindings`, `build_call_nttp_bindings`, `has_forwarded_nttp`,
  `try_infer_arg_type_for_deduction`, `try_deduce_template_type_args`,
  `deduction_covers_all_type_params`, `fill_deduced_defaults`, and
  `merge_explicit_and_deduced_type_bindings`) out of
  `src/frontend/hir/hir_templates.cpp` into the new
  `src/frontend/hir/hir_templates_deduction.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_template_function_deduction_binding`,
  `cpp_hir_forward_pick_ref_specialization_identity`, and
  `cpp_positive_sema_template_arg_deduction_cpp`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard
  passed with `3330/3330` tests passing before and `3340/3340` after, with no
  new failures.
- Recorded the twenty-second before/after extraction measurement: compiling the
  pre-split `src/frontend/hir/hir_templates.cpp` from `HEAD` on the generated
  optimized command took `4.623s`, the reduced
  `src/frontend/hir/hir_templates.cpp` took `3.907s`, and the new
  `src/frontend/hir/hir_templates_deduction.cpp` compiled in `2.809s` on the
  direct comparison rerun.
- Refreshed the direct hotspot comparison after the deduction split:
  `hir_expr.cpp` measured `2.740s`, `hir_templates.cpp` `2.592s`,
  `hir_stmt.cpp` `2.510s`, and
  `stmt_emitter_call_vaarg_amd64.cpp` `2.492s`.

- Added focused x86_64 LLVM IR coverage in
  `backend_ir_x86_64_variadic_pair_amd64_vaarg` to pin the AMD64 register /
  stack `va_arg` join emitted for `tests/c/internal/backend_case/variadic_pair_second.c`.
- Executed the sixteenth Step 4 slice by moving the remaining AMD64 `va_arg`
  helper family (`load_amd64_va_list_ptrs`, `emit_amd64_va_arg`,
  `emit_amd64_va_arg_from_registers`, and
  `emit_amd64_va_arg_from_overflow`) out of
  `src/codegen/lir/stmt_emitter_call.cpp` into the new
  `src/codegen/lir/stmt_emitter_call_vaarg_amd64.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `backend_ir_x86_64_variadic_pair_amd64_vaarg`,
  `backend_runtime_variadic_double_bytes`,
  `backend_runtime_variadic_pair_second`,
  `backend_runtime_variadic_sum2`,
  `abi_abi_variadic_forward_wrapper_c`,
  `abi_abi_variadic_struct_result_c`, and
  `abi_abi_variadic_va_copy_accumulate_c`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3330/3330 tests passing before and 3337/3337 after, with no new
  failures.
- Recorded the sixteenth before/after extraction measurement: compiling the
  pre-split `src/codegen/lir/stmt_emitter_call.cpp` from `HEAD` on the
  generated optimized command took 3.368s, the reduced
  `src/codegen/lir/stmt_emitter_call.cpp` took 0.545s, and the new
  `src/codegen/lir/stmt_emitter_call_vaarg_amd64.cpp` took 3.017s on the
  direct comparison rerun and 3.613s on the refreshed hotspot pass.
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
- Refreshed the optimized hotspot ranking after the builtin-call split using
  the generated commands from `build/compile_commands.json` in their recorded
  build directories: `hir_stmt.cpp` measured 3.876s,
  `hir_templates.cpp` 3.699s, `hir_expr.cpp` 3.471s,
  `stmt_emitter_expr.cpp` 3.406s, and `stmt_emitter_call.cpp` 2.958s.
- Chose `src/frontend/hir/hir_stmt.cpp` as the next Step 4 slice because it
  has retaken the lead over `hir_templates.cpp`, and the `switch` / `case` /
  `default` lowering family inside `lower_stmt_node` is a tighter,
  lower-risk extraction seam than returning to template-heavy helper motion.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/hir_stmt_switch_helper_hir.cpp` and wired the
  new `cpp_hir_stmt_switch_helper` test into
  `tests/cpp/internal/InternalTests.cmake`.
- Executed the tenth Step 4 slice by moving the `NK_SWITCH`, `NK_CASE`,
  `NK_CASE_RANGE`, and `NK_DEFAULT` lowering family out of
  `src/frontend/hir/hir_stmt.cpp` into the new
  `src/frontend/hir/hir_stmt_switch.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_stmt_switch_helper`,
  `cpp_hir_stmt_range_for_helper`,
  `cpp_positive_sema_constexpr_local_switch_cpp`,
  `positive_sema_ok_enum_scope_no_leak_after_block_c`, and
  `negative_tests_bad_flow_continue_in_switch`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3325/3325 tests passing before and 3328/3328 after, with no new
  failures.
- Recorded the tenth before/after extraction measurement: compiling the
  pre-split `src/frontend/hir/hir_stmt.cpp` from `HEAD` on the generated
  optimized command took 4.914s, the post-split
  `src/frontend/hir/hir_stmt.cpp` took 3.952s on the direct comparison rerun,
  and the new `src/frontend/hir/hir_stmt_switch.cpp` compiled in 1.074s.
- Refreshed the optimized hotspot ranking after the switch-family split:
  `hir_stmt.cpp` measured 4.764s, `hir_templates.cpp` 3.945s,
  `hir_expr.cpp` 3.712s, `stmt_emitter_expr.cpp` 3.707s, and
  `stmt_emitter_call.cpp` 3.180s.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/hir_stmt_control_flow_helper_hir.cpp` and
  wired the new `cpp_hir_stmt_control_flow_helper` test into
  `tests/cpp/internal/InternalTests.cmake`.
- Executed the eleventh Step 4 slice by moving the remaining basic
  control-flow lowering family (`NK_IF`, `NK_WHILE`, `NK_FOR`, and
  `NK_DO_WHILE`) out of `src/frontend/hir/hir_stmt.cpp` into the new
  `src/frontend/hir/hir_stmt_control_flow.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_stmt_control_flow_helper`,
  `cpp_hir_stmt_switch_helper`,
  `cpp_hir_stmt_range_for_helper`,
  `cpp_positive_sema_constexpr_local_switch_cpp`,
  `positive_sema_ok_enum_scope_no_leak_after_block_c`, and
  `negative_tests_bad_flow_continue_in_switch`.
- Re-ran the full suite into `test_after.log`; the regression guard passed
  with 3328/3328 tests passing before and 3329/3329 after, with no new
  failures.
- Recorded the eleventh before/after extraction measurement: compiling the
  pre-split `src/frontend/hir/hir_stmt.cpp` from `HEAD` on the generated
  optimized command took 3.95s, the post-split
  `src/frontend/hir/hir_stmt.cpp` took 3.57s, and the new
  `src/frontend/hir/hir_stmt_control_flow.cpp` compiled in 0.97s.
- Measured the current `src/frontend/hir/hir_templates.cpp` optimized
  single-TU compile at 4.14s, which leaves it ahead of the newly reduced
  `src/frontend/hir/hir_stmt.cpp` in the direct comparison.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/template_global_specialization_hir.cpp` and
  wired the new `cpp_hir_template_global_specialization` test into
  `tests/cpp/internal/InternalTests.cmake`.
- Executed the twelfth Step 4 slice by moving the template-global helper
  family (`collect_template_global_definitions`,
  `find_template_global_primary`,
  `find_template_global_specializations`, and
  `ensure_template_global_instance`) out of
  `src/frontend/hir/hir_templates.cpp` into the new
  `src/frontend/hir/hir_templates_global.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_template_global_specialization`,
  `cpp_hir_template_value_arg_static_member_trait`,
  `cpp_hir_template_inherited_member_typedef_trait`, and
  `cpp_positive_sema_qualified_variable_template_compare_parse_cpp`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3325/3325 tests passing before and 3330/3330 after, with no new
  failures.
- Recorded the twelfth before/after extraction measurement: the direct
  optimized compile of `src/frontend/hir/hir_templates.cpp` improved from
  4.14s to 3.864s, and the new
  `src/frontend/hir/hir_templates_global.cpp` compiled in 1.658s.
- Reconfigured `build` with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`, rebuilt, and
  refreshed the current direct hotspot comparison from
  `build/compile_commands.json`: `hir_stmt.cpp` measured 7.659s,
  `hir_templates.cpp` 6.379s, `hir_expr.cpp` 4.406s, and
  `stmt_emitter_call.cpp` 3.181s, which invalidated the earlier
  `hir_templates.cpp`-led follow-on assumption.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/hir_stmt_local_decl_helper_hir.cpp` and wired
  the new `cpp_hir_stmt_local_decl_helper` test into
  `tests/cpp/internal/InternalTests.cmake`.
- Executed the thirteenth Step 4 slice by moving
  `Lowerer::lower_local_decl_stmt` out of
  `src/frontend/hir/hir_stmt.cpp` into the new
  `src/frontend/hir/hir_stmt_decl.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_stmt_local_decl_helper`,
  `cpp_positive_sema_default_ctor_basic_cpp`,
  `cpp_positive_sema_copy_init_basic_cpp`,
  `cpp_positive_sema_copy_move_init_basic_cpp`,
  `cpp_positive_sema_rvalue_ref_decl_basic_cpp`, and
  `cpp_positive_sema_ctor_init_member_typedef_ctor_runtime_cpp`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3330/3330 tests passing before and 3331/3331 after, with no new
  failures.
- Recorded the thirteenth before/after extraction measurement: compiling the
  pre-split `src/frontend/hir/hir_stmt.cpp` from `HEAD` on the generated
  optimized command took 3.725s, the post-split
  `src/frontend/hir/hir_stmt.cpp` took 2.540s, and the new
  `src/frontend/hir/hir_stmt_decl.cpp` compiled in 2.520s.
- Refreshed the optimized hotspot ranking after the local-declaration split:
  `hir_expr.cpp` is now 3.653s, `hir_templates.cpp` 3.579s,
  `stmt_emitter_call.cpp` 2.937s, and the reduced `hir_stmt.cpp` 2.540s.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/hir_expr_object_materialization_helper_hir.cpp`
  and wired the new `cpp_hir_expr_object_materialization_helper` test into
  `tests/cpp/internal/InternalTests.cmake`.
- Executed the fourteenth Step 4 slice by moving the object-materialization
  helper family out of `src/frontend/hir/hir_expr.cpp` into the new
  `src/frontend/hir/hir_expr_object.cpp`, including
  `hoist_compound_literal_to_global`,
  `try_lower_direct_struct_constructor_call`,
  `describe_initializer_list_struct`,
  `materialize_initializer_list_arg`,
  `lower_compound_literal_expr`,
  `lower_new_expr`, and `lower_delete_expr`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_expr_object_materialization_helper`,
  `cpp_hir_expr_call_member_helper`,
  `cpp_hir_builtin_layout_query_sizeof_type`,
  `cpp_hir_builtin_layout_query_alignof_type`,
  `cpp_hir_builtin_layout_query_alignof_expr`,
  `cpp_llvm_initializer_list_runtime_materialization`,
  `cpp_positive_sema_new_delete_side_effect_runtime_cpp`, and
  `cpp_positive_sema_class_specific_new_delete_side_effect_runtime_cpp`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3330/3330 tests passing before and 3332/3332 after, with no new
  failures.
- Recorded the fourteenth before/after extraction measurement: compiling the
  pre-split `src/frontend/hir/hir_expr.cpp` from `HEAD` on the generated
  optimized command took 4.087s, the post-split `src/frontend/hir/hir_expr.cpp`
  took 2.639s, and the new `src/frontend/hir/hir_expr_object.cpp` compiled in
  2.179s.
- Refreshed the optimized hotspot ranking after the object-materialization
  split: `hir_templates.cpp` is now 3.522s, `stmt_emitter_call.cpp` 3.036s,
  `hir_stmt.cpp` 2.773s, and the reduced `hir_expr.cpp` 2.639s.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/template_struct_arg_materialization_hir.cpp`
  and wired the new `cpp_hir_template_struct_arg_materialization` test into
  `tests/cpp/internal/InternalTests.cmake`.
- Executed the fifteenth Step 4 slice by moving the template-struct argument
  materialization and mangling cluster
  (`materialize_template_args`, `prepare_template_struct_instance`,
  `build_template_mangled_name`, and the private `HirTemplateArgMaterializer`
  family) out of `src/frontend/hir/hir_templates.cpp` into the new
  `src/frontend/hir/hir_templates_materialization.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_template_struct_arg_materialization`,
  `cpp_hir_template_struct_body_instantiation`,
  `cpp_hir_template_value_arg_static_member_trait`, and
  `cpp_hir_template_global_specialization`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard passed
  with 3330/3330 tests passing before and 3333/3333 after, with no new
  failures.
- Recorded the fifteenth before/after extraction measurement: compiling the
  pre-split `src/frontend/hir/hir_templates.cpp` from `HEAD^` on the generated
  optimized command took 4.145s, the post-split
  `src/frontend/hir/hir_templates.cpp` measured 3.250s on the direct
  comparison rerun and 2.953s on the refreshed hotspot sweep, and the new
  `src/frontend/hir/hir_templates_materialization.cpp` compiled in 2.786s.
- Refreshed the optimized hotspot ranking after the materialization split:
  `stmt_emitter_expr.cpp` measured 3.435s, `stmt_emitter_call.cpp` 3.106s,
  `hir_templates.cpp` 2.953s, `hir_expr.cpp` 2.639s, and `hir_stmt.cpp`
  2.565s.

## Next Slice

- Stay in `src/frontend/hir/hir_expr.cpp` next, because the refreshed direct
  comparison after the operator/member split still leaves the reduced TU at
  `2.480s`, just ahead of `hir_stmt.cpp` at `2.450s`, the reduced AMD64
  `va_arg` TU at `2.370s`, and `hir_templates.cpp` at `2.100s`.

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
  3341/3341 tests, and the monotonic regression guard remains green.
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
- The refreshed hotspot order after the builtin-call split is now led by
  `src/frontend/hir/hir_stmt.cpp` at 3.876s, with
  `src/frontend/hir/hir_templates.cpp` close behind at 3.699s.
- The tenth extraction slice reduced `src/frontend/hir/hir_stmt.cpp` from
  4.914s to 3.952s on the direct `HEAD` versus working-tree compile
  comparison, so unlike the earlier range-for split it does count as a
  measured hotspot reduction for that TU.
- The latest post-split hotspot rerun still leads with
  `src/frontend/hir/hir_stmt.cpp` at 4.764s, followed by
  `src/frontend/hir/hir_templates.cpp` at 3.945s.
- The eleventh extraction slice reduced `src/frontend/hir/hir_stmt.cpp` from
  3.95s to 3.57s on the direct `HEAD` versus working-tree compile comparison,
  so this control-flow split counts as another measured hotspot reduction for
  that TU.
- The latest full-suite rerun passes 3329/3329 tests, and the monotonic
  regression guard remains green.
- The post-split direct comparison now has `src/frontend/hir/hir_templates.cpp`
  at 4.14s versus `src/frontend/hir/hir_stmt.cpp` at 3.57s, so the next
  highest-value slice should move back to `hir_templates.cpp`.
- The twelfth extraction slice reduced `src/frontend/hir/hir_templates.cpp`
  from 4.14s to 3.864s on the direct optimized compile command, so this
  template-global split counts as another measured hotspot reduction for that
  TU.
- The latest full-suite rerun passes 3330/3330 tests, and the monotonic
  regression guard remains green.
- The refreshed direct hotspot comparison before the latest slice measured
  `src/frontend/hir/hir_stmt.cpp` at 7.659s and
  `src/frontend/hir/hir_templates.cpp` at 6.379s, so the previous
  `hir_templates.cpp`-first follow-on plan no longer matched the measured
  state.
- The thirteenth extraction slice reduced `src/frontend/hir/hir_stmt.cpp`
  from 3.725s to 2.540s on the direct `HEAD` versus working-tree compile
  comparison, so this local-declaration split counts as another measured
  hotspot reduction for that TU.
- The latest full-suite rerun passes 3331/3331 tests, and the monotonic
  regression guard remains green.
- The refreshed post-split hotspot order is now led by
  `src/frontend/hir/hir_expr.cpp` at 3.653s, followed closely by
  `src/frontend/hir/hir_templates.cpp` at 3.579s, so the next
  highest-value slice should inspect `hir_expr.cpp` before returning to the
  remaining template-heavy seams.
- The fourteenth extraction slice reduced `src/frontend/hir/hir_expr.cpp`
  from 4.087s to 2.639s on the direct `HEAD` versus working-tree compile
  comparison, so this object-materialization split counts as another measured
  hotspot reduction for that TU.
- The latest full-suite rerun passes 3332/3332 tests, and the monotonic
  regression guard remains green.
- The refreshed post-split hotspot order is now led by
  `src/frontend/hir/hir_templates.cpp` at 3.522s, followed by
  `src/codegen/lir/stmt_emitter_call.cpp` at 3.036s,
  `src/frontend/hir/hir_stmt.cpp` at 2.773s, and the reduced
  `src/frontend/hir/hir_expr.cpp` at 2.639s, so the next
  highest-value slice should move back to `hir_templates.cpp`.
- The fifteenth extraction slice reduced `src/frontend/hir/hir_templates.cpp`
  from 4.145s to 3.250s on the direct `HEAD^` versus working-tree compile
  comparison, so this materialization/mangling split counts as another
  measured hotspot reduction for that TU.
- The latest full-suite rerun passes 3333/3333 tests, and the monotonic
  regression guard remains green.
- The refreshed post-fifteenth-slice hotspot order is now led by
  `src/codegen/lir/stmt_emitter_expr.cpp` at 3.435s, followed by
  `src/codegen/lir/stmt_emitter_call.cpp` at 3.106s and the reduced
  `src/frontend/hir/hir_templates.cpp` at 2.953s, so the next
  highest-value slice should shift back to LIR helper extraction before
  returning to the remaining frontend seams.
- Added `tests/c/internal/positive_case/ok_expr_access_misc_runtime.c` as
  focused runtime coverage for member/index/ternary/sizeof lowering in the
  remaining `stmt_emitter_expr.cpp` helper family.
- Executed the sixteenth Step 4 slice by moving the remaining non-binary
  expression payload helpers (`UnaryExpr`, `AssignExpr`, `CastExpr`,
  `TernaryExpr`, `SizeofExpr`, `SizeofTypeExpr`, `LabelAddrExpr`,
  `PendingConstevalExpr`, `IndexExpr`, and `MemberExpr`) out of
  `src/codegen/lir/stmt_emitter_expr.cpp` into the new
  `src/codegen/lir/stmt_emitter_expr_misc.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `positive_sema_ok_expr_access_misc_runtime_c` and
  `positive_sema_ok_expr_unary_binary_runtime_c`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard
  passed with 3330/3330 tests passing before and 3334/3334 after, with no new
  failures.
- Recorded the sixteenth before/after extraction measurement: compiling the
  pre-split `src/codegen/lir/stmt_emitter_expr.cpp` from `HEAD` on the direct
  optimized command took 3.471s, the post-split
  `src/codegen/lir/stmt_emitter_expr.cpp` took 2.332s, and the new
  `src/codegen/lir/stmt_emitter_expr_misc.cpp` compiled in 2.839s.
- The latest full-suite rerun passes 3335/3335 tests, and the monotonic
  regression guard remains green.
- The seventeenth extraction slice reduced
  `src/codegen/lir/stmt_emitter_call.cpp` from 3.227s to 2.931s on the direct
  optimized compile command, so this call-argument helper split counts as a
  measured hotspot reduction for that TU.
- Added `tests/c/internal/positive_case/ok_call_variadic_aggregate_runtime.c`
  as focused runtime coverage for fixed by-value aggregate calls, default
  variadic promotions, and variadic aggregate lowering.
- Executed the seventeenth Step 4 slice by moving the generic call-argument
  preparation cluster (`callee_needs_va_list_by_value_copy`,
  `apply_default_arg_promotion`, `prepare_call_arg`,
  `prepare_amd64_variadic_aggregate_arg`, and `prepare_call_args`) out of
  `src/codegen/lir/stmt_emitter_call.cpp` into the new
  `src/codegen/lir/stmt_emitter_call_args.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `positive_sema_ok_call_variadic_aggregate_runtime_c`,
  `positive_sema_ok_fn_returns_variadic_fn_ptr_c`,
  `backend_lir_aarch64_variadic_pair_ir`, and
  `backend_runtime_variadic_pair_second`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard
  passed with 3330/3330 tests passing before and 3335/3335 after, with no new
  failures.
- Recorded the seventeenth before/after extraction measurement: compiling the
  pre-split `src/codegen/lir/stmt_emitter_call.cpp` from `HEAD` on the direct
  optimized command took 3.227s, the post-split
  `src/codegen/lir/stmt_emitter_call.cpp` took 2.931s, and the new
  `src/codegen/lir/stmt_emitter_call_args.cpp` compiled in 2.807s.
- Refreshed the optimized hotspot order after the call-argument split:
  `stmt_emitter_call.cpp` measured 3.136s, `hir_expr.cpp` 2.558s,
  `hir_templates.cpp` 2.483s, `hir_stmt.cpp` 2.415s, and
  `stmt_emitter_expr.cpp` 2.244s, so the next slice stayed in the remaining
  call-lowering family.
- Added `tests/c/internal/positive_case/ok_call_target_resolution_runtime.c`
  as focused runtime coverage for direct calls, function-pointer calls,
  builtin-alias call resolution, and unresolved external varargs calls.
- Executed the eighteenth Step 4 slice by moving the call-target resolution
  and final call-emission helpers (`resolve_call_target_info`,
  `emit_void_call`, and `emit_call_with_result`) out of
  `src/codegen/lir/stmt_emitter_call.cpp` into the new
  `src/codegen/lir/stmt_emitter_call_target.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `positive_sema_ok_call_target_resolution_runtime_c`,
  `positive_sema_ok_call_builtin_runtime_c`, and
  `positive_sema_ok_call_variadic_aggregate_runtime_c`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard
  passed with 3330/3330 tests passing before and 3336/3336 after, with no new
  failures.
- Recorded the eighteenth before/after extraction measurement: compiling the
  `HEAD` version of `src/codegen/lir/stmt_emitter_call.cpp` on the direct
  optimized command took 3.570s, the post-split working-tree
  `src/codegen/lir/stmt_emitter_call.cpp` took 3.490s, and the new
  `src/codegen/lir/stmt_emitter_call_target.cpp` compiled in 1.379s.
- The refreshed post-eighteenth-slice hotspot order is still led by
  `src/codegen/lir/stmt_emitter_call.cpp` at 3.216s, followed by
  `src/frontend/hir/hir_expr.cpp` at 2.629s,
  `src/frontend/hir/hir_templates.cpp` at 2.506s,
  `src/frontend/hir/hir_stmt.cpp` at 2.465s, and
  `src/codegen/lir/stmt_emitter_expr.cpp` at 2.367s.
- Because `stmt_emitter_call.cpp` still leads the measured tier and its
  remaining cohesive seam is the AMD64 `va_arg` helper family, the next
  highest-value slice should stay in `stmt_emitter_call.cpp` before returning
  to the frontend hotspots.
- Added focused x86_64 LLVM IR coverage in
  `backend_ir_x86_64_variadic_mixed_int_double_amd64_vaarg` to pin mixed
  integer/SSE AMD64 `va_arg` register reconstruction, offset updates, and the
  register/stack join.
- Executed the nineteenth Step 4 slice by moving
  `emit_amd64_va_arg_from_registers` out of
  `src/codegen/lir/stmt_emitter_call_vaarg_amd64.cpp` into the new
  `src/codegen/lir/stmt_emitter_call_vaarg_amd64_registers.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `backend_ir_x86_64_variadic_pair_amd64_vaarg`,
  `backend_ir_x86_64_variadic_mixed_int_double_amd64_vaarg`,
  `backend_runtime_variadic_double_bytes`,
  `backend_runtime_variadic_pair_second`,
  `abi_abi_variadic_forward_wrapper_c`,
  `abi_abi_variadic_struct_result_c`,
  `abi_abi_variadic_va_copy_accumulate_c`, and
  `positive_sema_ok_call_variadic_aggregate_runtime_c`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard
  passed with 3330/3330 tests passing before and 3338/3338 after, with no new
  failures.
- Recorded the nineteenth before/after extraction measurement: compiling the
  pre-split `src/codegen/lir/stmt_emitter_call_vaarg_amd64.cpp` from `HEAD`
  on the direct optimized command took 3.664s, the reduced
  `src/codegen/lir/stmt_emitter_call_vaarg_amd64.cpp` took 3.076s, and the
  new `src/codegen/lir/stmt_emitter_call_vaarg_amd64_registers.cpp` compiled
  in 2.707s.
- Refreshed the direct comparison against the remaining frontend leaders:
  `hir_expr.cpp` measured 3.277s, `hir_templates.cpp` 3.166s,
  `hir_stmt.cpp` 3.159s, and the reduced
  `stmt_emitter_call_vaarg_amd64.cpp` 3.076s.
- Tightened the focused HIR builtin-layout checks so
  `cpp_hir_builtin_layout_query_alignof_type` and
  `cpp_hir_builtin_layout_query_alignof_expr` now require the concrete
  `return 16` result in the dumped HIR.
- Executed the twentieth Step 4 slice by moving the builtin layout-query
  helper family (`LayoutQueries`, `builtin_query_result_type`,
  `resolve_builtin_query_type`, `lower_builtin_sizeof_type`,
  `lower_builtin_alignof_type`, `builtin_alignof_expr_bytes`, and
  `lower_builtin_alignof_expr`) out of `src/frontend/hir/hir_expr.cpp` into
  the new `src/frontend/hir/hir_expr_builtin.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_builtin_layout_query_sizeof_type`,
  `cpp_hir_builtin_layout_query_alignof_type`, and
  `cpp_hir_builtin_layout_query_alignof_expr`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard
  passed with 3330/3330 tests passing before and 3338/3338 after, with no new
  failures.
- Recorded the twentieth before/after extraction measurement: compiling the
  pre-split `src/frontend/hir/hir_expr.cpp` from `HEAD` on the direct
  optimized command took 3.148s, the reduced `src/frontend/hir/hir_expr.cpp`
  took 2.667s, and the new `src/frontend/hir/hir_expr_builtin.cpp` compiled
  in 1.060s.
- Refreshed the direct comparison against the remaining leaders:
  `hir_templates.cpp` remains at 3.166s, `hir_stmt.cpp` remains at 3.159s,
  the reduced `stmt_emitter_call_vaarg_amd64.cpp` remains at 3.076s, and the
  reduced `hir_expr.cpp` now measures 2.667s.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/template_deferred_nttp_cast_static_member_expr_hir.cpp`
  and wired the new `cpp_hir_template_deferred_nttp_cast_static_member_expr`
  test into `tests/cpp/internal/InternalTests.cmake`.
- Executed the twenty-first Step 4 slice by moving the deferred NTTP
  expression evaluator cluster (`parse_pack_binding_name`,
  `count_pack_bindings_for_name`, `DeferredNttpExprCursor`,
  `DeferredNttpExprEnv`, `DeferredNttpTemplateLookup`,
  `DeferredNttpExprParser`, and `eval_deferred_nttp_expr_hir`) out of
  `src/frontend/hir/hir_templates.cpp` into the new
  `src/frontend/hir/hir_templates_deferred_nttp.cpp`.
- Rebuilt after the split and re-ran focused coverage:
  `cpp_hir_template_deferred_nttp_expr`,
  `cpp_hir_template_deferred_nttp_arith_expr`,
  `cpp_hir_template_deferred_nttp_logic_expr`,
  `cpp_hir_template_deferred_nttp_static_member_expr`,
  `cpp_hir_template_deferred_nttp_cast_static_member_expr`, and
  `cpp_hir_template_deferred_nttp_sizeof_pack_expr`.
- Re-ran the full suite into `test_fail_after.log`; the regression guard
  passed with 3330/3330 tests passing before and 3339/3339 after, with no new
  failures.
- Recorded the twenty-first before/after extraction measurement: compiling the
  pre-split `src/frontend/hir/hir_templates.cpp` from `HEAD` on the direct
  optimized command took 3.524s, the reduced `src/frontend/hir/hir_templates.cpp`
  took 3.416s, and the new `src/frontend/hir/hir_templates_deferred_nttp.cpp`
  compiled in 2.361s.
- The twenty-first extraction slice reduced `src/frontend/hir/hir_templates.cpp`
  from 3.524s to 3.416s on the direct `HEAD` versus working-tree compile
  comparison, so this deferred-NTTP evaluator split counts as a small but
  measured hotspot reduction for that TU.
- Refreshed the direct comparison against the remaining leaders:
  `hir_templates.cpp` now measures 2.672s, `hir_expr.cpp` 2.402s,
  `hir_stmt.cpp` 2.330s, and the reduced
  `stmt_emitter_call_vaarg_amd64.cpp` 2.147s.
- The post-split handoff still keeps `src/frontend/hir/hir_templates.cpp` as
  the highest direct-TU hotspot, so the next extraction slice should remain in
  that file instead of switching back to `hir_expr.cpp` or `hir_stmt.cpp`.
