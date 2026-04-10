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
