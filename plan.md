# Backend Port Roadmap Runbook

Status: Active
Source Idea: ideas/open/__backend_port_plan.md
Activated from: umbrella roadmap after closing the bounded x86 linker plan

## Purpose

Re-establish a narrow executable backend slice from the umbrella roadmap so
implementation can continue from a bounded child idea instead of from the
umbrella file directly.

## Goal

Identify the next highest-value backend slice still missing from
`ideas/open/`, write it as a bounded child idea, and switch execution back to
that narrower plan before resuming implementation work.

## Core Rule

Do not implement backend code directly from this umbrella runbook; use it only
to restore a concrete child-idea work queue that can then be activated on its
own.

## Read First

- [ideas/open/__backend_port_plan.md](/workspaces/c4c/ideas/open/__backend_port_plan.md)
- [ideas/closed/24_backend_builtin_linker_x86_plan.md](/workspaces/c4c/ideas/closed/24_backend_builtin_linker_x86_plan.md)
- [prompts/AGENT_PROMPT_SWITCH_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_SWITCH_PLAN.md)
- [prompts/AGENT_PROMPT_PLAN_FROM_IDEA.md](/workspaces/c4c/prompts/AGENT_PROMPT_PLAN_FROM_IDEA.md)

## Current Targets

- `ideas/open/__backend_port_plan.md`
- `ideas/open/`
- `ideas/closed/`
- backend roadmap continuity after the first built-in x86 linker closure

## Non-Goals

- direct backend implementation from the umbrella roadmap
- mixing multiple backend initiatives into one child idea
- reopening the closed x86 linker slice unless a concrete regression requires it

## Working Model

1. Inspect the umbrella roadmap against the current open and closed idea
   inventory.
2. Determine the next bounded backend slice that should become a new child idea.
3. Write that child idea under `ideas/open/` with clear scope, guardrails, and
   validation targets.
4. Switch the active lifecycle state from this umbrella runbook to the new
   child idea before any implementation continues.

## Execution Rules

- Preserve the roadmap's mechanical-porting strategy and target isolation.
- Prefer one mechanism family per new child idea.
- If multiple candidate slices exist, choose the smallest one that restores a
  sensible implementation queue.
- Keep durable planning intent in `ideas/open/*.md`, not only in `plan_todo.md`.

## Ordered Steps

### Step 1: Audit the remaining backend roadmap queue

Goal: identify what work remains actionable now that the bounded x86 linker
slice is closed.

Primary target: `ideas/open/__backend_port_plan.md` and current idea inventory.

Concrete actions:

- compare the umbrella roadmap's listed child ideas against files currently
  present under `ideas/open/` and `ideas/closed/`
- note which roadmap entries are already complete and which work families still
  lack an open child idea
- identify the narrowest viable next slice that matches the roadmap guardrails

Completion check:

- there is one explicit candidate slice for the next child idea

### Step 2: Write the next bounded child idea

Goal: turn the selected remaining slice into a durable open idea document.

Primary target: `ideas/open/<new-child>.md`

Concrete actions:

- write the new child idea with goal, scope, non-goals, suggested execution
  order, and validation intent
- keep the idea narrow enough to activate without further decomposition
- link it back to `ideas/open/__backend_port_plan.md` as the umbrella source

Completion check:

- one bounded backend child idea exists under `ideas/open/`

### Step 3: Switch lifecycle state to the new child idea

Goal: stop treating the umbrella roadmap as the active execution target.

Primary target: `plan.md` and `plan_todo.md`

Concrete actions:

- fold any roadmap-audit notes that should persist back into
  `ideas/open/__backend_port_plan.md`
- activate the new child idea into a fresh runbook
- reset `plan_todo.md` so implementation resumes from the child idea, not the
  umbrella roadmap

Completion check:

- the active plan points at the new child idea rather than the umbrella file
