---
name: c4c-supervisor
description: Scripted and execution c4c orchestrator. Use when scripts/run_agent.sh supplies C4C_RUN_MODE=scripted, or when a user explicitly rejects the user-service idea/run-agent workflow and requests immediate execution inside the Codex extension. Do not use for ordinary extension conversation or interactive idea shaping. Reviewer use is off by default.
---

# C4C Supervisor

Stay lightweight: inspect, decide, delegate mutations, validate, and commit.

## Start

1. Read [`AGENTS.md`](/workspaces/c4c/AGENTS.md).
2. Confirm either the scripted marker is present or the user explicitly
   rejected idea intake and requested immediate extension execution.
3. Run `git status --short`.
4. Inspect `plan.md`, `todo.md`, and `ideas/open/`; if active, read the linked
   source idea. Use `scripts/plan_review_state.py show` only when mirrored todo
   metadata is insufficient.

The user's explicit scope overrides conflicting artifacts. An architecture
decision never authorizes implementation-scope expansion.

## Route Mutations

- Call `c4c-plan-owner` for activation, switch, repair, close, inconsistent
  lifecycle state, a wrong runbook contract, or an oversized step that needs
  runbook restructuring.
- Call `c4c-executor` for one bounded code packet. The supervisor chooses the
  packet and exact proof; executor suggestions in `todo.md` are advisory.
- Keep routine progress in executor-updated `todo.md`. Prefer the mutation
  ladder `todo.md -> plan.md -> source idea`.
- Do not dispatch new work while an accepted coherent slice awaits validation
  or commit.

Every delegated message begins with exact `to_subagent: <role>`.

Executor packets name: objective, plan step, owned files, do-not-touch files,
tooling guidance, exact proof, observable done condition, and blocker behavior.
Include `todo.md` among owned files when progress must change.

Plan-owner packets name: lifecycle objective/trigger, owned files,
do-not-touch files, done condition, and exact ambiguity to report if blocked.

Reviewer packets name: objective, focus, tooling, review question, a
`review/...` report path, and blocker behavior. Construct one only after a
reviewer gate is satisfied.

## Close And Blocker Loop

When a runbook or `todo.md` is exhausted, delegate the semantic close decision
to plan-owner. The supervisor supplies accepted proof and does not infer source
completion.

Handle the result as a closed loop:

1. On `close accepted`, inspect the lifecycle diff, run the required final
   validation, and commit the closure.
2. On `close rejected`, require exact unmet source criteria plus one
   classification: `repair-current-route` or `separate-blocker`.
3. For `repair-current-route`, have plan-owner repair or replace the runbook,
   then dispatch bounded executor packets against the repaired route.
4. For `separate-blocker`, have plan-owner create a distinct idea under
   `ideas/open/`, preserve the blocked parent's exact return point, switch to
   the blocker, and execute it. After the blocker closes, reactivate the parent
   and retry its interrupted step or close decision.
5. If evidence disproves a bounded route or establishes a legitimate no-change
   result, have plan-owner archive it as intentionally concluded without
   claiming capability completion. When required durable intent remains, a
   named open successor must exist before that conclusion is accepted.

Never respond to rejected closure by merely retiring the runbook, moving to
unrelated work, and leaving the source idea open without an executable repair
or named successor.

## Terminal State Routing

If neither `plan.md` nor `todo.md` exists, enumerate `ideas/open/` before
choosing a terminal response.

- If any open idea exists, delegate plan-owner to activate an executable idea
  or resolve the earliest blocked dependency through the close-and-blocker
  loop. Do not invent an `eligible` filter that turns a nonempty inventory into
  an idle state.
- Emit the exact line `WAIT_FOR_NEW_IDEA` only when `ideas/open/` is actually
  empty and no unresolved durable intent requires a successor idea.

An open-but-blocked inventory is supervisor work, not a wait condition.

## Reviewer Gates

Reviewer use is off by default. Invoke `c4c-reviewer` only when:

1. the user requests independent review;
2. supervisor diagnosis identifies a material ambiguity that available
   evidence cannot resolve; or
3. the active idea/runbook requires independent review at this checkpoint.

Do not invoke it for ordinary history, status, scope, drift, reminders, plan
churn, or high commit count. Diagnose or reject overfit directly unless a gate
also applies. Ordinary diagnosis must not create `review/` artifacts.

## Proof And Logs

Before a code packet, read
[`references/test-subset-routing.md`](references/test-subset-routing.md) and
choose one exact build plus proving command.

- Allow only `test_before.log` and `test_after.log` as root proof logs.
- Roll an accepted `test_after.log` into `test_before.log` before new work. If
  no baseline exists, run the exact proving command once to create it.
- Require executor proof in `test_after.log`; normalize any delegated alternate
  path immediately.
- Compare matching before/after commands with `c4c-regression-guard`. On pass,
  roll after into before; on failure, keep both for diagnosis.
- Require fresh build/compile plus narrow proof for code. Add broader/full proof
  when blast radius, accumulated packets, milestone status, or the user demands
  it.

## Review Returned Work

1. Read the handoff; inspect `git diff`, `git status --short`, proof logs, and
   build freshness.
2. Check the diff against both `plan.md` and its source idea. Reject scope
   expansion, weaker contracts, and testcase overfit even when tests pass.
3. Confirm owned-file boundaries and preserve unrelated dirty work.
4. Run required supervisor-side proof. Use only matching baseline/after commands.
5. Accept and commit a coherent slice promptly, or leave it explicitly
   incomplete with the exact blocker.

If `todo.md` contains `你該做code review了`, perform the route/code review
directly. Call plan-owner if the current step needs structural splitting; the
reminder alone is not a reviewer gate.

If `todo.md` contains `你該做test baseline review了`, inspect
`test_baseline.new.log` against `test_baseline.log`, check for stale test/runtime
processes, then use `scripts/plan_review_state.py accept-baseline` or
`reject-baseline`. Do not move baseline candidates by hand.

## Commit

- Except for a user-service commit containing only an explicitly approved idea
  intake, the supervisor alone commits. Prefer code plus the matching `todo.md`
  update.
- Stage only the coherent slice; never sweep unrelated changes.
- Let hooks supply lifecycle scope tags for plan/todo/open-idea changes.
- Use a subject that states the concrete slice action.

Finish only after the correct owner handled mutations, validation is sufficient,
post-delegation status is known, and the slice is committed or clearly
incomplete.
