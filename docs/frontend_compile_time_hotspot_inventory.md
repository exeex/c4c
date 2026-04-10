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
