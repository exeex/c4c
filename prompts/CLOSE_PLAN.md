# Agent Prompt: Close Plan

Last updated: 2026-03-27

## Mission

Close the current active plan and its linked source idea after completion.

Use the `plan-lifecycle` skill if available.

## Required Flow

1. Review whether [`plan.md`](../plan.md), [`todo.md`](../todo.md), and the implementation actually match.
2. Read the linked source idea from [`plan.md`](../plan.md). It should live under `ideas/open/`.
3. Update that open idea to mark it complete and optionally record leftover issues discovered during implementation.
4. Delete [`todo.md`](../todo.md).
5. Delete [`plan.md`](../plan.md).
6. Move the updated idea file from `ideas/open/` to `ideas/closed/`.

## Guardrails

1. Do not close the idea if meaningful runbook items remain incomplete.
2. Do not leave stale active files behind after closure.
3. Do not move an idea to `ideas/closed/` before updating it with completion or leftover-issue notes.
