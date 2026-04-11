# Supervisor Execution Control

Use this reference when `plan.md` is active and implementation work is in
flight.

## Execution Model

The supervisor owns planning, delegation, review, lifecycle state, broad
validation, and final commit authority. Workers execute bounded packets and
return results.

The supervisor does not edit implementation code. Code changes belong to
workers. Supervisor-local work is limited to `plan.md`/`todo.md`, worker packet
construction, test execution, `git diff` review, acceptance or rejection
decisions, and final commit assembly.

Only the supervisor decides whether a commit should be created, when to create
it, and which worker changes belong in it.

## Required Flow

1. Read `plan.md`.
2. Read `todo.md`, or create it from `plan.md` if missing.
3. Confirm `plan.md` and `todo.md` point to the same source idea.
4. Choose the highest-priority incomplete slice in `todo.md`.
5. Translate that slice into one bounded worker packet.
6. Delegate the code-changing slice to a worker instead of editing locally.
7. When the worker returns, inspect both the handoff and `git diff` before
   choosing the next action.
8. Update `todo.md` when the target, blocker, or next slice changes.

## Guardrails

1. Do not let the worker browse `todo.md` to choose work.
2. Do not widen into issue triage unless the issue blocks the current slice.
3. Do not silently absorb a separate initiative into the active plan.
4. Treat direct-call tests, spacing regressions, and similar guardrails as proof
   artifacts, not as the default queue.
5. Do not patch production or test implementation files as supervisor.
6. Do not accept a worker result without reviewing the actual diff.

## Validation Responsibility

For a meaningful implementation slice:

1. Record the full-suite baseline.
2. Require a targeted proof test for the active slice.
3. Compare against Clang or other stated behavioral references when needed.
4. Re-run targeted validation after the change.
5. Run broad validation before final handoff or commit.
6. Enforce monotonic full-suite results when the slice requires the regression
   guard.

Use `c4c-regression-guard` when comparing before/after logs.

## Commit Responsibility

Before committing:

1. Update `todo.md` to match reality.
2. Ensure the commit captures one coherent plan slice.
3. Do not bundle unrelated fixes discovered during worker execution.
4. Treat worker output as candidate content; the supervisor decides whether it
   is commit-ready.
