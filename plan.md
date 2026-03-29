# std::vector Bring-up Runbook

Status: Active
Source Idea: ideas/open/std_vector_bringup_plan.md

## Purpose

Drive `tests/cpp/std/std_vector_simple.cpp` from its current libstdc++ header-frontier failure state to a stable, regression-guarded passing state without broadening scope into general STL support.

## Goal

Remove the next concrete parser / sema / codegen blockers exposed by `std_vector_simple.cpp`, one reduced test-backed slice at a time, until the case parses, lowers, and is protected by regression coverage.

## Core Rule

Work only on the narrowest blocker directly exposed by `tests/cpp/std/std_vector_simple.cpp` or by a reduced repro derived from it. Every fix must land with a targeted validating test before broader regression runs.

## Read First

- [ideas/open/std_vector_bringup_plan.md](/workspaces/c4c/ideas/open/std_vector_bringup_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)

## Current Scope

- Keep the plan centered on `tests/cpp/std/std_vector_simple.cpp`.
- Reduce failures from libstdc++ headers into tiny standalone tests.
- Fix the narrowest responsible layer under:
  - `src/frontend/preprocessor/`
  - `src/frontend/parser/`
  - `src/frontend/sema/`
  - `src/frontend/hir/`
  - `src/codegen/llvm/`
- Compare behavior against Clang whenever the reduced case can be compiled there.

## Non-Goals

- Do not promise broad libstdc++ or STL completeness from this one bring-up.
- Do not add broad allowlists for implementation identifiers.
- Do not absorb adjacent diagnostics work unless it is required to unblock the current parser frontier.
- Do not perform unrelated opportunistic regression fixing.

## Working Model

1. Reproduce the current `std_vector_simple.cpp` failure cheaply.
2. Reduce the first structured blocker into the smallest standalone test.
3. Add the reduced test to `ctest` before changing compiler behavior.
4. Fix one root cause in the narrowest layer.
5. Re-run the reduced test, nearby subsystem tests, and the full suite.
6. Update `plan_todo.md` with the exact next blocker before stopping.

## Execution Rules

- Prefer `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp` for the initial repro.
- Use `--parser-debug` when the reported token mismatch is too flat to show the real failing parser entry point.
- For preprocess-only mismatches, compare against `clang -E`.
- For parse/codegen cases, compare against `clang -S -emit-llvm -O0`.
- Keep each slice shippable and commit one slice per commit.

## Ordered Steps

### Step 1: Lock In The Repro Harness

Goal: keep the main failing case cheap to rerun and easy to inspect.

Actions:

- Confirm the cheapest direct repro command still matches the current frontier.
- Preserve or add a documented quick-run workflow if the current command becomes ambiguous.
- Keep temporary reduced repro files long enough to localize the failing layer.

Completion Check:

- One command reproduces the current `std_vector_simple.cpp` status.

### Step 2: Reduce The Next Header Blocker

Goal: convert the first current libstdc++ frontier failure into a tiny standalone test.

Primary Targets:

- `tests/cpp/std/std_vector_simple.cpp`
- likely current header families:
  - `bits/stl_iterator.h`
  - `bits/predefined_ops.h`
  - `bits/stl_algobase.h`
  - `bits/new_allocator.h`
  - `bits/stl_uninitialized.h`

Actions:

- Inspect the current repro output and choose the earliest structured blocker.
- Reduce it to the smallest standalone source that still fails in the same way.
- Add the reduced test under `tests/cpp/internal/postive_case/` or the appropriate nearby test area.

Completion Check:

- A new reduced regression test exists and reproduces the chosen blocker before the fix.

### Step 3: Fix The Narrowest Responsible Layer

Goal: make the reduced blocker pass with the smallest defensible compiler change.

Actions:

- Classify the failure as preprocessor, parser, sema, HIR, or codegen.
- Inspect the responsible implementation surface and compare expected behavior with Clang when applicable.
- Implement the smallest change that fixes the reduced case without broad rewrites.

Completion Check:

- The new reduced test passes and the original `std_vector_simple.cpp` repro advances or changes in a meaningful way.

### Step 4: Regress Nearby Coverage

Goal: prove the slice did not break nearby functionality.

Actions:

- Re-run the targeted reduced test.
- Re-run nearby parser / sema tests in the same subsystem.
- Re-run the direct `std_vector_simple.cpp` repro.

Completion Check:

- Targeted coverage passes and the main repro does not regress.

### Step 5: Run Full Regression Guard

Goal: preserve full-suite monotonicity for each slice.

Actions:

- Record a `ctest` baseline before implementation if one has not been captured for the iteration.
- Run the full rebuild and `ctest --output-on-failure` after the slice.
- Compare before/after logs and confirm no previously passing test regressed.

Completion Check:

- Full-suite pass count is monotonic and no new failures were introduced.

### Step 6: Advance Toward Parse, Codegen, And Guarded Coverage

Goal: keep moving the case from header compatibility into actual `std::vector` parsing, lowering, and regression protection.

Actions:

- Repeat Steps 2 through 5 for each newly exposed blocker.
- Once the case compiles, validate with `--codegen=legacy`, `--codegen=lir`, and `--codegen=compare`.
- Promote the passing case into maintained regression coverage.

Completion Check:

- `tests/cpp/std/std_vector_simple.cpp` compiles successfully, reduced blocker tests cover the path it exposed, and the case is included in regression protection.
