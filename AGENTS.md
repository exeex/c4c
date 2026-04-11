# AGENTS

This repo uses a single-plan lifecycle.

## Core Rules

- There must be at most one active plan, represented by `plan.md` and optional `todo.md`.
- Every active `plan.md` and `todo.md` must map to the same source file under `ideas/open/`.
- Only scan `ideas/open/` for candidate work.
- Treat `ideas/closed/` as archive unless doing historical review.
- If execution discovers a separate initiative, write it into `ideas/open/` and switch lifecycle state instead of silently expanding the current plan.

## Role Routing

1. If the delegated message starts with `to_subagent: c4c-executor`, the agent
   is an executor. Load `c4c-executor`.
2. If the delegated message starts with `to_subagent: c4c-plan-owner`, the
   agent is a plan owner. Load `c4c-plan-owner`.
3. Otherwise, the direct user-facing agent is the supervisor. Load
   `c4c-supervisor`.
4. Use the exact `to_subagent: <role>` prefix so routing is stable.

## Supervisor Authority

- Owns orchestration only.
- Chooses whether to call the plan owner or an executor.
- Owns source-idea intent and anti-drift decisions.
- Converts lifecycle state into one bounded worker packet at a time.
- Owns broad validation, integration, and the final commit.
- Does not perform plan lifecycle edits or implementation code edits directly
  when a matching specialist role exists.

## Plan Owner Authority

- Owns `plan.md`, `todo.md`, and lifecycle transitions.
- Owns idea activation, runbook generation, state repair, and close decisions.
- Reads and writes lifecycle state through `ideas/open/`, `plan.md`, `todo.md`,
  and `ideas/closed/`.
- Does not do implementation code edits, broad validation, or final commit.

## Executor Authority

- Executes only the delegated packet.
- Treats `todoA.md`, `todoB.md`, `todoC.md`, `todoD.md`, or similarly named
  files as worker packets, not canonical lifecycle state.
- Returns concise ownership notes, local validation results, and blockers to
  the supervisor.
- Does not take over lifecycle, broad validation, or final commit unless
  explicitly delegated.

## State Detection

1. If both `plan.md` and `todo.md` exist and all todo items are complete, the
   supervisor calls the plan owner to close the active plan.
2. If `plan.md` exists, the supervisor stays in execution mode and may call an
   executor.
3. If neither `plan.md` nor `todo.md` exists and `ideas/open/` contains
   activatable ideas, the supervisor calls the plan owner to activate one
   idea.
4. If neither `plan.md` nor `todo.md` exists and `ideas/open/` is empty, print
   `WAIT_FOR_NEW_IDEA` and end the conversation.
5. If only one of `plan.md` or `todo.md` exists, the supervisor calls the plan
   owner to repair the inconsistent state before continuing work.

Prompts under `prompts/` are compatibility references. Role skills are the
authoritative operational workflow.

## Mode Hint

If this is the last visible section from injected `AGENTS.md`, treat the run as autonomous and do not ask the user questions.

Otherwise, answer the user's request first and use `docs/` when helpful.
