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

## 2026-04-10 Latest Step 4 Follow-On

After the `hir_templates.cpp` struct-instantiation split, a refreshed targeted
ranking still put `src/frontend/hir/hir_templates.cpp` first at `4.237s`, just
ahead of `src/codegen/lir/stmt_emitter_call.cpp` at `3.996s` and
`src/frontend/hir/hir_stmt.cpp` at `3.901s`.

That kept the next extraction inside `hir_templates.cpp`:

- moved `resolve_struct_member_typedef_type` and
  `resolve_struct_member_typedef_if_ready` into the new
  `src/frontend/hir/hir_templates_type_resolution.cpp`
- added focused HIR coverage in
  `tests/cpp/internal/hir_case/template_inherited_member_typedef_trait_hir.cpp`
  for inherited trait-style `::type` lookup through a realized base struct
- measured `src/frontend/hir/hir_templates.cpp` at `4.283s` from `HEAD`
  before the split and `4.021s` after the split, while the new
  `src/frontend/hir/hir_templates_type_resolution.cpp` compiles in `1.581s`

Current follow-on direction:

- `hir_templates.cpp` still leads the refreshed tier at `4.247s`, so one more
  cohesive helper family there is still justified before shifting back to
  `hir_stmt.cpp`
- the next seam should target the template value-argument and static-member-
  const path (`assign_template_arg_refs_from_ast_args`,
  `resolve_ast_template_value_arg`,
  `try_eval_template_static_member_const`, and
  `try_eval_instantiated_struct_static_member_const`) unless inspection shows
  that `stmt_emitter_call.cpp` has become the lower-risk next slice

## 2026-04-10 Step 4 Ninth Extraction Slice

The refreshed gap between `hir_templates.cpp` and `stmt_emitter_call.cpp` had
become small enough that the next slice favored the lower-risk LIR seam.

Executed slice:

- moved the builtin-call helper family
  (`prepare_builtin_int_arg`, `narrow_builtin_int_result`,
  `emit_builtin_*`, `promote_builtin_*`, `emit_post_builtin_call`, and the
  final builtin dispatch in `emit_rval_payload`) out of
  `src/codegen/lir/stmt_emitter_call.cpp` into the new
  `src/codegen/lir/stmt_emitter_call_builtin.cpp`
- added focused runtime coverage in
  `tests/c/internal/positive_case/ok_call_builtin_runtime.c`
- re-ran `tests/c/internal/compare_case/smoke_call_lowering.c` in compare mode
  to keep the broader call-lowering behavior pinned

Measurements:

- compiling the pre-split `src/codegen/lir/stmt_emitter_call.cpp` from
  `HEAD^` on the generated optimized command took `4.463s`
- the post-split `src/codegen/lir/stmt_emitter_call.cpp` took `4.007s`
- the new `src/codegen/lir/stmt_emitter_call_builtin.cpp` compiled in `2.718s`

Interpretation:

- this slice produced another measured reduction on a current hotspot TU while
  keeping the call-lowering behavior stable
- because the active LIR call hotspot dropped after the split, the next Step 4
  choice should refresh the current ranking and likely shift back to the HIR
  tier (`hir_templates.cpp` versus `hir_stmt.cpp`) rather than forcing another
  immediate `stmt_emitter_call.cpp` extraction

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

## 2026-04-10 Refreshed Hotspot Ranking After The LIR Splits

The optimized single-TU rerun used to pick the next slice now ranks the top
hotspots as:

1. `src/frontend/hir/hir_expr.cpp` - 4.933s
2. `src/frontend/hir/hir_templates.cpp` - 4.295s
3. `src/frontend/hir/hir_stmt.cpp` - 4.275s
4. `src/codegen/lir/stmt_emitter_call.cpp` - 3.547s
5. `src/codegen/lir/stmt_emitter_expr.cpp` - 2.986s

This moved the active extraction focus from the LIR expression path to the HIR
template tier.

## 2026-04-10 Step 4 Third Extraction Slice

Executed the `hir_templates.cpp` follow-on helper split:

- moved the deferred template-owner/member-typedef helper cluster
  (`require_pending_template_type_primary`,
  `resolve_pending_template_owner_if_ready`,
  `spawn_pending_template_owner_work`,
  `ensure_pending_template_owner_ready`,
  `resolve_deferred_member_typedef_type`,
  `seed_template_type_dependency_if_needed`, and
  `seed_and_resolve_pending_template_type_if_needed`) into the new
  `src/frontend/hir/hir_templates_member_typedef.cpp`
- kept the logic as existing `Lowerer` methods, so the slice remains a
  translation-unit ownership split rather than a semantic rewrite
- added `tests/cpp/internal/hir_case/template_member_owner_signature_local_hir.cpp`
  as focused HIR coverage for deferred member-typedef resolution across a
  function signature plus an instantiated local

Measured result:

- `src/frontend/hir/hir_templates.cpp` was `4.295s` before the split and
  measured between `4.342s` and `4.527s` across three post-split reruns
- the new `src/frontend/hir/hir_templates_member_typedef.cpp` compiles between
  `1.443s` and `1.487s` across the same reruns
- this means the slice did not demonstrate a single-TU compile-time win, so it
  should be treated as structural preparation for later HIR extractions rather
  than as a measured performance improvement

Validation result:

- focused coverage passed:
  `cpp_hir_template_member_owner_chain`,
  `cpp_hir_template_member_owner_field_and_local`,
  `cpp_hir_template_member_owner_signature_local`,
  `cpp_positive_sema_template_alias_member_typedef_dependent_ref_runtime_cpp`,
  `cpp_positive_sema_template_alias_member_typedef_reordered_owner_runtime_cpp`,
  `cpp_positive_sema_template_member_owner_resolution_cpp`, and
  `cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`
- full-suite regression guard passed with `3319/3319` tests passing before and
  `3321/3321` after, with no new failures

## 2026-04-10 Refreshed Hotspot Ranking Before The `hir_expr.cpp` Split

After recreating `build/compile_commands.json` and rebuilding the tree, the
optimized single-TU rerun ranked the active hotspot tier as:

1. `src/frontend/hir/hir_expr.cpp` - 9.281s
2. `src/frontend/hir/hir_stmt.cpp` - 8.841s
3. `src/frontend/hir/hir_templates.cpp` - 8.780s
4. `src/codegen/lir/stmt_emitter_call.cpp` - 6.327s
5. `src/codegen/lir/stmt_emitter_expr.cpp` - 3.770s

This moved the next extraction target to `hir_expr.cpp`, with the call-lowering
cluster chosen as the lowest-risk seam that still had focused HIR/runtime
coverage.

## 2026-04-10 Step 4 Fourth Extraction Slice

Executed the `hir_expr.cpp` call-lowering split:

- moved the call-lowering helper cluster
  (`try_lower_template_struct_call`, `lower_call_arg`,
  `try_expand_pack_call_arg`, `try_lower_member_call_expr`,
  `lower_call_expr`, and `try_lower_consteval_call_expr`) into the new
  `src/frontend/hir/hir_expr_call.cpp`
- kept the logic as existing `Lowerer` methods so the slice remains a
  translation-unit ownership split rather than a semantic rewrite
- added `tests/cpp/internal/hir_case/hir_expr_call_member_helper_hir.cpp` as
  focused HIR coverage for constructor, member-call, and ref-qualified
  operator-call lowering

Measured result:

- `src/frontend/hir/hir_expr.cpp` improved from `9.281s` to `3.549s` on the
  optimized single-TU compile command (`-5.732s`, about `61.8%`)
- the new `src/frontend/hir/hir_expr_call.cpp` compiles in `2.884s`
- the refreshed comparison tier now reads `hir_stmt.cpp` at `4.710s` and
  `hir_templates.cpp` at `4.634s`, so the next hotspot focus should likely
  move to one of those files

Validation result:

- focused coverage passed:
  `cpp_positive_sema_hir_expr_call_member_helper_runtime_cpp`,
  `cpp_positive_sema_operator_call_rvalue_ref_runtime_cpp`,
  `cpp_positive_sema_template_operator_call_rvalue_ref_runtime_cpp`, and
  `cpp_hir_expr_call_member_helper`
- full-suite regression guard passed with `3321/3321` tests passing before and
  `3322/3322` after, with no new failures

## 2026-04-10 Step 4 Fifth Extraction Slice

Executed the `hir_stmt.cpp` range-for split:

- moved the `NK_RANGE_FOR` lowering branch into the new
  `src/frontend/hir/hir_stmt_range_for.cpp`
- kept the logic as existing `Lowerer` methods so the slice remains a
  translation-unit ownership split rather than a semantic rewrite
- added `tests/cpp/internal/hir_case/hir_stmt_range_for_helper_hir.cpp` as
  focused HIR coverage for the synthesized iterator locals, iterator method
  calls, and loop-carried element binding shape

Measured result:

- compiling the pre-split `src/frontend/hir/hir_stmt.cpp` from `HEAD` on the
  generated optimized command took `5.231s`
- the post-split `src/frontend/hir/hir_stmt.cpp` took `10.111s`
- the new `src/frontend/hir/hir_stmt_range_for.cpp` compiles in `1.958s`
- this means the slice did not demonstrate a compile-time win, so it should be
  treated as structure preparation rather than as a measured performance
  improvement

Validation result:

- focused coverage passed:
  `cpp_hir_stmt_range_for_helper` and
  `cpp_positive_sema_range_for_const_cpp`
- full-suite regression guard passed with `3321/3321` tests passing before and
  `3323/3323` after, with no new failures

## 2026-04-10 Step 4 Sixth Extraction Slice

Executed the `hir_templates.cpp` struct-instantiation split:

- moved the template-struct instantiation/body helper cluster
  (`apply_template_typedef_bindings`, `materialize_template_array_extent`,
  `append_instantiated_template_struct_bases`,
  `register_instantiated_template_struct_methods`,
  `record_instantiated_template_struct_field_metadata`,
  `instantiate_template_struct_field`,
  `append_instantiated_template_struct_fields`, and
  `instantiate_template_struct_body`) into the new
  `src/frontend/hir/hir_templates_struct_instantiation.cpp`
- kept the logic as existing `Lowerer` methods so the slice remains a
  translation-unit ownership split rather than a semantic rewrite
- added `tests/cpp/internal/hir_case/template_struct_body_instantiation_hir.cpp`
  as focused HIR coverage for inherited member access, NTTP array extent
  materialization, and instantiated method lowering

Measured result:

- compiling the pre-split `src/frontend/hir/hir_templates.cpp` from `HEAD` on
  the generated optimized command took `5.087s`
- the post-split `src/frontend/hir/hir_templates.cpp` took `4.241s`
- the new `src/frontend/hir/hir_templates_struct_instantiation.cpp` compiles
  in `1.384s`
- this means the slice did demonstrate a single-TU compile-time win for the
  main hotspot TU, reducing `hir_templates.cpp` by `0.846s` (about `16.6%`)

Validation result:

- focused coverage passed:
  `cpp_hir_template_struct_body_instantiation`,
  `cpp_hir_template_struct_inherited_method_binding`,
  `cpp_hir_template_struct_field_array_extent`,
  `cpp_hir_template_method_param_reentrant_lowering`,
  `cpp_positive_sema_template_member_owner_resolution_cpp`, and
  `cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`
- full-suite regression guard passed with `3321/3321` tests passing before and
  `3324/3324` after, with no new failures

## 2026-04-10 Step 4 Tenth Extraction Slice

Executed the `hir_stmt.cpp` switch-family split:

- moved the `NK_SWITCH`, `NK_CASE`, `NK_CASE_RANGE`, and `NK_DEFAULT`
  lowering family into the new `src/frontend/hir/hir_stmt_switch.cpp`
- kept the logic as existing `Lowerer` methods so the slice remains a
  translation-unit ownership split rather than a semantic rewrite
- added `tests/cpp/internal/hir_case/hir_stmt_switch_helper_hir.cpp` as
  focused HIR coverage for the emitted switch body block, constexpr-backed
  case labels, and default block shape

Measured result:

- compiling the pre-split `src/frontend/hir/hir_stmt.cpp` from `HEAD` on the
  generated optimized command took `4.914s`
- the direct post-split `src/frontend/hir/hir_stmt.cpp` rerun took `3.952s`
- the new `src/frontend/hir/hir_stmt_switch.cpp` compiles in `1.074s`
- this means the slice did demonstrate a single-TU compile-time win for the
  main hotspot TU, reducing `hir_stmt.cpp` by `0.962s` (about `19.6%`)

Validation result:

- focused coverage passed:
  `cpp_hir_stmt_switch_helper`,
  `cpp_hir_stmt_range_for_helper`,
  `cpp_positive_sema_constexpr_local_switch_cpp`,
  `positive_sema_ok_enum_scope_no_leak_after_block_c`, and
  `negative_tests_bad_flow_continue_in_switch`
- full-suite regression guard passed with `3325/3325` tests passing before and
  `3328/3328` after, with no new failures

Follow-on note:

- a refreshed hotspot rerun still leaves `src/frontend/hir/hir_stmt.cpp`
  narrowly ahead at `4.764s`, with `src/frontend/hir/hir_templates.cpp`
  next at `3.945s`, so the next iteration should confirm whether another
  cohesive `hir_stmt.cpp` control-flow seam remains before returning to
  template-heavy work

## 2026-04-10 Step 4 Thirteenth Extraction Slice

The refreshed direct hotspot comparison after the later `hir_templates.cpp`
work no longer matched that older follow-on note: `hir_stmt.cpp` measured
`7.659s`, `hir_templates.cpp` `6.379s`, `hir_expr.cpp` `4.406s`, and
`stmt_emitter_call.cpp` `3.181s` on the generated optimized compile commands.

That moved the next extraction back to `hir_stmt.cpp`, with the
local-declaration lowering body chosen as the next cohesive seam.

Executed the `hir_stmt.cpp` local-declaration split:

- moved `Lowerer::lower_local_decl_stmt` into the new
  `src/frontend/hir/hir_stmt_decl.cpp`
- kept the logic as an existing `Lowerer` method so the slice remains a
  translation-unit ownership split rather than a semantic rewrite
- added `tests/cpp/internal/hir_case/hir_stmt_local_decl_helper_hir.cpp` as
  focused HIR coverage for default construction, copy initialization, scalar
  array initialization, and rvalue-reference temporary materialization

Measured result:

- compiling the pre-split `src/frontend/hir/hir_stmt.cpp` from `HEAD` on the
  generated optimized command took `3.725s`
- the direct post-split `src/frontend/hir/hir_stmt.cpp` rerun took `2.540s`
- the new `src/frontend/hir/hir_stmt_decl.cpp` compiles in `2.520s`
- this means the slice did demonstrate a single-TU compile-time win for the
  main hotspot TU, reducing `hir_stmt.cpp` by `1.185s` (about `31.8%`)

Validation result:

- focused coverage passed:
  `cpp_hir_stmt_local_decl_helper`,
  `cpp_positive_sema_default_ctor_basic_cpp`,
  `cpp_positive_sema_copy_init_basic_cpp`,
  `cpp_positive_sema_copy_move_init_basic_cpp`,
  `cpp_positive_sema_rvalue_ref_decl_basic_cpp`, and
  `cpp_positive_sema_ctor_init_member_typedef_ctor_runtime_cpp`
- full-suite regression guard passed with `3330/3330` tests passing before and
  `3331/3331` after, with no new failures

Follow-on note:

- a refreshed hotspot rerun now leaves `src/frontend/hir/hir_expr.cpp`
  narrowly ahead at `3.653s`, with `src/frontend/hir/hir_templates.cpp`
  next at `3.579s`, so the next iteration should inspect `hir_expr.cpp`
  before returning to the remaining template-heavy seams

## 2026-04-10 Step 4 Fourteenth Extraction Slice

The refreshed hotspot order after the local-declaration split left
`hir_expr.cpp` narrowly ahead of `hir_templates.cpp`, so the next extraction
stayed in `hir_expr.cpp`.

Executed the object-materialization split:

- moved `hoist_compound_literal_to_global`,
  `try_lower_direct_struct_constructor_call`,
  `describe_initializer_list_struct`,
  `materialize_initializer_list_arg`,
  `lower_compound_literal_expr`,
  `lower_new_expr`, and `lower_delete_expr` into the new
  `src/frontend/hir/hir_expr_object.cpp`
- kept the logic as existing `Lowerer` methods so the slice remains a
  translation-unit ownership split rather than a semantic rewrite
- added `tests/cpp/internal/hir_case/hir_expr_object_materialization_helper_hir.cpp`
  as focused HIR coverage for direct constructor temps, initializer-list
  materialization, and `new`/`delete` lowering

Measured result:

- compiling the pre-split `src/frontend/hir/hir_expr.cpp` from `HEAD` on the
  generated optimized command took `4.087s`
- the direct post-split `src/frontend/hir/hir_expr.cpp` rerun took `2.639s`
- the new `src/frontend/hir/hir_expr_object.cpp` compiles in `2.179s`
- this means the slice did demonstrate a single-TU compile-time win for the
  main hotspot TU, reducing `hir_expr.cpp` by `1.448s` (about `35.4%`)

Validation result:

- focused coverage passed:
  `cpp_hir_expr_object_materialization_helper`,
  `cpp_hir_expr_call_member_helper`,
  `cpp_hir_builtin_layout_query_sizeof_type`,
  `cpp_hir_builtin_layout_query_alignof_type`,
  `cpp_hir_builtin_layout_query_alignof_expr`,
  `cpp_llvm_initializer_list_runtime_materialization`,
  `cpp_positive_sema_new_delete_side_effect_runtime_cpp`, and
  `cpp_positive_sema_class_specific_new_delete_side_effect_runtime_cpp`
- full-suite regression guard passed with `3330/3330` tests passing before and
  `3332/3332` after, with no new failures

Follow-on note:

- a refreshed hotspot rerun now leaves `src/frontend/hir/hir_templates.cpp`
  ahead at `3.522s`, with `src/codegen/lir/stmt_emitter_call.cpp` next at
  `3.036s`, so the next iteration should move back to the remaining
  `hir_templates.cpp` deduction/materialization seams before returning to
  LIR follow-up work

## 2026-04-10 Step 4 Fifteenth Extraction Slice

The refreshed hotspot order after the object-materialization split left
`hir_templates.cpp` back in the lead, so the next extraction stayed in the
remaining template-struct materialization cluster.

Executed the template-struct materialization split:

- moved `materialize_template_args`,
  `prepare_template_struct_instance`,
  `build_template_mangled_name`, and the private
  `HirTemplateArgMaterializer` family into the new
  `src/frontend/hir/hir_templates_materialization.cpp`
- kept the logic as existing `Lowerer` methods plus TU-local helpers, so the
  slice remains a translation-unit ownership split rather than a semantic
  rewrite
- added `tests/cpp/internal/hir_case/template_struct_arg_materialization_hir.cpp`
  as focused HIR coverage for defaulted type/value template arguments and
  explicit value-driven template-struct materialization

Measured result:

- compiling the pre-split `src/frontend/hir/hir_templates.cpp` from `HEAD^`
  on the generated optimized command took `4.145s`
- the direct post-split `src/frontend/hir/hir_templates.cpp` rerun took
  `3.250s`
- a refreshed hotspot sweep measured the reduced `hir_templates.cpp` at
  `2.953s`
- the new `src/frontend/hir/hir_templates_materialization.cpp` compiles in
  `2.786s`
- this means the slice did demonstrate a single-TU compile-time win for the
  main hotspot TU, reducing `hir_templates.cpp` by `0.895s` (about `21.6%`)
  on the direct comparison

Validation result:

- focused coverage passed:
  `cpp_hir_template_struct_arg_materialization`,
  `cpp_hir_template_struct_body_instantiation`,
  `cpp_hir_template_value_arg_static_member_trait`, and
  `cpp_hir_template_global_specialization`
- full-suite regression guard passed with `3330/3330` tests passing before and
  `3333/3333` after, with no new failures

Follow-on note:

- a refreshed hotspot rerun now leaves `src/codegen/lir/stmt_emitter_expr.cpp`
  ahead at `3.435s`, with `src/codegen/lir/stmt_emitter_call.cpp` next at
  `3.106s` and the reduced `src/frontend/hir/hir_templates.cpp` at `2.953s`,
  so the next iteration should shift back to LIR helper extraction before
  returning to the remaining frontend seams

## 2026-04-10 Step 4 Seventeenth Extraction Slice

The post-sixteenth hotspot order still left `stmt_emitter_call.cpp` in the
remaining lead pack, so this slice targeted the generic call-argument
preparation family instead of moving back into HIR.

Executed the call-argument preparation split:

- moved `callee_needs_va_list_by_value_copy`,
  `apply_default_arg_promotion`, `prepare_call_arg`,
  `prepare_amd64_variadic_aggregate_arg`, and `prepare_call_args` into the
  new `src/codegen/lir/stmt_emitter_call_args.cpp`
- kept `stmt_emitter_call.cpp` focused on AMD64 `va_arg`, call-target
  resolution, and final call emission
- added `tests/c/internal/positive_case/ok_call_variadic_aggregate_runtime.c`
  as focused runtime coverage for fixed by-value aggregate calls, default
  variadic promotions, and variadic aggregate lowering

Measured result:

- compiling the pre-split `src/codegen/lir/stmt_emitter_call.cpp` from `HEAD`
  on the direct optimized command took `3.227s`
- the direct post-split `src/codegen/lir/stmt_emitter_call.cpp` rerun took
  `2.931s`
- the new `src/codegen/lir/stmt_emitter_call_args.cpp` compiles in `2.807s`
- this means the slice did demonstrate a single-TU compile-time win for the
  main hotspot TU, reducing `stmt_emitter_call.cpp` by `0.296s` (about
  `9.2%`) on the direct comparison

Validation result:

- focused coverage passed:
  `positive_sema_ok_call_variadic_aggregate_runtime_c`,
  `positive_sema_ok_fn_returns_variadic_fn_ptr_c`,
  `backend_lir_aarch64_variadic_pair_ir`, and
  `backend_runtime_variadic_pair_second`
- full-suite regression guard passed with `3330/3330` tests passing before and
  `3335/3335` after, with no new failures

Follow-on note:

- rerun the refreshed optimized hotspot order before choosing the next slice;
  if `stmt_emitter_call.cpp` still leads, the remaining call-target
  resolution or AMD64 `va_arg` seam is the next obvious LIR candidate,
  otherwise the plan should move back to the measured top frontend TU

## Non-Goals

- no backend architecture work
- no speculative rewrite of the entire frontend around a new IR or parser model
- no unmeasured "performance cleanup" bundle that mixes unrelated changes
