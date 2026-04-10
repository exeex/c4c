# Frontend Compile-Time Hotspots And Extraction Candidates

Status: Active
Source Idea: ideas/open/02_frontend_compile_time_hotspots_and_extraction_candidates.md
Source Plan: plan.md

## Active Item

- Step 4: Choose the next hottest HIR seam after the `hir_expr.cpp`
  call-lowering split, with `hir_stmt.cpp` and `hir_templates.cpp` now leading
  the refreshed tier.

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

## Next Slice

- Start from the refreshed post-split tier, which now has
  `hir_stmt.cpp` at 4.710s and `hir_templates.cpp` at 4.634s ahead of the
  remaining HIR/LIR candidates.
- Prefer a low-risk `hir_stmt.cpp` helper extraction if it has a focused test
  surface comparable to the `hir_expr.cpp` call-lowering split; otherwise
  revisit the next cohesive `hir_templates.cpp` seam.

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
  3322/3322 tests, and the monotonic regression guard remains green.
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
