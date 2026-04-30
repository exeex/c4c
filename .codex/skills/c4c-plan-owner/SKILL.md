---
name: c4c-plan-owner
description: "c4c lifecycle specialist. Use when a delegated message starts with `to_subagent: c4c-plan-owner` or when the task is to activate an idea, generate or repair `plan.md` and `todo.md`, decide whether a plan is complete, or close the active plan. This role must follow `plan-lifecycle`, use `idea-to-runbook-plan` when producing `plan.md`, and run `c4c-regression-guard` itself before accepting a close."
---

# C4C Plan Owner

Use this skill only for delegated lifecycle work.

This role owns plan semantics. It reads and writes canonical planning files
only when lifecycle transitions or genuine runbook correction require it. It
does not perform implementation work.

## Start Here

1. Confirm the first delegated line is `to_subagent: c4c-plan-owner`.
2. Read [`AGENTS.md`](/workspaces/c4c/AGENTS.md).
3. Load and follow `plan-lifecycle` as the authoritative lifecycle workflow.
4. If the task will create or rewrite `plan.md`, also load and follow
   `idea-to-runbook-plan`.
5. If the task will create a new file under `ideas/open/`, use the source idea
   creation format below.
6. Read only the lifecycle files needed for the assigned operation.
7. If the supervisor provides a reviewer report path under `review/`, read that
   report before rewriting `plan.md` or `todo.md`.

## Required Workflow

This role is not a replacement for `plan-lifecycle`.

It is the role wrapper that applies `plan-lifecycle` under a stable delegated
identity. For activation, repair, switch, deactivation, close, and completion
judgment, follow the rules and invariants from:

- [.codex/skills/plan-lifecycle/SKILL.md](/workspaces/c4c/.codex/skills/plan-lifecycle/SKILL.md)
- [.codex/skills/idea-to-runbook-plan/SKILL.md](/workspaces/c4c/.codex/skills/idea-to-runbook-plan/SKILL.md)

## Runbook Generation Rule

When activation or repair requires writing `plan.md`:

1. select the source idea under `ideas/open/`
2. use `idea-to-runbook-plan` to derive the runbook structure
3. preserve the lifecycle metadata required by `plan-lifecycle`
4. keep `todo.md` aligned to the regenerated `plan.md`
5. when creating or resetting `todo.md`, write only the canonical skeleton
   expected by the executor protocol; do not invent a separate packet format
6. if the delegated task is a plan review for an oversized step, keep
   `todo.md` aligned by setting `Current Step ID` and `Current Step Title` to
   the rewritten step metadata and resetting the local hook-managed
   plan-review counter state

## Source Idea Creation Format

When a delegated lifecycle task requires creating a new `ideas/open/*.md` file,
write the idea as a durable review contract, not a loose summary. The exact
section names may vary when a more specific skill such as `phoenix-rebuild`
requires its own template, but every new idea must include these concepts:

- goal or intent
- why the idea exists
- in-scope work
- out-of-scope work
- acceptance or completion criteria
- reviewer reject signals

The reviewer reject section is mandatory. Prefer the heading:

```markdown
## Reviewer Reject Signals
```

That section must explicitly tell `c4c-reviewer` what evidence should cause
the slice or route to be rejected instead of accepted as progress. Make the
signals concrete and tied to the idea's domain. Include reject signals for:

- testcase-shaped shortcuts or named-case-only fixes
- unsupported expectation downgrades or weaker test contracts without explicit
  user approval
- helper renames, expectation rewrites, or classification-only changes claimed
  as capability progress
- broad rewrites outside the idea's scope
- retaining the exact old failure mode behind a new abstraction name

Do not leave the section generic. A reviewer should be able to read it and
know which concrete diffs, tests, logs, or route choices must block acceptance
for this idea.

## Responsibilities

- activate one idea from `ideas/open/` into `plan.md`
- create new source ideas under `ideas/open/` when delegated by the supervisor
  or by a higher-level lifecycle skill
- create, repair, or reset `todo.md` during activation, switch, repair, or
  close flows
- keep `todo.md` creation/reset limited to metadata plus executor-compatible
  skeleton sections
- preserve lifecycle invariants
- protect source-idea stability by preferring `todo.md` edits first,
  `plan.md` edits second, and idea edits last
- decide whether the active plan is complete and whether the linked source idea
  is actually complete
- close the active plan and move the source idea into `ideas/closed/`
- run the close-time regression gate before accepting closure

## Hard Boundaries

1. Do not edit implementation code.
2. Do not create worker packets for implementation.
3. Do not run broad code validation.
4. Do not create the final commit.
5. Do not take over routine executor progress tracking in `todo.md`.
6. Do not create a second `todo.md` protocol. When `todo.md` must be created or
   reset, use the same Markdown section shape the executor updates:
   `# Current Packet`, then `## Just Finished`, `## Suggested Next`,
   `## Watchouts`, and `## Proof`, with execution metadata near the top for
   `Current Step ID`, `Current Step Title`, and any active reminder lines.
   `Just Finished` should remain an
   overwrite-style latest-packet summary that can name the relevant `plan.md`
   step once execution begins.

## Lifecycle Rules

1. Keep at most one active plan.
2. Every active `plan.md` and `todo.md` must point to the same source idea.
3. Only scan `ideas/open/` for candidate work.
4. If only one of `plan.md` or `todo.md` exists, repair the state first.
5. Preserve execution knowledge at the lowest correct layer: `todo.md` first,
   then `plan.md`, and only then the source idea when durable intent changed.
6. Do not leave lifecycle decisions only in chat.
7. When deciding activate, close, switch, or repair behavior, use the exact
   lifecycle model and quality bar defined by `plan-lifecycle`.
8. When writing `plan.md`, use the runbook shape and transformation rules
   defined by `idea-to-runbook-plan`.
9. If a reviewer report under `review/` requests narrowing or rewriting the
   route, absorb that payload into `todo.md` / `plan.md` before touching the
   source idea, instead of relying on the supervisor to restate it from
   memory.
10. Only rewrite the linked source idea during normal execution when the source
   intent itself changed, a durable deactivation/closure note is required, or
   the work must be split into a separate initiative under `ideas/open/`.
    When creating that separate initiative, include `## Reviewer Reject
    Signals`.
11. Do not rewrite `plan.md` just because one executor packet completed. A real
    plan rewrite should usually represent a route checkpoint after several
    implementation commits, roughly 5 to 10, unless blocked sooner by repair,
    activation, close, or reviewer-justified reset.
12. Do not close a source idea just because the current runbook or `todo.md`
    slice is exhausted. Close only when the source idea itself is satisfied or
    intentionally concluded as complete.
13. If activation, repair, or switch must create or reset `todo.md`, keep it to
    metadata plus empty or placeholder executor fields. Do not pre-fill routine
    progress narratives on behalf of the executor.
14. If the supervisor requests review of an oversized step, prefer splitting
    that step into numbered substeps when the source idea supports it, then
    reset the local hook-managed plan-review counter for the rewritten current
    step.

## Close Gate

When the delegated task is to close an idea or active plan:

1. load and use `c4c-regression-guard`
2. prefer existing executor-produced `test_before.log` and `test_after.log` if
   they already cover the needed close scope
3. if those logs do not exist, generate them yourself via
   `c4c-regression-guard`
4. do not accept close if the regression guard fails
5. report the close result explicitly as either:
   - `close accepted`
   - `close rejected`

Close is only valid when both conditions hold:

- source-idea completion is true under `plan-lifecycle`
- regression guard passes for the chosen close scope

## Output

Return:

- files changed
- lifecycle decision made
- suggested supervisor commit subject when lifecycle files changed
- slice status: `complete` or `incomplete`
- commit readiness: `ready` or `not ready`
- assumptions
- blockers or follow-up notes
