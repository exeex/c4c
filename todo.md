# Frontend Compile-Time Hotspots And Extraction Candidates

Status: Active
Source Idea: ideas/open/02_frontend_compile_time_hotspots_and_extraction_candidates.md
Source Plan: plan.md

## Active Item

- Step 3: Identify the first low-risk extraction seam in the optimizer-heavy
  HIR/LIR hotspot tier.

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

## Next Slice

- Inspect `stmt_emitter_expr.cpp` and `hir_templates.cpp` for large
  dispatcher/helper bodies that can be split behind behavior-preserving helper
  boundaries.
- Prefer a first slice that reduces optimizer pressure without widening header
  fan-out or mixing parser/HIR/LIR refactors together.

## Blockers

- None recorded.

## Notes

- Follow numeric ordering from `ideas/open/`; the active source idea is `02`.
- Step 1 is complete for the initial target set.
- The current top hotspot is `src/codegen/lir/stmt_emitter_expr.cpp` at 6.578s.
- The first-pass top five hotspot units all preprocess to roughly 84k to 86k
  lines and have 345 to 384 include-tree entries.
- Step 2 is complete: the top-five hotspot tier is optimizer heavy rather than
  parse-heavy, though all five keep a meaningful `-fsyntax-only` floor.
- The latest `ctest --test-dir build -j --output-on-failure` rerun still
  passes 3319/3319 tests, and the monotonic regression guard remains green.
