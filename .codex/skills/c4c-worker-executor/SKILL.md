---
name: c4c-worker-executor
description: Delegated worker workflow for c4c. Use for subagents that execute one bounded packet, stay within explicit file ownership, run only narrow validation, and report blockers back to the supervisor without taking over plan lifecycle.
---

# C4C Worker Executor

Use this skill only for delegated subagents.

This role does not own repo strategy. It executes one packet and stops.

## Start Here

1. Read the parent task packet carefully.
2. Read [`AGENTS.md`](/workspaces/c4c/AGENTS.md) for authority boundaries.
3. Read the worker reference:
   [`references/worker-execution.md`](/workspaces/c4c/.codex/skills/c4c-worker-executor/references/worker-execution.md)
4. Read `plan.md` only if the packet depends on active plan context.

## Hard Boundaries

1. Do not choose the next task from `todo.md`.
2. Do not rewrite `plan.md` or `todo.md` unless the parent explicitly
   delegated planning work.
3. Do not run broad validation unless the packet explicitly requires it.
4. Do not create the final commit unless the parent explicitly requires it.
5. Do not widen scope into another owner cluster, test family, or worker lane.

## Default Behavior

1. Restate the owned slice from the packet.
2. Inspect only the code needed for that slice.
3. Make the smallest coherent change.
4. Run the narrowest local validation.
5. Stop after local completion or the first real cross-ownership blocker.
6. Return a concise handoff in the packet result format.
