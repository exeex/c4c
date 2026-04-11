---
name: plan-lifecycle
description: Manage the c4c planning lifecycle across ideas/open/*.md, ideas/closed/*.md, plan.md, and todo.md. Use when activating a plan from an open idea, executing an active plan, deactivating or switching plans, or closing a completed idea.
---

# Plan Lifecycle

Use this skill when the task involves the lifecycle of planning artifacts in this repo.

The repo has four planning locations:

1. `ideas/open/*.md`: open idea inventory
2. `ideas/closed/*.md`: closed archive
3. [`plan.md`](/workspaces/c4c/plan.md): the single active execution runbook
4. [`todo.md`](/workspaces/c4c/todo.md): execution-state scratchpad for the active plan

## Lifecycle Model

The standard workflow is:

1. human discussion produces or updates one `ideas/open/*.md`
2. one idea is activated into [`plan.md`](/workspaces/c4c/plan.md)
3. the agent implements from [`plan.md`](/workspaces/c4c/plan.md) and tracks progress in [`todo.md`](/workspaces/c4c/todo.md)
4. when complete, close both the active runbook and the linked idea

If work is interrupted by a more important idea:

1. deactivate the current [`plan.md`](/workspaces/c4c/plan.md)
2. fold any relevant progress or decisions back into the linked file under `ideas/open/`
3. activate a different idea into the new [`plan.md`](/workspaces/c4c/plan.md)

## Required Invariants

Always preserve these invariants:

1. there is at most one active plan, represented by the pair [`plan.md`](/workspaces/c4c/plan.md) and [`todo.md`](/workspaces/c4c/todo.md)
2. every active [`plan.md`](/workspaces/c4c/plan.md) must name exactly one source idea under `ideas/open/`
3. [`todo.md`](/workspaces/c4c/todo.md) belongs only to the active [`plan.md`](/workspaces/c4c/plan.md)
4. durable design intent lives in `ideas/open/*.md`, not only in [`todo.md`](/workspaces/c4c/todo.md)
5. implementation progress lives in [`todo.md`](/workspaces/c4c/todo.md), not only in commit history
6. lifecycle state is primarily determined by file existence:
   - neither `plan.md` nor `todo.md` exists: no active plan
   - both exist: active plan exists
   - only one exists: inconsistent state that must be repaired first

## Required Metadata

### `plan.md`

An active runbook should declare near the top:

- `Status: Active`
- `Source Idea: ideas/open/<name>.md`
- optional `Supersedes:` or `Activated from:` notes when relevant

### `todo.md`

Execution state should declare near the top:

- `Status: Active`
- `Source Idea: ideas/open/<name>.md`
- `Source Plan: plan.md`

### `ideas/open/*.md` and `ideas/closed/*.md`

When touched as part of lifecycle work, path already carries most status:

- files in `ideas/open/` are open
- files in `ideas/closed/` are closed
- the active open idea is the one linked from [`plan.md`](/workspaces/c4c/plan.md)

Do not force one exact header layout inside idea files, but completion notes and leftover issues should be discoverable.

## Operations

### Activate Plan

Use when one `ideas/open/*.md` should become the current runbook.

Do this:

1. read the source `ideas/open/*.md`
2. check whether [`plan.md`](/workspaces/c4c/plan.md) and [`todo.md`](/workspaces/c4c/todo.md) exist
3. if both exist, another plan is already active
4. if only one exists, repair the inconsistent state first
5. rewrite [`plan.md`](/workspaces/c4c/plan.md) as a runbook derived from the chosen idea
6. include the source-idea link in [`plan.md`](/workspaces/c4c/plan.md)
7. create or reset [`todo.md`](/workspaces/c4c/todo.md) for the new active plan

### Execute Plan

Use when implementing from the current active runbook.

Do this:

1. treat [`plan.md`](/workspaces/c4c/plan.md) as the contract
2. treat [`todo.md`](/workspaces/c4c/todo.md) as the mutable execution state
3. if new ideas are discovered, do not silently absorb them into the active plan
4. either:
   - record them as bounded notes if they are in-scope follow-on work, or
   - create/update a separate file in `ideas/open/` and request a lifecycle transition

### Deactivate Plan

Use when the active runbook should stop being the current execution target without being declared complete.

Do this:

1. read [`plan.md`](/workspaces/c4c/plan.md) and [`todo.md`](/workspaces/c4c/todo.md)
2. merge important progress, constraints, blockers, and discovered sub-ideas back into the source file in `ideas/open/`
3. remove or replace [`plan.md`](/workspaces/c4c/plan.md)
4. remove or replace [`todo.md`](/workspaces/c4c/todo.md)

### Switch Active Plan

Use when deactivating one idea and activating another in one task.

Do this in order:

1. deactivate the old active plan
2. preserve relevant execution knowledge back into the old source idea
3. activate the new source idea into [`plan.md`](/workspaces/c4c/plan.md)
4. reset [`todo.md`](/workspaces/c4c/todo.md) to the new plan

### Close Plan

Use when the active plan is complete.

Do this:

1. verify the runbook is actually complete
2. update the source file in `ideas/open/` to mark it complete and add leftover-issue notes if needed
3. delete [`todo.md`](/workspaces/c4c/todo.md)
4. delete [`plan.md`](/workspaces/c4c/plan.md)
5. move the updated source file from `ideas/open/` to `ideas/closed/`

## Decision Rules

When new work appears during execution:

- If it is required to finish the current active runbook, keep it within the current plan.
- If it is adjacent but not required, record it in the linked open idea or a new file under `ideas/open/`.
- If it is more important than the current active runbook, perform a deactivation/switch instead of mutating the plan ad hoc.

## Quality Bar

Before finishing any lifecycle operation, check:

- Can a new agent tell which idea is active?
- Can it tell where execution state lives?
- Can it tell whether the current plan is active, parked, or closed?
- If switched, was knowledge preserved back into the old idea?
