# AGENTS

This repo uses one active-plan lifecycle.

## Lifecycle Invariants

- `plan.md` and `todo.md` either both exist or both do not. When present, both
  must name the same source under `ideas/open/`.
- Scan only `ideas/open/` for candidate work. Treat `ideas/draft/` as parked and
  `ideas/closed/` as archive unless history is explicitly needed.
- Keep at most one active plan. Treat idea, plan, and todo as durable intent,
  executable runbook, and live packet state respectively.
- Apply execution updates in this order: `todo.md`, then `plan.md`, then the
  source idea. Do not promote information when a lower layer is sufficient.
- Keep regex-friendly single-line `Current Step ID:` and `Current Step Title:`
  fields near the top of `todo.md`. Add hook-managed review/baseline reminders
  only when emitted; do not keep a permanent review counter there.
- Runbook exhaustion does not prove source-idea completion. A runbook may be
  retired, replaced, or blocked while its idea stays open.
- Record a separate initiative under `ideas/open/` instead of silently
  expanding the current idea.

## Interactive Authority

- The supervisor answers read-only interactive diagnosis directly, including
  git history, status, scope comparison, and drift triage. Specialist existence
  does not require delegation for read-only questions.
- The user's explicit source intent or scope overrides idea, plan, todo,
  reviewer, and historical agent artifacts. Treat conflicts as stale artifacts.
- Agreement on architecture changes only that decision. It does not authorize
  absorbing prerequisites, downstream implementation, or other scope into the
  current idea.
- Delegate lifecycle and implementation mutations to their owners. Do not
  delegate merely to restate evidence the supervisor can inspect directly.

## Role Routing

Use the exact first line `to_subagent: <role>` for delegated work:

- `c4c-plan-owner`: activate, repair, switch, deactivate, or close lifecycle
  state; create or edit source ideas and runbooks. It does not edit code,
  perform broad validation, or commit.
- `c4c-executor`: implement one bounded packet, update its assigned `todo.md`
  section, and run the exact delegated proof. It does not choose lifecycle,
  broader validation, or commits.
- `c4c-reviewer`: provide an independent read-only route review when one of the
  reviewer gates below applies. It writes only transient `review/` artifacts.
- Any direct user-facing agent is `c4c-supervisor`.

The supervisor owns orchestration, anti-drift decisions, proving-command
selection, canonical regression logs, broader validation, and final commits.
It delegates lifecycle edits to plan-owner and implementation edits to executor.
Use `c4c-divide-and-conquer` when repeated collisions justify a separate
decomposition initiative.

## Reviewer Gates

Reviewer use is off by default. Invoke it only when:

1. the user explicitly requests an independent review;
2. supervisor diagnosis leaves a material unresolved ambiguity; or
3. the active idea/runbook explicitly requires independent review at the
   current acceptance gate.

Do not invoke a reviewer or create `review/` output for ordinary git-log,
status, scope, drift, reminder, or high-commit-count questions. When invoked,
pass the resulting `review/...` path to plan-owner only if lifecycle repair is
needed.

## Acceptance Rules

- Reject testcase overfit: expectation downgrades, supported-to-unsupported
  changes, named-case matchers, rendered-text probes, or testcase-shaped
  backend shortcuts are not capability progress.
- Require nearby same-feature coverage; one target testcase is insufficient
  when the idea claims a semantic capability.
- Require a fresh build or compile plus the narrow delegated proof for code.
  Escalate to broader/full proof for shared code, accumulated narrow packets,
  milestones, or explicit user requests.
- Canonical root regression logs are only `test_before.log` and
  `test_after.log`. The supervisor owns their preparation and roll-forward.
- A green test does not override scope drift, overfit, or weaker contracts.

## Commit Rules

- The supervisor creates every final commit, including lifecycle-only commits.
- Commit only coherent, validated slices and preserve unrelated user changes.
- Prefer code plus executor-updated `todo.md` in one routine execution commit.
- Do not commit overfit work. Do not leave an accepted coherent slice pending
  while dispatching new work.
- Let the git hook add lifecycle scope tags when staged files include
  `plan.md`, `todo.md`, or `ideas/open/*`; do not duplicate those tags manually.

## State Routing

- Both `plan.md` and `todo.md`, incomplete work: stay in execution mode.
- Both present, todo complete: ask plan-owner whether to close, deactivate, or
  replace; do not infer idea completion.
- Only one present: plan-owner repairs the inconsistent state.
- Neither present, open ideas exist: plan-owner activates one.
- Neither present, no open ideas: print `WAIT_FOR_NEW_IDEA` and stop.

Prompts under `prompts/` are compatibility references; role skills are
authoritative workflows. If no user prompt follows this file, run autonomously.
Otherwise answer the user first and apply lifecycle rules secondarily.
