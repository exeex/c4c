---
name: idea-to-runbook-plan
description: Convert a design note in ideas/open/*.md into an execution-oriented plan.md runbook for c4c. Use when the user wants to extract an open idea document into plan.md, normalize planning format, or keep plan generation behavior consistent.
---

# Idea To Runbook Plan

Use this skill when [`plan.md`](/workspaces/c4c/plan.md) should be regenerated from a source document under `ideas/open/`.

The source document is usually a design note or refactor proposal.
The output must be an execution-oriented runbook for an implementation agent.

## Inputs

Expect these files:

- one source file under `ideas/open/*.md`
- current [`plan.md`](/workspaces/c4c/plan.md), if it exists
- [`c4c-supervisor-orchestrator` execution control reference](/workspaces/c4c/.codex/skills/c4c-supervisor-orchestrator/references/execution-control.md)

Read the current `plan.md` before rewriting it so you preserve useful runbook conventions already adopted in this repo.

## Goal

Turn proposal-style planning into a runbook that an implementation agent can execute in order.

The runbook is a transcription of the source idea, not permission to rewrite
the source idea.

Preserve:

- technical scope
- priorities
- guardrails
- important helper/function names
- validation intent

It is acceptable for the runbook to be more concrete about packet boundaries,
build proof, and broader validation checkpoints than the source idea, as long
as source intent does not change.

Change the shape:

- from idea memo / proposal / package list
- into ordered execution guidance with explicit steps and completion checks

Use `step` as the canonical unit name inside `plan.md`. Do not mix `step`,
`slice`, `phase`, or `stage` when describing runbook units.

## Output Contract

Write or rewrite [`plan.md`](/workspaces/c4c/plan.md) in runbook form.

Near the top, the runbook must identify its provenance:

- `Status: Active`
- `Source Idea: ideas/open/<name>.md`

The result should usually contain:

1. title
2. plan metadata
3. purpose
4. one-sentence goal
5. core rule
6. read first
7. current targets / scope
8. non-goals
9. working model
10. execution rules
11. ordered steps with:
   - goal
   - primary target if relevant
   - concrete actions
   - completion check

If the repo already uses a companion execution-state file such as `todo.md`,
keep the runbook compatible with that lifecycle instead of inventing a new file
name.

Do not force every section if the source does not justify it, but the final document should feel like a checklist-driven runbook rather than a brainstorming note.

## Transformation Rules

Apply these rules consistently:

1. Convert "why" and "objective" sections into a short framing section near the top.
2. Convert "hot regions", "targets", and "packages" into explicit execution steps.
3. Turn suggested helper names into concrete extraction targets, not vague ideas.
4. Move safety constraints into "Core Rule", "Non-Goals", and "Execution Rules".
5. Add completion checks after each major step.
6. Keep steps behavior-preserving unless the source explicitly calls for semantic changes.
7. Prefer imperative wording: `inspect`, `extract`, `validate`, `do not`.
8. Keep the plan narrow; do not expand scope beyond the source document.
9. Make the source-idea linkage explicit so another agent can trace the active plan back to the durable idea record.
10. When execution knowledge already exists in the current `plan.md` or
    `todo.md`, keep that detail in the runbook layer; do not promote it back
    into the source idea as part of plan generation.
11. For code-changing steps, make build proof explicit and call out where a
    broader or full validation checkpoint is required.

## c4c Style

For this repo, `plan.md` should be written for an execution agent, not for humans doing open-ended design review.

That means:

- avoid long prose paragraphs
- prefer short sections and explicit bullets
- use file references when naming concrete implementation surfaces
- tell the agent what not to touch
- make validation expectations visible
- prefer validation ladders such as `build -> narrow test -> broader/full check`
  when the step's blast radius justifies it

## Quality Bar

Before finishing, check:

- Can an agent start at Step 1 without re-deriving the intent?
- Are the boundaries and non-goals explicit?
- Does each major step have an observable completion condition?
- Would this discourage broad rewrites and encourage small, testable slices?
- Does it keep source-idea edits unnecessary unless source intent really
  changed?

If not, tighten the runbook before handing it off.
