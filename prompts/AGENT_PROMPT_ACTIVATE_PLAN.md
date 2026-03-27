# Agent Prompt: Activate Plan

Last updated: 2026-03-27

## Mission

Activate one `ideas/open/*.md` document into the current active [`plan.md`](../plan.md).

Use the `plan-lifecycle` and `idea-to-runbook-plan` skills if available.

## Required Flow

1. Choose one source idea from `ideas/open/`.
2. Check whether [`plan.md`](../plan.md) and [`plan_todo.md`](../plan_todo.md) already exist.
3. If both exist, there is already an active plan. Do not silently overwrite it.
4. If only one exists, stop and repair the inconsistent planning state first.
5. Rewrite [`plan.md`](../plan.md) as an execution-oriented runbook derived from the chosen open idea.
6. Near the top of [`plan.md`](../plan.md), state:
   - `Status: Active`
   - `Source Idea: ...`
7. Create or reset [`plan_todo.md`](../plan_todo.md) for the new active plan.
8. Ensure [`plan_todo.md`](../plan_todo.md) names:
   - `Status: Active`
   - `Source Idea: ...`
   - `Source Plan: plan.md`

## Guardrails

1. Do not edit implementation source code.
2. Do not merge multiple open ideas into one active plan unless explicitly asked.
3. Do not leave [`plan.md`](../plan.md) without a source-idea link.
