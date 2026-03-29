# std::vector Bring-up Runbook

Status: Active
Source Idea: ideas/open/std_vector_bringup_plan.md

## Purpose

Push `tests/cpp/std/std_vector_simple.cpp` from its current parser/header
compatibility failure state to a stable, regression-guarded passing state.

## Goal

Make `tests/cpp/std/std_vector_simple.cpp` compile successfully with reduced
repros and regression coverage for each blocker found along the way.

## Core Rule

Implement only the smallest parser / preprocessor / sema / codegen change needed
for the current blocker, and add a reduced test before broadening scope.

## Read First

- use `tests/cpp/std/std_vector_simple.cpp` as the primary end-to-end target
- treat Clang / LLVM behavior as the reference when behavior is ambiguous
- preserve the narrow scope of this bring-up; do not turn it into generic STL
  support work
- compare-mode validation only matters once the case reaches codegen

## Current Target

The active blocker is the later parse-state-loss failure now reported while
processing the `std::vector` header stack:

- current direct repro: `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp`
- current failure shape: `error: expected RBRACE but got '' at line 10`

## Non-Goals

- broad libstdc++ compatibility claims beyond this case
- unrelated frontend cleanups
- random allowlists for implementation identifiers
- codegen-path triage before the case reaches lowering

## Working Model

1. lock in a cheap repro path for `std_vector_simple.cpp`
2. reduce the next blocker into the smallest standalone parser/frontend test
3. fix the narrowest responsible layer
4. rerun the reduced test, nearby subsystem tests, and the full suite
5. update `plan_todo.md` with the next blocker before ending the slice

## Execution Rules

- update `plan_todo.md` before implementation and when the active slice changes
- add or update the reduced validating test before the implementation change
- if the failure is unclear, capture preprocessed output and inspect the reduced
  region before changing code
- if the case reaches codegen, validate with `--codegen=legacy|lir|compare`
- if new work is adjacent but not required for this blocker, record it back in
  the source idea instead of silently expanding the active plan

## Ordered Steps

### Step 1: Reproduce The Active Failure

Goal:
- confirm the current parser/header failure from `std_vector_simple.cpp`

Actions:
- build the tree and record a full-suite baseline in `test_before.log`
- run the documented direct repro command
- capture enough preprocessed or parser output to localize the failure region

Completion Check:
- the active failure is reproduced locally and localized enough to reduce

### Step 2: Add A Reduced Repro Test

Goal:
- turn the next blocker into the narrowest standalone test

Primary Target:
- parser/frontend handling for the construct that causes the unmatched-brace or
  parse-state-loss failure

Actions:
- create a focused test under `tests/cpp/internal/` that fails before the fix
- register the test in the relevant CMake list if needed
- prefer a parser-only or sema-only reduced case over a `<vector>`-dependent test

Completion Check:
- one reduced test reproduces the blocker without depending on the full header
  stack

### Step 3: Implement The Narrow Fix

Goal:
- make the reduced test and the end-to-end `std_vector_simple.cpp` case move
  forward

Primary Target:
- the narrowest responsible frontend layer

Actions:
- inspect the exact parser / sema path handling the reduced construct
- implement the smallest behavior change that matches the reduced case
- avoid unrelated refactors while in the failing subsystem

Completion Check:
- the reduced test passes and `std_vector_simple.cpp` gets farther or reaches the
  next meaningful boundary

### Step 4: Regressions And Next Slice

Goal:
- prove the slice did not regress the suite and record the next blocker

Actions:
- rerun the reduced test, nearby parser/frontend tests, and the full suite
- write `test_after.log`
- compare the before/after suite results for monotonicity
- update `plan_todo.md` with completed work, next target, and blockers

Completion Check:
- suite is monotonic and the next execution slice is recorded

### Step 5: Later Bring-up Phases

Goal:
- finish the remaining planned phases once parsing succeeds

Actions:
- continue deeper reductions until the case reaches semantics/codegen
- validate under `--codegen=legacy|lir|compare` once lowering is possible
- promote the case into maintained regression coverage after it passes

Completion Check:
- `tests/cpp/std/std_vector_simple.cpp` compiles and remains regression-guarded
