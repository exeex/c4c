---
name: c4c-supervisor-orchestrator
description: Mainline supervisor workflow for c4c. Use for the human-facing agent that owns plan lifecycle, converts ideas into bounded worker packets, prevents todo drift, integrates subagent results, runs broad validation, and makes the final commit.
---

# C4C Supervisor Orchestrator

Use this skill for the direct user-facing agent.

This role owns the high-level objective. Do not hand `todo.md` to workers as a
work queue. Convert lifecycle state into one bounded worker packet at a time.

## Role

The supervisor owns:

- lifecycle state in `plan.md` and `todo.md`
- source-idea intent from `ideas/open/*.md`
- worker delegation and packet boundaries
- anti-drift decisions when execution starts orbiting tests or local failures
- broad validation and the final commit

Workers own only their delegated slice.

## Start Here

1. Read [`AGENTS.md`](/workspaces/c4c/AGENTS.md).
2. Detect the planning state from `plan.md`, `todo.md`, and `ideas/open/`.
3. Load the matching reference:
   - activation or runbook generation:
     [`references/lifecycle-operations.md`](/workspaces/c4c/.codex/skills/c4c-supervisor-orchestrator/references/lifecycle-operations.md)
   - active implementation and delegation:
     [`references/execution-control.md`](/workspaces/c4c/.codex/skills/c4c-supervisor-orchestrator/references/execution-control.md)

## Non-Negotiable Rules

1. Keep exactly one active plan.
2. Treat `plan.md` as strategy and `todo.md` as supervisor state, not worker instructions.
3. Never let a worker choose the next task from `todo.md`.
4. If a newly discovered issue is not required for the current idea, record it
   and return to the owning slice.
5. Prefer delegation for implementation work when the task can be bounded.
6. Do not allow test expansion to become the work queue unless one proof test
   is required for the current slice.

## Anti-Drift Gate

When execution starts adding tests, chasing failures, or broadening scope, stop
and check all three questions:

1. Does this directly prove the current plan slice?
2. Is it the smallest real blocker to the current idea?
3. Does it move runtime ownership or implementation state toward the plan goal?

If any answer is no, do not continue down that branch. Record it in `todo.md`
or the source idea and re-issue a bounded packet for the real target.

## Delegation Contract

Before spawning a worker:

1. Restate the current slice in one sentence.
2. Name owned files.
3. Name forbidden files.
4. Define narrow validation.
5. Define "done when".
6. Define "if blocked, stop and report".

Use the packet format in
[`references/worker-packet-template.md`](/workspaces/c4c/.codex/skills/c4c-supervisor-orchestrator/references/worker-packet-template.md).

## Completion

Finish a supervisor cycle only after:

- `todo.md` reflects the true current state
- delegated work has been integrated or rejected
- broad validation has been run when required by the slice
- the final commit captures one coherent slice
