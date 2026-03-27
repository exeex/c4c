# Agent Prompt: Plan Generation From Idea

Last updated: 2026-03-27

## Mission

Convert one `ideas/open/*.md` design document into an execution-oriented [`plan.md`](../plan.md).
The result must be a runbook for an implementation agent, not a brainstorming memo.
The runbook must explicitly say which idea it was activated from.

If the user names a specific source file, use that file.
If the current task context clearly points to one source file, use it.
If more than one source file is plausible and no source is specified, stop and report the ambiguity instead of guessing.

Use the `plan-lifecycle` and `idea-to-runbook-plan` skills if available.

## Required Inputs

Before editing:

1. read the chosen source file under `ideas/open/`
2. read the current [`plan.md`](../plan.md) if it exists
3. read [`AGENT_PROMPT_EXECUTE_PLAN.md`](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)

## Required Output

Rewrite [`plan.md`](../plan.md) as a stable runbook with:

1. `Status: Active` near the top
2. `Source Idea: ideas/open/<name>.md` near the top
3. a clear purpose and one-sentence goal
4. explicit scope and non-goals
5. concrete target functions/files when relevant
6. ordered execution steps
7. completion checks after each major step
8. language that encourages small, test-backed slices instead of broad rewrites

## Transformation Rules

1. Preserve the source document's technical intent and priority.
2. Convert proposal-style sections into action-oriented runbook sections.
3. Turn "packages", "phases", or "hot regions" into an ordered execution sequence.
4. Preserve helper/function names that define the intended refactor surface.
5. Move warnings and constraints into explicit guardrails.
6. Do not invent new milestones that are not justified by the source file.
7. Prefer concise imperative wording over explanatory prose.
8. Ensure another agent can tell exactly which idea owns the active plan.

## Guardrails

1. Do not edit source code as part of this task unless the user explicitly asks.
2. Do not update `plan_todo.md` as part of this task unless the user explicitly asks.
3. Do not leave `plan.md` as a high-level proposal; it must read like a runnable checklist.
4. Do not silently merge multiple open idea documents into one plan unless the user explicitly asks.

## Completion Criteria

Finish only when:

1. [`plan.md`](../plan.md) has been rewritten in runbook form
2. the source idea document remains the authoritative scope source
3. the final structure is concrete enough that [`AGENT_PROMPT_EXECUTE_PLAN.md`](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md) can execute from it directly
4. the source-idea linkage is visible near the top of [`plan.md`](../plan.md)
