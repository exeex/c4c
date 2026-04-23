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

Think of them as layered artifacts:

- `ideas/open/*.md` is durable source intent
- `plan.md` is the active transcription of that intent into execution order
- `todo.md` is the mutable execution-state scratchpad

## Lifecycle Model

The standard workflow is:

1. human discussion produces or updates one `ideas/open/*.md`
2. one idea is activated into [`plan.md`](/workspaces/c4c/plan.md)
3. the agent implements from [`plan.md`](/workspaces/c4c/plan.md) and tracks progress in [`todo.md`](/workspaces/c4c/todo.md)
4. when the source idea itself is complete, close both the active runbook and
   the linked idea

The active runbook can be exhausted, blocked, or retired without the linked
source idea being complete. Do not treat runbook exhaustion as automatic idea
closure.

If work is interrupted by a more important idea:

1. deactivate the current [`plan.md`](/workspaces/c4c/plan.md)
2. fold only durable summary, leftover issues, and true source-intent updates
   back into the linked file under `ideas/open/`
3. activate a different idea into the new [`plan.md`](/workspaces/c4c/plan.md)

## Required Invariants

Always preserve these invariants:

1. there is at most one active plan, represented by the pair [`plan.md`](/workspaces/c4c/plan.md) and [`todo.md`](/workspaces/c4c/todo.md)
2. every active [`plan.md`](/workspaces/c4c/plan.md) must name exactly one source idea under `ideas/open/`
3. [`todo.md`](/workspaces/c4c/todo.md) belongs only to the active [`plan.md`](/workspaces/c4c/plan.md)
4. durable design intent lives in `ideas/open/*.md`, not only in [`todo.md`](/workspaces/c4c/todo.md)
5. implementation progress lives in [`todo.md`](/workspaces/c4c/todo.md), not only in commit history
6. source ideas should change rarely; execution churn should normally stop at
   [`todo.md`](/workspaces/c4c/todo.md) or [`plan.md`](/workspaces/c4c/plan.md)
7. lifecycle state is primarily determined by file existence:
   - neither `plan.md` nor `todo.md` exists: no active plan
   - both exist: active plan exists
   - only one exists: inconsistent state that must be repaired first

## Mutation Priority

When new information appears during execution, apply this ladder in order:

1. update [`todo.md`](/workspaces/c4c/todo.md) for current packet state,
   progress notes, blockers, proof commands, and temporary sequencing
2. update [`plan.md`](/workspaces/c4c/plan.md) only when the active runbook
   contract, ordering, or proof expectations changed within the same source
   idea
3. update the linked `ideas/open/*.md` only when one of these is true:
   - the human changed source intent
   - deactivation or closure needs a compact durable summary
   - a reviewer or lifecycle repair proves the source idea itself is wrong
   - a separate initiative must be recorded under `ideas/open/`

If a change fits at a lower layer, do not promote it upward.

Routine executor progress should normally stop at
[`todo.md`](/workspaces/c4c/todo.md). `plan.md` should act like a route
checkpoint, not a per-packet scratch file.

## Required Metadata

### `plan.md`

An active runbook should declare near the top:

- `Status: Active`
- `Source Idea: ideas/open/<name>.md`
- optional `Supersedes:` or `Activated from:` notes when relevant

### `todo.md`

Execution state should declare near the top:

- `Status: Active`
- `Source Idea Path: ideas/open/<name>.md`
- `Source Plan Path: plan.md`
- `Current Step ID: <step number from plan.md, or none>`
- `Current Step Title: <step label from plan.md, or none>`
- optional reminder lines emitted only when a hook-managed limit is hit:
  `你該做code review了` and/or `你該做baseline sanity check了`

When lifecycle work creates or resets [`todo.md`](/workspaces/c4c/todo.md), use
an executor-compatible skeleton instead of a custom format. The mutable packet
body should be framed with:

- `# Current Packet`
- `## Just Finished`
- `## Suggested Next`
- `## Watchouts`
- `## Proof`

`## Just Finished` should be written as a short overwrite-style summary of the
latest completed packet, and should identify which `plan.md` step it advanced.

`Current Step ID` and `Current Step Title` are the supervisor-facing pointer to
the active runbook step.
Canonical machine state may live in a local ignored file maintained by repo
scripts. `todo.md` should mirror only the active step pointer plus reminder
lines when a review or baseline limit has actually been hit. The review limit
itself should be taken from local script-managed state rather than hard-coded
in skill text.

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
8. when doing so, write only metadata plus an executor-compatible skeleton;
   routine packet content belongs to later executor updates

### Execute Plan

Use when implementing from the current active runbook.

Do this:

1. treat [`plan.md`](/workspaces/c4c/plan.md) as the contract
2. treat [`todo.md`](/workspaces/c4c/todo.md) as the mutable execution state
3. let routine packet completion update [`todo.md`](/workspaces/c4c/todo.md)
   without requiring a plan-owner rewrite
4. if new ideas are discovered, do not silently absorb them into the active plan
5. either:
  - record them in [`todo.md`](/workspaces/c4c/todo.md) if they are execution
     notes for the active packet
   - update [`plan.md`](/workspaces/c4c/plan.md) if the active runbook needs a
     clearer contract or stronger proof
   - create a separate file in `ideas/open/` and request a lifecycle
     transition if the work is a distinct initiative

Do not continuously mirror execution churn back into the source idea.

Do not rewrite [`plan.md`](/workspaces/c4c/plan.md) after every packet. Normal
execution should be able to accumulate several implementation commits, roughly
5 to 10, between real plan checkpoints unless a blocker or route reset forces
an earlier rewrite.

### Deactivate Plan

Use when the active runbook should stop being the current execution target without being declared complete.

Do this:

1. read [`plan.md`](/workspaces/c4c/plan.md) and [`todo.md`](/workspaces/c4c/todo.md)
2. distill only the durable summary, leftover issues, and true source-intent
   updates back into the source file in `ideas/open/`
3. remove or replace [`plan.md`](/workspaces/c4c/plan.md)
4. remove or replace [`todo.md`](/workspaces/c4c/todo.md)

### Switch Active Plan

Use when deactivating one idea and activating another in one task.

Do this in order:

1. deactivate the old active plan
2. preserve relevant execution knowledge back into the old source idea
3. activate the new source idea into [`plan.md`](/workspaces/c4c/plan.md)
4. reset [`todo.md`](/workspaces/c4c/todo.md) to the new plan using metadata
   plus an executor-compatible skeleton, not a separate plan-owner packet
   format

### Close Plan

Use when the active plan is complete and the source idea itself is complete.

Do this:

1. verify the runbook is actually complete
2. verify the source idea is actually complete, not merely that the current
   route is exhausted
3. update the source file in `ideas/open/` only enough to mark it complete and
   add durable leftover-issue notes if needed
4. delete [`todo.md`](/workspaces/c4c/todo.md)
5. delete [`plan.md`](/workspaces/c4c/plan.md)
6. move the updated source file from `ideas/open/` to `ideas/closed/`

## Decision Rules

When new work appears during execution:

- If it is required to finish the current active runbook, keep it within
  [`todo.md`](/workspaces/c4c/todo.md) or [`plan.md`](/workspaces/c4c/plan.md),
  not the source idea.
- If the current runbook is exhausted but the source idea still has durable
  remaining scope, deactivate or rewrite the runbook; do not close the idea.
- If it is adjacent but not required, prefer a new file under `ideas/open/`
  over mutating the linked source idea.
- If it is more important than the current active runbook, perform a deactivation/switch instead of mutating the plan ad hoc.

## Quality Bar

Before finishing any lifecycle operation, check:

- Can a new agent tell which idea is active?
- Can it tell where execution state lives?
- Can it tell whether the current plan is active, parked, or closed?
- If switched, was knowledge preserved back into the old idea?
- Did you avoid promoting `todo.md` or `plan.md` churn into the source idea?
