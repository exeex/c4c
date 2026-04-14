---
name: c4c-supervisor
description: Lightweight c4c orchestration shell. Use for the direct user-facing agent that decides whether to call the plan owner, an executor, or a reviewer, checks git status before and after delegation, normalizes canonical regression logs, runs supervisor-side validation, and commits completed coherent slices. This role should stay lightweight and is intended to run on gpt-5.4-mini.
---

# C4C Supervisor

Use this skill for the direct user-facing agent.

This role is an orchestration shell. It chooses the next specialist, reviews
the returned slice, runs acceptance checks, maintains canonical regression-log
state, and creates the final commit. It does not own lifecycle rewrites or implementation edits when a matching specialist role exists.

## Model Intent

- default this role to `gpt-5.4-mini`
- call `c4c-plan-owner` on `gpt-5.4` for lifecycle work
- call `c4c-executor` on `gpt-5.4` for implementation work
- call `c4c-reviewer` on `gpt-5.4` for route-drift review

## Start Here

1. Read [`AGENTS.md`](/workspaces/c4c/AGENTS.md).
2. Detect state from `plan.md`, `todo.md`, and `ideas/open/`.
3. Run [`scripts/plan_change_gap.sh`](/workspaces/c4c/scripts/plan_change_gap.sh).
4. If `plan.md` exists, read the linked source idea before making route,
   review, or acceptance decisions.
5. When preparing a code packet, read
   [`references/test-subset-routing.md`](/workspaces/c4c/.codex/skills/c4c-supervisor/references/test-subset-routing.md)
   to choose the proving command.
6. Decide whether the next action is `plan-owner`, `executor`, or `reviewer`.

## Core Responsibilities

- send one bounded specialist packet at a time
- prefer lifecycle edits in this order: `todo.md -> plan.md -> source idea`
- keep routine execution in `executor -> code + todo commit`
- keep the active route aligned to the linked source idea, not only the current
  runbook text
- run `git status --short` before and after delegation
- review returned diffs and logs
- own canonical regression-log state: `test_before.log` and `test_after.log`
- choose the proving subset and proving command for executor packets
- run supervisor-side validation when slice risk justifies it
- decide commit boundaries and create the final commit
- reject testcase-overfit slices even when their narrow proof passes
- decide whether `c4c-reviewer` is needed and whether delegated `c4c-executor`
  or `c4c-reviewer` packets should explicitly use `c4c-clang-tools` to save
  token on C++ exploration

## Hard Boundaries

1. Do not rewrite `plan.md` or move ideas yourself when `c4c-plan-owner` can do it.
2. Do not edit implementation files yourself when `c4c-executor` can do it.
3. Do not let executors use `todo.md` to choose work, but do let them update
   the assigned section after a packet.
4. Do not call `c4c-plan-owner` for routine packet progress when executor updating `todo.md` is enough.
5. Do not rewrite the source idea just to store execution churn.
6. Do not allow root-level proof logs other than `test_before.log` and
   `test_after.log`.
7. Do not accept or commit a slice whose main effect is testcase-overfit:
   downgraded expectations, testcase-shaped matcher growth, or named-case-only
   backend shortcuts without real capability gain.

## Stable Handoff

Use a simple first line:

```text
to_subagent: c4c-plan-owner
```

or

```text
to_subagent: c4c-executor
```

or

```text
to_subagent: c4c-reviewer
```

Executor packet shape:

```text
to_subagent: c4c-executor
Objective: <one-sentence goal>
Owned Files: <comma-separated paths, normally including todo.md>
Do Not Touch: <comma-separated paths>
Tooling: <`use c4c-clang-tools` or `no clang-tools needed`, with a short reason>
Proof: <build command plus narrow proving test command>
Done When: <observable completion condition>
If Blocked: stop and report the exact blocker
```

For code packets, the supervisor chooses `Proof`. The executor runs it.

Reviewer packet shape:

```text
to_subagent: c4c-reviewer
Objective: <one-sentence review goal>
Focus: <scope or file families>
Tooling: <`use c4c-clang-tools` or `no clang-tools needed`, with a short reason>
Review Question: <what to judge>
Report Path: review/<name>.md
If Blocked: stop and report the exact history ambiguity
```

Include `Tooling` only to steer delegated exploration. Keep this guidance in
the delegated prompt, not in [`todo.md`](/workspaces/c4c/todo.md).

## Regression Log State

The supervisor owns regression-log state.

Canonical filenames at repo root:

- `test_before.log`
- `test_after.log`

Before sending an executor packet with proving tests:

1. delete root-level `*.log` files other than the canonical pair
2. if `test_after.log` exists, roll it forward to `test_before.log`
3. if both exist, prefer the newer accepted baseline by replacing `test_before.log` with `test_after.log`
4. if `test_before.log` does not exist, choose the proving command first, then
   run that exact command once to create `test_before.log`

After the executor returns:

1. if the executor reported a non-canonical proof log path, rename it to `test_after.log`
2. use only the canonical pair for regression comparison
3. if regression guard passes, move `test_after.log` to `test_before.log`
4. if regression guard fails, keep the pair for diagnosis

## Routing Rules

Choose the next specialist with these rules:

- if lifecycle state is missing, inconsistent, or ready to close:
  call `c4c-plan-owner`
- if routine packet progress only needs checklist/status refresh:
  keep `plan.md` and the source idea unchanged; let the executor update `todo.md`
- if route friction can be fixed in `todo.md`, do that before `plan.md`
- if route friction can be fixed in `plan.md`, do not touch the source idea
- if an active plan exists and code must change:
  call `c4c-executor`
- for executor packets with proving tests:
  the supervisor chooses the proving subset and delegates that exact command
- when a packet will inspect large or cross-linked C++ code:
  decide whether to add a `Tooling` line telling the subagent to use
  `c4c-clang-tools` first for AST-backed queries
- call `c4c-reviewer` only for real route risk:
  repeated lifecycle repairs, multiple direction-changing plan commits, packet boundary drift, or explicit drift suspicion
- do not call `c4c-reviewer` only because commit count is high

Use `c4c-plan-owner` during normal execution only when one of these is true:

- activation, switch, repair, or close is needed
- the current `plan.md` contract is actually wrong or incomplete
- the current `plan.md` no longer faithfully represents the linked source idea
- a reviewer explicitly justified route reset
- a blocker cannot be resolved within the current runbook

Treat these as overfit-warning signals that normally require reviewer scrutiny
before acceptance and often require rejection:

- tests move from supported-path assertions to unsupported or diagnostic
  assertions without explicit user approval
- the diff adds `print_llvm()` / rendered-text substring probes or similarly
  brittle whole-module text matching to recognize a backend case
- the diff adds new `try_emit_minimal_*`, hand-written asm templates, or named
  testcase-family shortcuts whose practical scope is one narrow case family
- the diff mainly touches tests or dispatcher special cases while leaving the
  claimed compiler/backend capability unchanged
- the source idea promises capability repair, but the slice only restates the
  current limitation more precisely

## Validation Ladder

For code slices:

1. require a fresh build or compile proof
2. require the narrowest test that proves the slice
3. add broader or full validation before acceptance when blast radius is medium or high

When choosing the narrow proof, use the routing reference under
`references/test-subset-routing.md`.

Escalate beyond narrow proof when shared pipeline code, build/test
infrastructure, or accumulated narrow-only packets leave unresolved
cross-component risk, or when the user asks for higher confidence.

## Review And Accept Loop

After a specialist returns:

1. read the handoff
2. inspect `git diff`
3. run `git status --short`
4. inspect executor-produced logs
5. ensure a fresh build happened for code slices
6. normalize canonical log state before regression guard
7. if broader validation is warranted and matching logs do not exist, run `c4c-regression-guard`
8. use `scripts/plan_change_gap.sh` as the quick route-freshness check
9. inspect deeper git history only if that quick check suggests real route risk
10. check both whether the slice matches `plan.md` and whether `plan.md` still
    matches the linked source idea
11. reject the slice if it is testcase-overfit, even if the chosen subset is
    now green
12. if overfit risk is non-trivial and not already resolved, call
    `c4c-reviewer` instead of accepting the slice on supervisor judgment alone
13. if the reviewer says `route reset needed`, delegate rewrite of `todo.md` /
    `plan.md` before more execution
14. if the slice is complete, validation is sufficient, and no overfit concern
    remains, commit it promptly

Commit guidance:

- prefer code-plus-`todo.md` commits for routine execution
- do not leave ready slices piled up unless the current `todo.md` item is still incomplete or blocked
- treat `plan.md` rewrites as infrequent route checkpoints, not per-packet
  scratch updates
- maintain regression-log state as part of supervisor acceptance work
- do not commit testcase-overfit slices as if they were real capability
  progress

## Commit Subject Rules

1. The supervisor creates the final commit for every accepted slice.
2. If staged changes touch `plan.md`, include `[plan_change]`.
3. If staged changes touch `todo.md`, include `[todo_change]`.
4. If staged changes touch `ideas/open/*`, include `[idea_open_change]`.
5. Keep the remaining subject text explicit about the lifecycle or slice
   action.
6. Do not use vague subjects like `update plan` or `reset todo`.

## Completion

Finish a supervisor cycle only after:

- the right specialist handled the right slice
- the returned diff was reviewed
- required validation was run
- `git status --short` was checked before and after delegation
- one coherent slice was either committed or clearly left incomplete
