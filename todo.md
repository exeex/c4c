# Frontend Compile-Time Hotspots And Extraction Candidates

Status: Active
Source Idea: ideas/open/02_frontend_compile_time_hotspots_and_extraction_candidates.md
Source Plan: plan.md

## Active Item

- Step 4: Re-measure the remaining hotspot order after the binary-expression
  split and choose between a follow-on unary extraction in
  `stmt_emitter_expr.cpp` and the `hir_templates.cpp` helper split.

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

## Next Slice

- Re-rank the current hotspots now that `stmt_emitter_expr.cpp` has dropped to
  3.401s and likely no longer leads the table.
- If `hir_templates.cpp` remains in the top hotspot tier, switch the next Step
  4 slice to the member-typedef/deferred-template-resolution extraction.
- Otherwise inspect the remaining unary-only cluster in
  `stmt_emitter_expr.cpp` for another low-risk out-of-line split.

## Blockers

- None recorded.

## Notes

- Follow numeric ordering from `ideas/open/`; the active source idea is `02`.
- Step 1 is complete for the initial target set.
- The original hotspot inventory is stale after two `stmt_emitter_expr.cpp`
  extractions and should be refreshed before selecting the next slice.
- The first-pass top five hotspot units all preprocess to roughly 84k to 86k
  lines and have 345 to 384 include-tree entries.
- Step 2 is complete: the top-five hotspot tier is optimizer heavy rather than
  parse-heavy, though all five keep a meaningful `-fsyntax-only` floor.
- The latest `ctest --test-dir build -j --output-on-failure` rerun passes
  3320/3320 tests, and the monotonic regression guard remains green.
- The first executed extraction slice reduced the hottest TU,
  `src/codegen/lir/stmt_emitter_expr.cpp`, by 1.219s on the optimized
  single-TU compile command.
- The second extraction slice reduced `src/codegen/lir/stmt_emitter_expr.cpp`
  by another 2.620s versus the immediate pre-split baseline used this
  iteration.
