# Frontend Compile-Time Hotspots And Extraction Candidates

Status: Active
Source Idea: ideas/open/02_frontend_compile_time_hotspots_and_extraction_candidates.md

## Purpose

Build a measured frontend compile-time map, then turn the highest-confidence
results into a small, behavior-preserving extraction plan that can be executed
incrementally.

## Goal

Identify the most expensive frontend and frontend-adjacent translation units,
separate parse/include cost from optimizer cost, and produce a prioritized set
of low-risk extraction targets with validation expectations.

## Core Rule

Measure first, extract second, and do not claim compile-time wins without
recorded before/after data for the touched hotspot units.

## Read First

- [ideas/open/02_frontend_compile_time_hotspots_and_extraction_candidates.md](/workspaces/c4c/ideas/open/02_frontend_compile_time_hotspots_and_extraction_candidates.md)
- [prompts/EXECUTE_PLAN.md](/workspaces/c4c/prompts/EXECUTE_PLAN.md)

## Scope

- [src/frontend/parser](/workspaces/c4c/src/frontend/parser)
- [src/frontend/hir](/workspaces/c4c/src/frontend/hir)
- [src/frontend/sema](/workspaces/c4c/src/frontend/sema)
- [src/frontend/preprocessor](/workspaces/c4c/src/frontend/preprocessor)
- [src/frontend/lexer](/workspaces/c4c/src/frontend/lexer)
- [src/codegen/lir](/workspaces/c4c/src/codegen/lir)
- frontend-owned headers that materially affect rebuild cost for those units
- build settings or compile modes that materially change frontend compile time

## Non-Goals

- backend-only architecture work
- broad unmeasured cleanup bundles
- speculative full-frontend rewrites
- container redesigns that are not required by a measured extraction step

## Working Model

- Treat compile-time investigation as implementation work with testable artifacts.
- Keep durable findings in the source idea when they matter beyond the current
  iteration.
- Prefer stable semantic seams such as dispatcher splitting, helper extraction,
  and moving heavy implementations out of widely included headers.
- Land changes in small slices that preserve diagnostics and existing behavior.

## Execution Rules

- Start from measurement and evidence, not intuition.
- Separate `-fsyntax-only`, `-O0 -c`, and optimized `-c` cost where possible.
- Record preprocess size, include fan-in, and timing output for hotspot units
  before choosing extraction targets.
- Do not mix parser, HIR, and LIR refactors into one broad patch unless the
  measured hotspot data requires it.
- When a new but separate initiative appears, write it back into
  `ideas/open/*.md` instead of silently expanding this plan.

## Step 1: Build The Hotspot Inventory

Goal: produce a ranked list of the most expensive frontend and
frontend-adjacent translation units under the common optimized build.

Primary targets:

- [src/frontend/hir/hir_expr.cpp](/workspaces/c4c/src/frontend/hir/hir_expr.cpp)
- [src/frontend/hir/hir_stmt.cpp](/workspaces/c4c/src/frontend/hir/hir_stmt.cpp)
- [src/frontend/hir/hir_templates.cpp](/workspaces/c4c/src/frontend/hir/hir_templates.cpp)
- [src/frontend/parser/parser_types_base.cpp](/workspaces/c4c/src/frontend/parser/parser_types_base.cpp)
- [src/frontend/parser/parser_declarations.cpp](/workspaces/c4c/src/frontend/parser/parser_declarations.cpp)
- [src/frontend/parser/parser_expressions.cpp](/workspaces/c4c/src/frontend/parser/parser_expressions.cpp)
- [src/frontend/parser/parser_statements.cpp](/workspaces/c4c/src/frontend/parser/parser_statements.cpp)
- [src/codegen/lir/stmt_emitter_expr.cpp](/workspaces/c4c/src/codegen/lir/stmt_emitter_expr.cpp)
- [src/codegen/lir/stmt_emitter_call.cpp](/workspaces/c4c/src/codegen/lir/stmt_emitter_call.cpp)

Actions:

- Measure representative single-TU compile times for the target set.
- Record preprocess size and any immediately obvious include amplification.
- Capture enough data to rank the first extraction candidates.

Completion check:

- A ranked hotspot list exists in working notes or `todo.md`.
- At least one parser file, one HIR file, and one frontend-adjacent codegen
  file have comparable measurements.

## Step 2: Separate Parse Cost From Optimization Cost

Goal: determine whether the worst hotspots are dominated by
parse/include cost, optimizer cost, or both.

Actions:

- Compare `-fsyntax-only`, `-O0 -c`, and optimized `-c` for the hottest units.
- Use compiler timing output such as `-ftime-report` where it clarifies pass
  cost or template-instantiation weight.
- Distinguish TU compile cost from final link cost.

Completion check:

- The hottest units have a short classification: parse/include heavy,
  optimizer heavy, or mixed.
- The plan can justify whether extraction should focus on headers, giant
  functions, or both.

## Step 3: Identify Extraction Seams

Goal: translate measurements into a short list of behavior-preserving
extraction targets with low semantic risk.

Primary targets:

- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [src/frontend/parser/types_helpers.hpp](/workspaces/c4c/src/frontend/parser/types_helpers.hpp)
- [src/frontend/hir/hir_lowerer_internal.hpp](/workspaces/c4c/src/frontend/hir/hir_lowerer_internal.hpp)
- [src/frontend/hir/compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)
- [src/frontend/hir/hir_ir.hpp](/workspaces/c4c/src/frontend/hir/hir_ir.hpp)

Actions:

- Identify giant dispatcher-style functions that should become thin
  coordinators plus named helpers.
- Identify heavy helper headers whose larger implementations can move behind
  `.cpp` boundaries.
- Prioritize seams that can land incrementally without changing semantics.

Completion check:

- A short prioritized extraction list exists with rationale tied to measured
  hotspots.
- Each candidate names the concrete file or function family to split or move.

## Step 4: Execute The Smallest High-Value Extraction Slice

Goal: land one measured, behavior-preserving extraction or structure change
from the prioritized list.

Actions:

- Choose the lowest-risk candidate with a clear validation surface.
- Add or update the narrowest tests needed before code changes.
- Implement the smallest extraction that improves structure without changing
  behavior.
- Record before/after compile-time measurements for the touched units.

Completion check:

- One extraction slice is implemented and validated.
- The touched hotspot units have before/after compile-time data.
- No new regression failures are introduced.

## Step 5: Validate And Preserve Findings

Goal: finish each slice with evidence, stable planning state, and clear next
steps.

Actions:

- Run targeted tests for the touched subsystem.
- Run the required broader regression checks before handoff.
- Update [todo.md](/workspaces/c4c/todo.md) with completed items, next slice,
  blockers, and measurement notes.
- If remaining work splits into a separate initiative, capture it under
  `ideas/open/`.

Completion check:

- Validation status is recorded.
- `todo.md` identifies the next highest-value slice.
- Another agent can resume without reconstructing measurements or scope.
