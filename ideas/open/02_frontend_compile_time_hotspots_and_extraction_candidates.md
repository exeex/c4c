# Frontend Compile-Time Hotspots And Extraction Candidates

Status: Open
Last Updated: 2026-04-10

## Goal

Investigate why frontend and frontend-adjacent translation units have become
noticeably slower to compile, identify the biggest compile-time hotspots, and
turn the findings into a prioritized list of extraction and structure changes
that can reduce optimized build cost without changing behavior.

## Why This Idea Exists

Recent local measurements suggest the slowdown is not explained by one issue
alone.

Representative observations from current frontend work:

- `src/frontend/hir/hir_expr.cpp` is only about 2.5k lines of source, but an
  optimized single-file compile still takes multiple seconds
- the file preprocesses to roughly 84k lines, so the compiler sees much more
  than the raw `.cpp` size suggests
- syntax-only parsing is materially cheaper than full optimized compilation,
  which suggests optimizer workload matters in addition to include cost
- the frontend contains multiple very large dispatcher-style functions in
  parser and HIR code, not just one isolated file
- several frontend headers are now large enough to amplify rebuild cost across
  many translation units

This makes it worth studying the problem directly instead of assuming the cause
is only includes, only templates, or only link time.

## Main Objective

Produce a frontend-focused compile-time map that answers:

1. which translation units are currently the most expensive to rebuild
2. how much of the cost is front-end parsing/includes versus optimization
3. which very large functions or helper-heavy headers are the strongest
   candidates for extraction
4. which fixes should be attempted first because they offer good compile-time
   benefit with low semantic risk

## Primary Scope

- `src/frontend/parser/*.cpp`
- `src/frontend/hir/*.cpp`
- `src/frontend/sema/*.cpp`
- `src/frontend/preprocessor/*.cpp`
- `src/frontend/lexer/*.cpp`
- `src/codegen/lir/*.cpp`
- frontend-owned headers that are widely included by those units
- build-system settings that materially change frontend compile cost

## Candidate Investigation Tasks

1. Measure representative single-TU compile times for the largest frontend and
   frontend-adjacent `.cpp` files under the common optimized configuration.
2. Compare `-fsyntax-only`, `-O0`, and optimized `-c` builds on hotspot files
   to separate parse/include cost from optimizer cost.
3. Use compiler timing output such as `-ftime-report` or equivalent to locate
   especially expensive optimization passes or template-instantiation work.
4. Record preprocess size and include fan-in for hotspot files to spot headers
   that magnify rebuild cost across many translation units.
5. Identify giant dispatcher functions that should become thin coordinators
   plus helper subflows.
6. Identify heavy helper headers whose larger implementations should move into
   `.cpp` files or narrower helper boundaries.
7. Distinguish translation-unit compile cost from final link cost so fixes are
   targeted accurately.

## Expected Hotspot Candidates

Based on current inspection, likely first-pass candidates include:

- `src/frontend/hir/hir_expr.cpp`
- `src/frontend/hir/hir_stmt.cpp`
- `src/frontend/hir/hir_templates.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_expressions.cpp`
- `src/frontend/parser/parser_statements.cpp`
- `src/codegen/lir/stmt_emitter_expr.cpp`
- `src/codegen/lir/stmt_emitter_call.cpp`

Likely high-impact headers to inspect include:

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/types_helpers.hpp`
- `src/frontend/hir/hir_lowerer_internal.hpp`
- `src/frontend/hir/compile_time_engine.hpp`
- `src/frontend/hir/hir_ir.hpp`

## Extraction Direction

The preferred direction is not blanket micro-optimization. The preferred
direction is to find stable semantic seams and extract around them.

Examples:

- turn giant parser functions into dispatch plus named parsing helpers for
  specific syntactic families
- turn giant HIR lowering functions into dispatch plus helpers for var/member/
  call/template/operator lowering paths
- centralize repeated temporary-materialization and ref/rvalue-ref lowering
  logic
- move heavy non-trivial helper implementations out of large shared headers
  when a `.cpp` boundary would reduce rebuild cost

## Constraints

- preserve existing frontend behavior and diagnostics
- do not mix this investigation with backend-only refactors
- do not assume compile-time wins unless they are measured
- avoid broad container rewrites as a first response
- prefer extraction plans that can be landed incrementally and validated with
  existing tests

## Validation

At minimum:

- record before/after compile-time measurements for the hotspot units touched
- ensure no new frontend test regressions
- ensure extracted helpers preserve parser, HIR, and LIR behavior on existing
  regression coverage

## 2026-04-10 Initial Inventory Notes

First pass measurements were captured from `build/compile_commands.json` using
the generated optimized single-TU compile commands, along with matching `-E`
and `-fsyntax-only -H` variants for preprocess size and include-tree counts.

Initial ranking:

1. `src/codegen/lir/stmt_emitter_expr.cpp` - 6.578s, 85882 preprocessed lines,
   382 include entries
2. `src/frontend/hir/hir_expr.cpp` - 5.226s, 84245 preprocessed lines,
   362 include entries
3. `src/frontend/hir/hir_stmt.cpp` - 4.998s, 83946 preprocessed lines,
   345 include entries
4. `src/frontend/hir/hir_templates.cpp` - 4.886s, 85265 preprocessed lines,
   360 include entries
5. `src/codegen/lir/stmt_emitter_call.cpp` - 3.939s, 85628 preprocessed lines,
   384 include entries
6. `src/frontend/parser/parser_declarations.cpp` - 2.957s, 65266
   preprocessed lines, 310 include entries
7. `src/frontend/parser/parser_types_base.cpp` - 2.696s, 67811 preprocessed
   lines, 317 include entries
8. `src/frontend/parser/parser_expressions.cpp` - 1.205s, 55439 preprocessed
   lines, 252 include entries
9. `src/frontend/parser/parser_statements.cpp` - 0.872s, 54420 preprocessed
   lines, 249 include entries

Current interpretation:

- the first-pass hotspot tier is concentrated in HIR and frontend-adjacent LIR
  rather than the parser
- the hottest five units all preprocess to roughly 84k to 86k lines, which is
  consistent with substantial include amplification
- Step 2 should classify the top HIR/LIR units first, then come back to parser
  files once the parse-versus-optimizer split is clearer

## 2026-04-10 Step 2 Parse Versus Optimization Split

The current top-five hotspot tier was re-measured with three compile modes per
TU using the generated commands from `build/compile_commands.json`:
`-fsyntax-only`, `-O0 -c`, and the existing optimized `-O2 -c` command.

Results:

| Translation unit | `-fsyntax-only` (s) | `-O0 -c` (s) | `-O2 -c` (s) | Classification |
| --- | ---: | ---: | ---: | --- |
| `src/codegen/lir/stmt_emitter_expr.cpp` | 1.170 | 2.250 | 6.712 | Optimizer heavy |
| `src/frontend/hir/hir_expr.cpp` | 0.685 | 1.793 | 4.899 | Optimizer heavy |
| `src/frontend/hir/hir_stmt.cpp` | 0.799 | 1.959 | 5.098 | Optimizer heavy |
| `src/frontend/hir/hir_templates.cpp` | 0.790 | 1.676 | 5.031 | Optimizer heavy |
| `src/codegen/lir/stmt_emitter_call.cpp` | 0.744 | 1.781 | 4.246 | Optimizer heavy |

Interpretation:

- none of the current top-five units are parse/include heavy in the narrow
  Step 2 sense; all five are dominated by the optimized delta above `-O0`
- the parse/include floor is still meaningful at roughly `0.7s` to `1.2s`,
  which keeps header amplification relevant but secondary for the first
  extraction choice
- sampled `-ftime-report` output shows optimizer/codegen dominating the worst
  units: `stmt_emitter_expr.cpp` spends `5.79s` of `7.54s` in `phase opt and
  generate`, and `hir_templates.cpp` spends `3.90s` of `5.40s` there
- `callgraph functions expansion` is a major contributor in both sampled cases,
  which suggests large dispatcher/helper bodies are likely better first seams
  than purely include-focused cleanup

Current Step 3 direction:

- prioritize extraction candidates inside the optimizer-heavy HIR/LIR units,
  especially `stmt_emitter_expr.cpp` and `hir_templates.cpp`
- treat header cleanup as a supporting follow-up unless a concrete seam clearly
  reduces both frontend and optimizer cost

## 2026-04-10 Step 3 Seam Ranking

The first prioritized extraction seams after inspecting the hottest optimizer-
heavy units are:

1. `src/codegen/lir/stmt_emitter_expr.cpp`: move the AArch64 `va_arg` lowering
   cluster into its own implementation file. The cluster is already declared in
   `stmt_emitter.hpp`, ABI-local, and covered by existing AArch64/runtime
   variadic tests, which makes it the lowest-risk out-of-line split.
2. `src/codegen/lir/stmt_emitter_expr.cpp`: split the remaining unary/binary
   expression lowering dispatchers into narrower helpers, especially the
   complex arithmetic/comparison branches and cast-heavy paths.
3. `src/frontend/hir/hir_templates.cpp`: extract the member-typedef and
   deferred-template-resolution cluster around
   `resolve_struct_member_typedef_type` into narrower helper entry points.

## 2026-04-10 Step 4 First Extraction Slice

Executed the first low-risk slice from that ranking:

- moved the AArch64 `va_arg` lowering cluster
  (`emit_aarch64_vaarg_gp_src_ptr`, `emit_aarch64_vaarg_fp_src_ptr`, and
  `emit_rval_payload(..., const VaArgExpr&, ...)`) out of
  `src/codegen/lir/stmt_emitter_expr.cpp` into the new
  `src/codegen/lir/stmt_emitter_vaarg.cpp`
- added the new translation unit to CMake and kept the logic as existing
  `StmtEmitter` methods, so the slice changes file ownership rather than
  behavior
- validated the move with focused variadic tests plus a full-suite rerun

Measured result:

- `src/codegen/lir/stmt_emitter_expr.cpp` improved from `6.712s` to `5.493s`
  on the optimized single-TU compile command (`-1.219s`, about `18%`)
- the new `src/codegen/lir/stmt_emitter_vaarg.cpp` compiles in `2.799s`

Validation result:

- targeted tests passed:
  `backend_lir_aarch64_variadic_*`,
  `backend_runtime_variadic_*`,
  `abi_abi_variadic_*`, and the `llvm_gcc_c_torture` `stdarg`/`va_arg` subset
- full-suite regression guard passed with `3319/3319` tests passing before and
  after, with no new failures

## 2026-04-10 Step 4 Second Extraction Slice

Executed the next low-risk seam inside the remaining
`stmt_emitter_expr.cpp` unary/binary cluster:

- moved the binary-expression lowering cluster
  (`emit_complex_binary_arith`, `emit_rval_payload(..., BinaryExpr, ...)`, and
  `emit_logical`) out of `src/codegen/lir/stmt_emitter_expr.cpp` into the new
  `src/codegen/lir/stmt_emitter_expr_binary.cpp`
- kept the logic as existing `StmtEmitter` methods so the slice remains a file
  ownership split rather than a semantic rewrite
- added `tests/c/internal/positive_case/ok_expr_unary_binary_runtime.c` as a
  focused runtime check for scalar unary/binary, pointer arithmetic, logical
  short-circuiting, compound assignment, and simple complex arithmetic

Measured result:

- `src/codegen/lir/stmt_emitter_expr.cpp` improved from `6.021s` to `3.401s`
  on the optimized single-TU compile command used for this iteration
  (`-2.620s`, about `43.5%`)
- the new `src/codegen/lir/stmt_emitter_expr_binary.cpp` compiles in `2.679s`

Validation result:

- focused coverage passed:
  `positive_sema_ok_expr_unary_binary_runtime_c`,
  `positive_sema_ok_expr_canonical_cast_index_c`,
  `positive_sema_ok_expr_canonical_types_c`,
  `smoke_aggregate_access.c`, and `smoke_call_lowering.c` in compare mode
- full-suite regression guard passed with `3320/3320` tests passing after the
  new focused test was added, with no new failures

Current follow-on:

- refresh the hotspot inventory before the next slice because
  `stmt_emitter_expr.cpp` may no longer be the lead hotspot after the two
  extractions
- if `hir_templates.cpp` stays near the top, move the next extraction slice to
  the member-typedef/deferred-template-resolution helper cluster

## Non-Goals

- no backend architecture work
- no speculative rewrite of the entire frontend around a new IR or parser model
- no unmeasured "performance cleanup" bundle that mixes unrelated changes
