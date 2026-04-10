# Frontend Compile-Time Hotspot Inventory

Date: 2026-04-10
Build: `build` (`RelWithDebInfo`, optimized single-TU compile commands from
`build/compile_commands.json`)

## Method

- Reconfigured `build` with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` so timings use
  the exact CMake-generated compile command for each TU.
- Timed the optimized `-O2 -g -DNDEBUG -c` compile for each candidate TU.
- Counted preprocessed output lines with the same compile command switched to
  `-E`.
- Counted include-tree entries with the same compile command switched to
  `-fsyntax-only -H`.

## Initial Ranking

| Rank | Translation unit | Optimized compile (s) | Preprocessed lines | Include entries |
| --- | --- | ---: | ---: | ---: |
| 1 | `src/codegen/lir/stmt_emitter_expr.cpp` | 6.578 | 85882 | 382 |
| 2 | `src/frontend/hir/hir_expr.cpp` | 5.226 | 84245 | 362 |
| 3 | `src/frontend/hir/hir_stmt.cpp` | 4.998 | 83946 | 345 |
| 4 | `src/frontend/hir/hir_templates.cpp` | 4.886 | 85265 | 360 |
| 5 | `src/codegen/lir/stmt_emitter_call.cpp` | 3.939 | 85628 | 384 |
| 6 | `src/frontend/parser/parser_declarations.cpp` | 2.957 | 65266 | 310 |
| 7 | `src/frontend/parser/parser_types_base.cpp` | 2.696 | 67811 | 317 |
| 8 | `src/frontend/parser/parser_expressions.cpp` | 1.205 | 55439 | 252 |
| 9 | `src/frontend/parser/parser_statements.cpp` | 0.872 | 54420 | 249 |

## Immediate Takeaways

- The hottest files in this first pass are LIR/HIR units, not parser units.
- The top five files all preprocess to roughly 84k to 86k lines, which is
  materially larger than their raw source size and supports the header
  amplification concern from the source idea.
- `stmt_emitter_expr.cpp` is the current top hotspot and is slightly heavier
  than the leading HIR units on both wall time and include count.
- Parser cost is still non-trivial, but the first-pass parser tier is clearly
  below the top HIR/LIR tier in optimized compile time.

## Step 2 Candidates

- `src/codegen/lir/stmt_emitter_expr.cpp`
- `src/frontend/hir/hir_expr.cpp`
- `src/frontend/hir/hir_stmt.cpp`
- `src/frontend/hir/hir_templates.cpp`
- `src/codegen/lir/stmt_emitter_call.cpp`

These are the best next candidates for `-fsyntax-only` versus `-O0 -c` versus
optimized `-c` classification.

## Step 2: Parse Versus Optimization Split

Method:

- Reused each TU's generated optimized compile command from
  `build/compile_commands.json`.
- Timed three variants per TU: `-fsyntax-only`, `-O0 -c`, and the existing
  optimized `-O2 -c` command.
- Sampled GCC `-ftime-report` on `stmt_emitter_expr.cpp` and
  `hir_templates.cpp` to confirm whether the optimized delta comes from
  optimization/code generation or from frontend work alone.

| Translation unit | `-fsyntax-only` (s) | `-O0 -c` (s) | `-O2 -c` (s) | Classification |
| --- | ---: | ---: | ---: | --- |
| `src/codegen/lir/stmt_emitter_expr.cpp` | 1.170 | 2.250 | 6.712 | Optimizer heavy |
| `src/frontend/hir/hir_expr.cpp` | 0.685 | 1.793 | 4.899 | Optimizer heavy |
| `src/frontend/hir/hir_stmt.cpp` | 0.799 | 1.959 | 5.098 | Optimizer heavy |
| `src/frontend/hir/hir_templates.cpp` | 0.790 | 1.676 | 5.031 | Optimizer heavy |
| `src/codegen/lir/stmt_emitter_call.cpp` | 0.744 | 1.781 | 4.246 | Optimizer heavy |

## Step 2 Interpretation

- None of the current top-five hotspot units are parse/include heavy in the
  narrow Step 2 sense. All five spend most of their optimized compile wall time
  beyond the `-O0 -c` baseline.
- The parse/include floor is still material. Even `-fsyntax-only` remains
  roughly `0.7s` to `1.2s` across the top tier, which matches the earlier
  84k to 86k preprocessed-line counts and keeps header pressure relevant for
  later extraction choices.
- `stmt_emitter_expr.cpp` is the clearest optimizer-dominated outlier. Its
  optimized compile is about `3.0x` the `-O0` compile and about `5.7x` the
  syntax-only pass.
- The HIR units show the same shape with a slightly smaller spread. Their
  optimized compiles are about `2.5x` to `3.0x` the `-O0` baseline, so they are
  also better treated as optimizer-heavy than as parse-only hotspots.
- The `-ftime-report` sample supports that classification. For
  `stmt_emitter_expr.cpp`, GCC reports `phase opt and generate` at `5.79s` of
  `7.54s` total wall time, with `callgraph functions expansion` alone taking
  `4.37s`. For `hir_templates.cpp`, `phase opt and generate` is `3.90s` of
  `5.40s`, with `callgraph functions expansion` at `3.07s`.

## Step 3 Direction

- The first extraction seam should favor large optimizer-stressing dispatcher
  or helper bodies inside `stmt_emitter_expr.cpp`, `hir_templates.cpp`, or the
  other hot HIR units rather than prioritizing header-only cleanup first.
- Header extractions still matter, but the current data says they are unlikely
  to erase the largest hotspot costs on their own.

## Step 3: Prioritized Extraction Seams

1. `src/codegen/lir/stmt_emitter_expr.cpp`: move the AArch64 `va_arg` lowering
   path (`emit_aarch64_vaarg_gp_src_ptr`, `emit_aarch64_vaarg_fp_src_ptr`, and
   `emit_rval_payload(..., const VaArgExpr&, ...)`) into a dedicated
   implementation file. This cluster is already declared in
   `stmt_emitter.hpp`, is ABI-local rather than expression-generic, and has a
   narrow validation surface through the existing AArch64/runtime variadic
   tests.
2. `src/codegen/lir/stmt_emitter_expr.cpp`: split the remaining unary/binary
   lowering dispatchers into smaller out-of-line helper families, especially
   the complex-arithmetic/comparison and cast-heavy branches inside
   `emit_rval_payload(..., const UnaryExpr&, ...)` and
   `emit_rval_payload(..., const BinaryExpr&, ...)`.
3. `src/frontend/hir/hir_templates.cpp`: isolate the member-typedef and
   pending-template-resolution cluster around
   `resolve_struct_member_typedef_type` and
   `resolve_struct_member_typedef_if_ready` behind narrower helper entry
   points. This area is large, stateful, and internally cohesive enough to
   split without mixing parser work into the same slice.

## Step 4: First Executed Slice

- Extracted the AArch64 `va_arg` lowering cluster from
  `src/codegen/lir/stmt_emitter_expr.cpp` into the new
  `src/codegen/lir/stmt_emitter_vaarg.cpp` translation unit and wired it into
  CMake.
- Kept the ABI-sensitive logic inside `StmtEmitter` methods so the change is a
  file-boundary extraction, not a semantic rewrite.
- Reused existing variadic coverage rather than introducing new tests because
  the move preserved behavior without changing syntax or emitted semantics.

## Step 4 Measurements

Optimized single-TU compile timings after the split, using the generated
`build/compile_commands.json` commands:

| Translation unit | Before `-O2 -c` (s) | After `-O2 -c` (s) | Delta (s) |
| --- | ---: | ---: | ---: |
| `src/codegen/lir/stmt_emitter_expr.cpp` | 6.712 | 5.493 | -1.219 |
| `src/codegen/lir/stmt_emitter_vaarg.cpp` | n/a | 2.799 | n/a |

Interpretation:

- The hottest expression TU dropped by about `18%` after moving the AArch64
  variadic lowering cluster behind its own `.cpp` boundary.
- This measurement is directly comparable for rebuilds that touch
  `stmt_emitter_expr.cpp`. A full clean build now also compiles the new
  `stmt_emitter_vaarg.cpp`, so the result should be read as a reduced hotspot
  rebuild surface rather than a claim about total whole-project compile time.

## Step 4: Refreshed Ranking After The `stmt_emitter_expr.cpp` Splits

The targeted optimized single-TU rerun used to select the next slice produced
this updated hotspot order:

| Rank | Translation unit | Optimized compile (s) |
| --- | --- | ---: |
| 1 | `src/frontend/hir/hir_expr.cpp` | 4.933 |
| 2 | `src/frontend/hir/hir_templates.cpp` | 4.295 |
| 3 | `src/frontend/hir/hir_stmt.cpp` | 4.275 |
| 4 | `src/codegen/lir/stmt_emitter_call.cpp` | 3.547 |
| 5 | `src/codegen/lir/stmt_emitter_expr.cpp` | 2.986 |

Interpretation:

- After the two `stmt_emitter_expr.cpp` extractions, the next hottest tier is
  HIR-led rather than LIR-led.
- `hir_templates.cpp` stayed close enough to the top that the member-typedef
  helper seam remained a valid Step 4 follow-on.

## Step 4: Third Executed Slice

- Extracted the deferred template-owner/member-typedef helper cluster
  (`require_pending_template_type_primary`,
  `resolve_pending_template_owner_if_ready`,
  `spawn_pending_template_owner_work`,
  `ensure_pending_template_owner_ready`,
  `resolve_deferred_member_typedef_type`,
  `seed_template_type_dependency_if_needed`, and
  `seed_and_resolve_pending_template_type_if_needed`) out of
  `src/frontend/hir/hir_templates.cpp` into the new
  `src/frontend/hir/hir_templates_member_typedef.cpp`.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/template_member_owner_signature_local_hir.cpp`
  to keep deferred member-typedef resolution concrete across both function
  signatures and instantiated locals.

## Step 4: Third Slice Measurements

Optimized single-TU compile timings after the split, using regenerated
`build/compile_commands.json` commands:

| Translation unit | Before `-O2 -c` (s) | After `-O2 -c` (s) | Notes |
| --- | ---: | ---: | --- |
| `src/frontend/hir/hir_templates.cpp` | 4.295 | 4.342 to 4.527 | three reruns after the split |
| `src/frontend/hir/hir_templates_member_typedef.cpp` | n/a | 1.443 to 1.487 | three reruns after the split |

Interpretation:

- This slice succeeded as a behavior-preserving file-boundary extraction, but
  the current timing reruns do not show a clear compile-time improvement for
  `hir_templates.cpp`.
- Because the before/after delta is roughly flat to slightly worse on the main
  hotspot TU, this change should be treated as structure preparation for later
  HIR work rather than as a measured compile-time win.

## Step 4: Refreshed Ranking Before The `hir_expr.cpp` Split

Recreated `build/compile_commands.json`, rebuilt the tree, and re-ran the
optimized single-TU timings that drive the active Step 4 choice:

| Rank | Translation unit | Optimized compile (s) |
| --- | --- | ---: |
| 1 | `src/frontend/hir/hir_expr.cpp` | 9.281 |
| 2 | `src/frontend/hir/hir_stmt.cpp` | 8.841 |
| 3 | `src/frontend/hir/hir_templates.cpp` | 8.780 |
| 4 | `src/codegen/lir/stmt_emitter_call.cpp` | 6.327 |
| 5 | `src/codegen/lir/stmt_emitter_expr.cpp` | 3.770 |

Interpretation:

- `hir_expr.cpp` retook the lead hotspot position, with `hir_stmt.cpp` and
  `hir_templates.cpp` close behind in the same HIR-heavy tier.
- The next extraction should therefore move from the earlier
  `hir_templates.cpp` follow-up to a `hir_expr.cpp` seam with a focused HIR
  and runtime validation surface.

## Step 4: Fourth Executed Slice

- Extracted the call-lowering helper cluster
  (`try_lower_template_struct_call`, `lower_call_arg`,
  `try_expand_pack_call_arg`, `try_lower_member_call_expr`,
  `lower_call_expr`, and `try_lower_consteval_call_expr`) out of
  `src/frontend/hir/hir_expr.cpp` into the new
  `src/frontend/hir/hir_expr_call.cpp`.
- Kept the logic as existing `Lowerer` methods, so this remains a
  translation-unit ownership split rather than a semantic rewrite.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/hir_expr_call_member_helper_hir.cpp` to pin the
  constructor, member-call, and ref-qualified operator-call lowering shape
  exercised by the extracted helper family.

## Step 4: Fourth Slice Measurements

Optimized single-TU compile timings after the split, using regenerated
`build/compile_commands.json` commands:

| Translation unit | Before `-O2 -c` (s) | After `-O2 -c` (s) | Notes |
| --- | ---: | ---: | --- |
| `src/frontend/hir/hir_expr.cpp` | 9.281 | 3.549 | direct post-split rerun |
| `src/frontend/hir/hir_expr_call.cpp` | n/a | 2.884 | new extracted helper TU |
| `src/frontend/hir/hir_stmt.cpp` | 8.841 | 4.710 | refreshed comparison tier |
| `src/frontend/hir/hir_templates.cpp` | 8.780 | 4.634 | refreshed comparison tier |

Interpretation:

- The targeted `hir_expr.cpp` hotspot dropped by `5.732s`, about `61.8%`,
  after moving the call-lowering cluster behind its own `.cpp` boundary.
- The refreshed comparison tier after that split showed `hir_templates.cpp`
  and `hir_stmt.cpp` still close enough to justify returning to HIR helper
  extraction rather than shifting back to parser work.

## Step 4: Seventh Executed Slice

- Refreshed the targeted optimized single-TU ranking after the
  `hir_templates.cpp` struct-instantiation split:

| Rank | Translation unit | Optimized compile (s) |
| --- | --- | ---: |
| 1 | `src/frontend/hir/hir_templates.cpp` | 4.237 |
| 2 | `src/codegen/lir/stmt_emitter_call.cpp` | 3.996 |
| 3 | `src/frontend/hir/hir_stmt.cpp` | 3.901 |
| 4 | `src/frontend/hir/hir_expr.cpp` | 3.834 |
| 5 | `src/codegen/lir/stmt_emitter_expr.cpp` | 3.261 |

- Because `hir_templates.cpp` remained the hottest unit, extracted the
  template member-typedef type-resolution cluster
  (`resolve_struct_member_typedef_type` and
  `resolve_struct_member_typedef_if_ready`) into the new
  `src/frontend/hir/hir_templates_type_resolution.cpp`.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/template_inherited_member_typedef_trait_hir.cpp`
  to pin inherited trait-style `::type` resolution through a realized base
  struct.

## Step 4: Seventh Slice Measurements

Optimized single-TU compile timings after the split, using regenerated
`build/compile_commands.json` commands:

| Translation unit | Before `-O2 -c` (s) | After `-O2 -c` (s) | Notes |
| --- | ---: | ---: | --- |
| `src/frontend/hir/hir_templates.cpp` | 4.283 | 4.021 | direct `HEAD` vs post-split comparison |
| `src/frontend/hir/hir_templates_type_resolution.cpp` | n/a | 1.581 | new extracted helper TU |

Interpretation:

- This slice preserved behavior and reduced the lead hotspot TU by `0.262s`,
  about `6.1%`, which makes it another measured compile-time improvement for
  `hir_templates.cpp`.
- The refreshed current hotspot tier is now:

| Rank | Translation unit | Optimized compile (s) |
| --- | --- | ---: |
| 1 | `src/frontend/hir/hir_templates.cpp` | 4.247 |
| 2 | `src/codegen/lir/stmt_emitter_call.cpp` | 4.097 |
| 3 | `src/frontend/hir/hir_stmt.cpp` | 4.063 |
| 4 | `src/frontend/hir/hir_expr.cpp` | 3.641 |
| 5 | `src/codegen/lir/stmt_emitter_expr.cpp` | 3.258 |
- The refreshed tier now shifts leadership to `hir_stmt.cpp` and
  `hir_templates.cpp`, so a subsequent Step 4 slice should likely move there
  rather than returning immediately to `hir_expr.cpp`.

## Step 4: Fifth Executed Slice

- Extracted the `NK_RANGE_FOR` lowering branch from
  `src/frontend/hir/hir_stmt.cpp` into the new
  `src/frontend/hir/hir_stmt_range_for.cpp`.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/hir_stmt_range_for_helper_hir.cpp` so the
  helper split keeps the synthetic iterator locals and iterator-call lowering
  shape stable.

## Step 4: Fifth Slice Measurements

Optimized single-TU compile timings for the range-for split:

| Translation unit | Before `-O2 -c` (s) | After `-O2 -c` (s) | Notes |
| --- | ---: | ---: | --- |
| `src/frontend/hir/hir_stmt.cpp` | 5.231 | 10.111 | before compiled from `HEAD`, after from the working tree |
| `src/frontend/hir/hir_stmt_range_for.cpp` | n/a | 1.958 | new extracted TU |

Interpretation:

- This slice preserved behavior and improved file structure, but the measured
  hotspot timing for `hir_stmt.cpp` moved in the wrong direction.
- It should therefore be treated as structure-only groundwork and not as a
  compile-time win.

## Step 4: Sixth Executed Slice

- Extracted the template-struct instantiation/body helper cluster
  (`apply_template_typedef_bindings`,
  `materialize_template_array_extent`,
  `append_instantiated_template_struct_bases`,
  `register_instantiated_template_struct_methods`,
  `record_instantiated_template_struct_field_metadata`,
  `instantiate_template_struct_field`,
  `append_instantiated_template_struct_fields`, and
  `instantiate_template_struct_body`) out of
  `src/frontend/hir/hir_templates.cpp` into the new
  `src/frontend/hir/hir_templates_struct_instantiation.cpp`.
- Kept the logic as existing `Lowerer` methods, so this remains a
  translation-unit ownership split rather than a semantic rewrite.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/template_struct_body_instantiation_hir.cpp` to
  pin inherited member access, NTTP array extent materialization, and the
  instantiated method body shape exercised by the extracted helper family.

## Step 4: Sixth Slice Measurements

Optimized single-TU compile timings for the struct-instantiation split:

| Translation unit | Before `-O2 -c` (s) | After `-O2 -c` (s) | Notes |
| --- | ---: | ---: | --- |
| `src/frontend/hir/hir_templates.cpp` | 5.087 | 4.241 | before compiled from `HEAD`, after from the working tree |
| `src/frontend/hir/hir_templates_struct_instantiation.cpp` | n/a | 1.384 | new extracted TU |

Interpretation:

- Unlike the earlier member-typedef split, this `hir_templates.cpp` slice did
  produce a measured hotspot reduction on the main TU.
- The main `hir_templates.cpp` rebuild surface dropped by `0.846s`, about
  `16.6%`, while preserving the template-struct body/materialization behavior.

## Step 4: Ninth Executed Slice

- Extracted the builtin-call helper family from
  `src/codegen/lir/stmt_emitter_call.cpp` into the new
  `src/codegen/lir/stmt_emitter_call_builtin.cpp`.
- Kept the extracted logic as existing `StmtEmitter` methods, so this remains a
  translation-unit ownership split around builtin dispatch rather than a
  semantic rewrite.
- Added focused runtime coverage in
  `tests/c/internal/positive_case/ok_call_builtin_runtime.c` for the
  integer-bit, floating-point math, `__builtin_classify_type`, and complex
  conjugation call paths exercised by the extracted helper family.
- Re-ran nearby call-lowering coverage with
  `tests/c/internal/compare_case/smoke_call_lowering.c` in compare mode.

## Step 4: Ninth Slice Measurements

Optimized single-TU compile timings for the builtin-call split:

| Translation unit | Before `-O2 -c` (s) | After `-O2 -c` (s) | Notes |
| --- | ---: | ---: | --- |
| `src/codegen/lir/stmt_emitter_call.cpp` | 4.463 | 4.007 | before compiled from `HEAD^`, after from the working tree |
| `src/codegen/lir/stmt_emitter_call_builtin.cpp` | n/a | 2.718 | new extracted TU |

Interpretation:

- This slice preserved behavior and reduced the main call-lowering hotspot TU
  by `0.456s`, about `10.2%`.
- With `stmt_emitter_call.cpp` now lower than its pre-split measurement, the
  next extraction choice should refresh the current HIR/LIR tier before picking
  the next seam between `hir_templates.cpp` and `hir_stmt.cpp`.
