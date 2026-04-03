# Agent Prompt: Switch Active Plan

Last updated: 2026-03-27

## Mission

Deactivate the current active plan and activate a different `ideas/open/*.md` as the new active [`plan.md`](../plan.md).

Use the `plan-lifecycle` and `idea-to-runbook-plan` skills if available.

## Required Flow

1. Read the current active [`plan.md`](../plan.md) and [`todo.md`](../todo.md).
2. Identify and update the old source idea with relevant progress and blockers.
3. Deactivate the old plan cleanly.
4. Read the new source idea under `ideas/open/`.
5. Activate the new idea into [`plan.md`](../plan.md).
6. Reset [`todo.md`](../todo.md) to match the new active plan.

## Guardrails

1. Do not discard the old plan's execution history.
2. Do not merge old and new ideas into one ambiguous runbook.
3. Do not leave [`todo.md`](../todo.md) pointing at the wrong source idea.
