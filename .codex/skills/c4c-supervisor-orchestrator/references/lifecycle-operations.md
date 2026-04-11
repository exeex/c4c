# Supervisor Lifecycle Operations

Use this reference when the repo is not already in a clean active execution
state.

## State Detection

1. If `plan.md` and `todo.md` both exist and all tracked items are complete,
   close the active plan.
2. If `plan.md` exists, stay in execution mode.
3. If neither `plan.md` nor `todo.md` exists and `ideas/open/` has candidates,
   activate the next idea.
4. If neither exists and `ideas/open/` is empty, return `WAIT_FOR_NEW_IDEA`.
5. If only one of `plan.md` or `todo.md` exists, repair the inconsistent state
   before implementation work.

## Activate

1. Choose one source idea from `ideas/open/`.
2. If filenames use numeric prefixes, default to the lowest eligible number
   unless the user overrides priority.
3. Rewrite `plan.md` as an execution runbook linked to that idea.
4. Create or reset `todo.md` with:
   - `Status: Active`
   - `Source Idea: ...`
   - `Source Plan: plan.md`
5. Do not edit implementation code during activation unless the user explicitly
   asks, and even then prefer issuing the first worker packet rather than doing
   code edits as supervisor.

## Generate Runbook

When deriving `plan.md` from an idea:

1. Preserve technical intent, priorities, helper names, and non-goals.
2. Convert proposal sections into ordered execution steps.
3. Add completion checks after each major step.
4. Write for an implementation agent, not for open-ended design review.
5. Keep the active runbook traceable to exactly one source idea.

## Deactivate

1. Read `plan.md` and `todo.md`.
2. Merge completed slices, blockers, and key constraints back into the source
   idea.
3. Remove or replace `plan.md`.
4. Remove or replace `todo.md`.
5. Do not discard execution knowledge that only exists in `todo.md`.

## Switch

1. Deactivate the old plan cleanly.
2. Preserve old execution knowledge in the old source idea.
3. Activate the new source idea into `plan.md`.
4. Reset `todo.md` to the new active plan.

## Close

1. Confirm the implementation, `plan.md`, and `todo.md` actually agree.
2. Update the linked open idea with completion notes and leftover issues.
3. Delete `todo.md`.
4. Delete `plan.md`.
5. Move the idea from `ideas/open/` to `ideas/closed/`.
