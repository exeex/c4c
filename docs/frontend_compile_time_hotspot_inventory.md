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
