# Frontend Compile-Time Hotspots And Extraction Candidates

Status: Active
Source Idea: ideas/open/02_frontend_compile_time_hotspots_and_extraction_candidates.md
Source Plan: plan.md

## Active Item

- Step 2: Separate parse cost from optimization cost for the hottest HIR and
  frontend-adjacent LIR units.

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

## Next Slice

- Compare `-fsyntax-only`, `-O0 -c`, and optimized `-c` for
  `stmt_emitter_expr.cpp`, `hir_expr.cpp`, `hir_stmt.cpp`,
  `hir_templates.cpp`, and `stmt_emitter_call.cpp`.
- Use the split to classify each as parse/include heavy, optimizer heavy, or
  mixed before choosing the first extraction seam.

## Blockers

- None recorded.

## Notes

- Follow numeric ordering from `ideas/open/`; the active source idea is `02`.
- Step 1 is complete for the initial target set.
- The current top hotspot is `src/codegen/lir/stmt_emitter_expr.cpp` at 6.578s.
- The first-pass top five hotspot units all preprocess to roughly 84k to 86k
  lines and have 345 to 384 include-tree entries.
