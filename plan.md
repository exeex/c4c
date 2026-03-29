# std::vector Bring-up Runbook

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Activated from: parked bring-up memo on 2026-03-29

## Purpose

Turn `tests/cpp/std/std_vector_simple.cpp` into a stable, regression-guarded
passing case by fixing only the parser/front-end gaps directly exposed by the
current libstdc++ frontier.

## Goal

Advance the active failing frontier from the current `noexcept` and
concept-heavy libstdc++ parse failures toward a passing `--parse-only` and then
full compile path for `tests/cpp/std/std_vector_simple.cpp`.

## Core Rule

Keep the plan narrow. Fix one concrete blocker at a time, add a reduced test
for that blocker first, and do not broaden this effort into general STL support
or unrelated frontend work.

## Read First

- [ideas/open/04_std_vector_bringup_plan.md](/workspaces/c4c/ideas/open/04_std_vector_bringup_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)
- Direct repro:
  `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp`
- Useful companions:
  `./build/c4cll --pp-only tests/cpp/std/std_vector_simple.cpp`
  `./build/c4cll --parser-debug --parse-only tests/cpp/std/std_vector_simple.cpp`

## Current Target

The source idea records that earlier libstdc++ blockers in `<type_traits>` and
`ext/numeric_traits.h` have been reduced already. The current active frontier is
the deeper parse layer around:

- `noexcept` appearing in expression contexts inside libstdc++ headers
- concept-heavy iterator helper declarations in
  `/usr/include/c++/14/bits/iterator_concepts.h`
- remaining expression/parser entry-point failures surfaced as unexpected
  `struct`, `typename`, or `...`

Execution should start with the cheapest blocker that can be isolated into a
small reduced test.

## Scope

- Reduce and fix parser/front-end gaps directly blocking
  `tests/cpp/std/std_vector_simple.cpp`
- Add targeted reduced regression tests for each newly isolated blocker
- Re-run the direct `std::vector` repro after each fix
- Keep regression evidence through targeted tests plus full-suite comparison

## Non-Goals

- Broad "support libstdc++" claims
- Random allowlists for implementation-private identifiers
- New diagnostics-only initiatives unless they are required to land the current
  blocker
- Backend/codegen redesign unrelated to this bring-up
- Activating the backend roadmap umbrella instead of this narrow header-compat
  slice

## Working Model

1. Reproduce the current `std::vector` failure cheaply.
2. Identify the first real parser/front-end blocker, not the noisiest error.
3. Reduce that blocker into the smallest standalone C++ test.
4. Add the reduced test to `ctest` before changing implementation.
5. Implement the smallest fix in the directly responsible subsystem.
6. Re-run the reduced test, nearby tests, and the `std::vector` repro.
7. Repeat until the next frontier is exposed.

## Execution Rules

- Treat `tests/cpp/std/std_vector_simple.cpp` as the contract case.
- Record the exact current slice in `plan_todo.md` before code changes.
- Prefer parser/support fixes attached to one reduced repro per root cause.
- Compare behavior against Clang when syntax or lowering intent is unclear.
- If a newly discovered issue is adjacent but not required for this bring-up,
  write it back to `ideas/open/` instead of silently expanding this plan.

## Ordered Steps

### Step 1: Lock the current repro harness

Goal: make the current frontier cheap to inspect and compare.

Primary target:
- `tests/cpp/std/std_vector_simple.cpp`

Actions:
- verify the direct repro command still fails in the shape recorded by the idea
- capture the first actionable failing locations
- preserve any temporary reduced source used to expose the real blocker

Completion check:
- one reproducible command confirms the current frontier
- the next slice names one concrete syntax/form to reduce

### Step 2: Reduce the next blocker

Goal: isolate the first parser/front-end blocker into a minimal regression test.

Primary target:
- whichever current libstdc++ construct fails first after Step 1 triage

Actions:
- derive the smallest standalone source matching the failing construct
- place the reduced repro under the appropriate parser/frontend test area
- register it in `ctest`
- confirm it fails before implementation

Completion check:
- a reduced test exists and fails for the same root cause as the `std::vector`
  repro

### Step 3: Implement the narrow fix

Goal: land the smallest behavior change required for the reduced repro.

Primary target:
- owning parser/front-end subsystem for the reduced case

Actions:
- inspect the responsible parser path
- implement one root-cause fix
- avoid unrelated refactors

Completion check:
- the reduced test passes
- no unrelated parser behavior is intentionally changed

### Step 4: Revalidate the `std::vector` frontier

Goal: confirm the fix moves the real bring-up forward.

Primary target:
- `tests/cpp/std/std_vector_simple.cpp`

Actions:
- rerun `--parse-only` on the `std::vector` case
- record whether the failure advanced, changed shape, or cleared
- if still failing, select the next blocker and return to Step 2

Completion check:
- the next frontier is explicitly recorded, or the `std::vector` parse succeeds

### Step 5: Guard with regression checks

Goal: prove monotonic progress.

Actions:
- run nearby targeted tests in the touched subsystem
- run full-suite before/after comparison as required by the execute prompt
- record pass-count and no-new-failures evidence in `plan_todo.md`

Completion check:
- targeted tests pass
- full-suite result is monotonic

### Step 6: Finish the bring-up

Goal: move from parse-only success to stable compile/codegen coverage for the
case, without broadening scope.

Actions:
- once parse-only passes, validate the next compile/codegen stage for
  `tests/cpp/std/std_vector_simple.cpp`
- compare against Clang IR when codegen behavior becomes relevant
- add only the minimal extra tests needed to guard the final passing state

Completion check:
- `tests/cpp/std/std_vector_simple.cpp` reaches the intended stable passing
  state with regression coverage
