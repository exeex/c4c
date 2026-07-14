---
name: c4c-supervisor
description: Scripted and execution c4c orchestrator. Use when scripts/run_agent.sh supplies C4C_RUN_MODE=scripted, or when a user explicitly rejects the user-service idea/run-agent workflow and requests immediate execution inside the Codex extension. Routes lifecycle, blocker, execution, proof, review, and commit work to the authoritative supervisor operation documents.
---

# C4C Supervisor

Confirm that `C4C_RUN_MODE=scripted` is present or that the user explicitly
requested immediate extension execution. Otherwise use `c4c-user-service`.

Before acting, read `AGENTS.md`, run `git status --short`, and inspect the
existence and source linkage of `plan.md`, `todo.md`, and `ideas/open/`.

## Operation Routing

Read [`lifecycle-operations.md`](lifecycle-operations.md) completely when any
of these applies:

- activate, resume, deactivate, switch, repair, close, or conclude an idea
- `plan.md` / `todo.md` are absent, inconsistent, exhausted, or name a
  different source
- required work exceeds the active idea's scope
- an active step is blocked and may require a separate idea
- no active plan exists or `WAIT_FOR_NEW_IDEA` is being considered

Read [`execution-control.md`](execution-control.md) completely when any of
these applies:

- select or delegate an implementation packet
- choose build/test proof or manage canonical regression logs
- review executor output, code, reminders, or commit readiness
- invoke a gated reviewer
- create the supervisor-owned commit

Read both documents when execution discovers a lifecycle blocker.

Delegate lifecycle mutations to `c4c-plan-owner` and implementation mutations
to `c4c-executor`. Every delegated message begins with exact first line
`to_subagent: <role>`.
