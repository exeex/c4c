# std::vector System-Header Frontline Reduction Runbook

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Activated from: prompts/AGENT_PROMPT_ACTIVATE_PLAN.md

## Purpose

Turn the parked `std::vector` bring-up note into an execution runbook that
reduces the current libstdc++ parser frontier in small, regression-guarded
slices.

## Goal

Move `tests/cpp/std/std_vector_simple.cpp` from the current parse-only frontier
toward stable frontend acceptance by reducing one concrete system-header blocker
at a time.

## Core Rule

Use `tests/cpp/std/std_vector_simple.cpp` only as the forcing repro. Reduce
each blocker into the narrowest internal testcase before changing parser code.
Do not broaden this plan into generic standard-library support promises.

## Read First

- [ideas/open/04_std_vector_bringup_plan.md](/workspaces/c4c/ideas/open/04_std_vector_bringup_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)
- [tests/cpp/std/std_vector_simple.cpp](/workspaces/c4c/tests/cpp/std/std_vector_simple.cpp)
- [src/frontend/parser/expressions.cpp](/workspaces/c4c/src/frontend/parser/expressions.cpp)
- [src/frontend/parser/declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
- [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
- [tests/cpp/internal/postive_case](/workspaces/c4c/tests/cpp/internal/postive_case)

## Current Targets

The current reduced frontier should stay limited to blockers reflected in the
latest `std::vector` repro:

1. `noexcept` expression parsing in libstdc++ helper code when a new reduced
   case still exists beyond the already-landed `noexcept_expr_parse.cpp`
2. concept-heavy iterator helper declarations around
   `/usr/include/c++/14/bits/iterator_concepts.h`
3. adjacent parser-boundary bugs only when they are directly required to move
   the `std::vector` repro forward

## Non-Goals

- full `std::vector` semantic correctness in one slice
- unrelated standard-library cleanup outside the active reduced blocker
- generic builtin-trait implementation work unless the current reduced case
  proves it is the immediate root cause
- reactivating the backend umbrella roadmap

## Working Model

- Reproduce the current `std::vector` parse-only failure first.
- Reduce the lead blocker into one internal positive-case parser regression.
- Fix only the parser mechanism required by that reduced case.
- Re-run the direct `std::vector` repro after each slice to confirm the frontier
  moves forward or becomes better localized.
- If a reduction exposes a separate cross-cutting initiative, record it in
  `ideas/open/` and switch lifecycle state instead of mutating this plan.

## Execution Rules

- Follow a test-first workflow for each reduced blocker.
- Prefer parser-hygiene tightening over broad skip/recovery rules.
- Keep each patch to one mechanism family.
- Preserve existing reduced regressions added during earlier bring-up slices.
- Record the exact surviving `std::vector` repro failure after each completed
  slice.

## Ordered Steps

### Step 1: Reconfirm the live std::vector frontier

Goal:
- capture the current parse-only failure shape for
  `tests/cpp/std/std_vector_simple.cpp`

Primary target:
- [tests/cpp/std/std_vector_simple.cpp](/workspaces/c4c/tests/cpp/std/std_vector_simple.cpp)

Actions:
- run `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp`
- record the first surviving parser errors and affected headers
- compare the lead failure against the parked idea notes from 2026-03-29

Completion check:
- `plan_todo.md` records one concrete lead blocker with exact error text or
  header location

### Step 2: Reduce the lead blocker to an internal testcase

Goal:
- isolate the current repro into the smallest parser regression that can be run
  without the full system-header stack

Primary target:
- [tests/cpp/internal/postive_case](/workspaces/c4c/tests/cpp/internal/postive_case)

Actions:
- reduce the current header failure to one internal positive-case testcase
- register the testcase in
  [tests/cpp/internal/InternalTests.cmake](/workspaces/c4c/tests/cpp/internal/InternalTests.cmake)
- keep the testcase scoped to the active blocker only

Completion check:
- one new or updated reduced testcase reproduces the live blocker independently

### Step 3: Fix the narrow parser mechanism

Goal:
- teach the parser to accept the reduced blocker without widening unrelated
  recovery behavior

Primary targets:
- [src/frontend/parser/expressions.cpp](/workspaces/c4c/src/frontend/parser/expressions.cpp)
- [src/frontend/parser/declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
- [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)

Actions:
- inspect the parser path reached by the reduced testcase
- patch the smallest shared parsing mechanism that resolves the failure
- avoid unrelated cleanup

Completion check:
- the reduced testcase passes and nearby parser coverage stays green

### Step 4: Re-run the forcing repro and stage the next frontier

Goal:
- prove the direct `std::vector` repro moved forward and document the next
  blocker clearly

Primary target:
- [tests/cpp/std/std_vector_simple.cpp](/workspaces/c4c/tests/cpp/std/std_vector_simple.cpp)

Actions:
- re-run `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp`
- record whether the lead failure moved, disappeared, or exposed a different
  localized frontier
- fold the new repro status into `plan_todo.md`

Completion check:
- the direct repro is strictly improved or the next blocker is more localized
  than before the slice

### Step 5: Validate with regression guardrails

Goal:
- prove the slice improved or preserved suite health without new unrelated
  failures

Actions:
- run the targeted reduced tests for the completed blocker
- run nearby parser tests in the same subsystem
- run the full configured `ctest` suite before handoff
- compare before/after logs and keep the result monotonic

Completion check:
- targeted tests pass for the finished slice
- full-suite results are monotonic with no newly failing tests
- `plan_todo.md` names the next intended `std::vector` frontier
