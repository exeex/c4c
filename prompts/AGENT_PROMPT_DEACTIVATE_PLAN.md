# Agent Prompt: Deactivate Plan

Last updated: 2026-03-27

## Mission

Deactivate the current active [`plan.md`](../plan.md) without declaring it complete.

Use the `plan-lifecycle` skill if available.

## Required Flow

1. Read [`plan.md`](../plan.md) and [`plan_todo.md`](../plan_todo.md).
2. Identify the linked source idea from [`plan.md`](../plan.md).
3. Fold important execution knowledge back into the source idea:
   - completed slices
   - blockers
   - important constraints
   - discovered follow-up ideas
4. Mark the source idea as inactive or parked.
5. Replace [`plan.md`](../plan.md) with a non-active placeholder or the next activated plan if instructed.
6. Clear or replace [`plan_todo.md`](../plan_todo.md) so stale active-state does not remain.

## Guardrails

1. Do not discard execution knowledge that exists only in [`plan_todo.md`](../plan_todo.md).
2. Do not leave [`plan.md`](../plan.md) appearing active after deactivation.
