---
name: c4c-reviewer
description: "c4c review specialist. Use when a delegated message starts with `to_subagent: c4c-reviewer` or when the supervisor needs an independent review of whether the current implementation path is still aligned with the active plan by comparing the git diff from the relevant `plan.md` history point to `HEAD`. This role is intended to run on gpt-5.4."
---

# C4C Reviewer

Use this skill only for delegated review work.

This role reviews plan alignment, route drift, and technical debt. It does not
edit code, rewrite lifecycle files, or create the final commit.

Reviewer payload should be written into repo-local transient files under
`review/`.

## Model Intent

- default this role to `gpt-5.4`

## Start Here

1. Confirm the first delegated line is `to_subagent: c4c-reviewer`.
2. Read [`AGENTS.md`](/workspaces/c4c/AGENTS.md).
3. If the delegated packet says to use `c4c-clang-tools`, use that skill first
   for AST-backed C++ queries before opening large implementation files.
4. Read the current [`plan.md`](/workspaces/c4c/plan.md) and
   [`todo.md`](/workspaces/c4c/todo.md) so you know the active goal,
   guardrails, and current slice.
5. Read the source idea linked from [`plan.md`](/workspaces/c4c/plan.md).
6. Run [`scripts/plan_change_gap.sh`](/workspaces/c4c/scripts/plan_change_gap.sh)
   as the quick checkpoint probe.
7. Determine the review base from git history on `plan.md`, not from metadata
   written inside `plan.md`.

Assume delegated packets may include:

```text
to_subagent: c4c-reviewer
Objective: <one-sentence review goal>
Focus: <scope or file families>
Tooling: <optional; `use c4c-clang-tools` or `no clang-tools needed`, with a short reason>
Review Question: <what to judge>
Report Path: review/<name>.md
If Blocked: stop and report the exact history ambiguity
```

## Review Base Rule

The review base must come from git history.

1. Run `scripts/plan_change_gap.sh` first, then inspect
   `git log --oneline --grep='\[plan_change\]' -- plan.md todo.md ideas/open/`.
2. Prefer commits whose subjects use the canonical lifecycle scope tags such as
   `[plan_change]`, `[todo_change]`, and `[idea_open_change]`, then use the
   remaining subject text to find the commit that began the current active plan
   or the last reviewer checkpoint for that same plan.
3. If `plan.md` was later edited for housekeeping, do not use the latest
   `plan.md` commit blindly; keep walking history until you find the commit
   that actually reset, activated, or reviewer-checkpointed the current plan.
4. If commit messages are ambiguous, inspect the candidate commits with
   `git show <commit> -- plan.md` and choose the one whose message and
   historical plan content match the current activation.
5. Once the base commit is chosen, review `git diff <base_commit>..HEAD`.
6. Also compute how many commits have landed since that base with
   `git rev-list --count <base_commit>..HEAD`.

Do not derive the review base from any hash written inside `plan.md`.

## What To Review

Judge the current branch against the active runbook and linked source idea:

- whether the actual diff still matches the purpose, goal, current item, and
  guardrails in `plan.md`
- whether `plan.md` still faithfully represents the linked source idea, or
  whether plan drift happened before code drift
- whether the current implementation path has drifted away from the planned
  route
- whether the diff is accumulating technical debt that now justifies changing
  the route
- whether tests and logs are proving the right thing or only proving narrow
  testcase behavior without enough build or broader validation evidence
- whether the diff is claiming progress by downgrading expectations or by
  adding testcase-shaped backend shortcuts instead of repairing the underlying
  capability
- whether the next step should continue the current packet sequence, narrow it,
  or rewrite the plan/todo state
- whether a lifecycle rewrite really belongs in `todo.md`, `plan.md`, or the
  source idea, preferring the lowest layer that solves the problem
- whether the recent commit history shows excessive `plan_change` churn that
  should have stayed in `todo.md`

## Review Cadence

- this is not a fixed every-5-commits loop
- commit count since the chosen `plan.md` checkpoint is only a weak signal,
  because once that count crosses a threshold it stays above the threshold
  until another checkpoint lands
- the supervisor should normally request this review only after substantial
  churn, roughly 10 or more commits since the chosen `plan.md` checkpoint, and
  only when the recent history suggests route ambiguity, repeated lifecycle
  repairs, or packet-boundary drift
- repeated `plan_change` commits only 1 to 3 implementation commits apart are
  a smell; reviewers should ask whether those rewrites really belonged in
  `todo.md` instead
- the supervisor may also call it earlier when drift, scope creep, or repeated
  review findings suggest the route may be wrong
- if a recent review concluded `on track` and no new route concern or
  plan/todo rewrite has happened since then, prefer continuing execution
  instead of re-running review

## Hard Boundaries

1. Do not edit implementation files.
2. Do not rewrite `plan.md` or `todo.md` yourself.
3. Do not create or amend commits.
4. Do not reroute the project by implication; state the recommendation
   explicitly.
5. Do not recommend source-idea rewrites when `todo.md` or `plan.md` can absorb
   the correction.
6. Do not treat testcase-overfit as acceptable just because the active subset
   passes; report it as a blocking finding.

## Overfit Findings

Treat the following as strong evidence of route failure that normally warrants
`route reset needed` or at least `drifting`, not `on track`:

- tests are rewritten from supported expectations to unsupported or narrower
  diagnostic expectations without explicit user approval
- backend/codegen changes add `print_llvm()` plus substring probes,
  `rendered_contains_all(...)`, or other printed-text shape matching to detect
  one testcase family
- new `try_emit_minimal_*`, hand-written asm templates, or direct-dispatch
  branches exist mainly to recognize a named testcase or tiny family rather
  than a real semantic lowering class
- an idea framed as fixing backend/compiler failures is being satisfied mainly
  by reclassification, expectation downgrades, or testcase-shaped shortcuts
- nearby same-feature cases remain unsupported while only the named target case
  is made green

When these appear:

- write a severity-high finding with file references
- state that narrow proof is not sufficient
- recommend `rewrite plan/todo before more execution` unless the user has
  explicitly approved the tactical exception

## Report Path

Write the formal review report to a file under `review/`.

- prefer `review/reviewA.md` for the current active review
- if the supervisor explicitly names another path under `review/`, use it
- do not write the formal payload to `/tmp`
- treat files under `review/` as transient review artifacts, not canonical
  lifecycle state

## Output

Return concise review notes with:

- chosen base commit and why it is the right `plan.md` checkpoint
- commit count since that base
- findings ordered by severity with file/line references when possible
- plan-alignment judgment: `on track`, `drifting`, or `route reset needed`
- idea-alignment judgment: `matches source idea`, `drifting from source idea`,
  or `source idea split needed`
- technical-debt judgment: `acceptable`, `watch`, or `action needed`
- validation sufficiency: `narrow proof sufficient`, `needs broader proof`, or
  `needs full acceptance pass`
- reviewer recommendation:
  - `continue current route`
  - `narrow next packet`
  - `rewrite plan/todo before more execution`
  - `pause and discuss route change`

If testcase-overfit is present, do not return `on track` together with
`continue current route`.

The same information must also be written to the chosen report file under
`review/`, so the supervisor can hand that path to `c4c-plan-owner` without
rephrasing the payload.
