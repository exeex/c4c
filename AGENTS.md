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
- Runbook exhaustion does not prove source-idea completion. Send exhausted
  runbooks to plan-owner for an explicit close, repair, replace, or conclude
  decision. Do not leave a retired runbook's idea open without an executable
  repair route or a named successor that owns the remaining intent.
- Record a separate initiative under `ideas/open/` instead of silently
  expanding the current idea.

## Interactive Authority

- A prompt containing exact line `C4C_RUN_MODE=scripted` was launched by
  `scripts/run_agent.sh`; route it to `c4c-supervisor` and run autonomously.
- Without that marker, the Codex extension direct user-facing role is
  `c4c-user-service`. Answer the user's current request without advancing the
  active plan merely because lifecycle state exists.
- User service answers read-only interactive diagnosis directly, including git
  history, status, scope comparison, and drift triage. Specialist existence
  does not require delegation for read-only questions.
- Treat a new implementation or design request as interactive idea intake.
  User service discusses scope, delegates plan-owner to maintain
  `ideas/draft/`, and requires explicit approval before promotion to
  `ideas/open/`.
- After approval, user service commits only the open idea and directs the user
  to `./scripts/run_agent.sh`. It does not activate or implement the idea.
- Transfer to supervisor inside the extension only when the user explicitly
  rejects the idea/run-agent workflow and asks to execute there immediately.
- The user's explicit source intent or scope overrides idea, plan, todo,
  reviewer, and historical agent artifacts. Treat conflicts as stale artifacts.
- Agreement on architecture changes only that decision. It does not authorize
  absorbing prerequisites, downstream implementation, or other scope into the
  current idea.
- Delegate lifecycle and implementation mutations to their owners. Do not
  delegate merely to restate evidence the supervisor can inspect directly.

## Role Routing

Use the exact first line `to_subagent: <role>` for delegated work:

- `c4c-plan-owner`: create or revise draft ideas, promote user-approved drafts,
  activate, repair, switch, deactivate, or close lifecycle state, and edit
  source ideas and runbooks. It does not edit code, perform broad validation,
  or commit.
- `c4c-executor`: implement one bounded packet, update its assigned `todo.md`
  section, and run the exact delegated proof. It does not choose lifecycle,
  broader validation, or commits.
- `c4c-reviewer`: provide an independent read-only route review when one of the
  reviewer gates below applies. It writes only transient `review/` artifacts.
- `c4c-user-service`: default Codex extension role for interactive questions.
  It shapes new requirements into user-approved open ideas, then stops before
  activation or implementation.
- `c4c-supervisor`: scripted `run_agent.sh` role and the execution workflow for
  a user request that explicitly rejects idea intake and asks the extension to
  execute immediately.

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
- Exception: user service may commit one plan-owner-produced, explicitly
  user-approved idea-intake slice without activating it.
- Commit only coherent, validated slices and preserve unrelated user changes.
- Prefer code plus executor-updated `todo.md` in one routine execution commit.
- Do not commit overfit work. Do not leave an accepted coherent slice pending
  while dispatching new work.
- Let the git hook add lifecycle scope tags when staged files include
  `plan.md`, `todo.md`, or `ideas/open/*`; do not duplicate those tags manually.

## State Routing

Apply this section only in scripted supervisor mode or after an explicit
user-service transfer to supervisor. Never apply it merely because an extension
conversation opened in a repo with lifecycle files.

- Both `plan.md` and `todo.md`, incomplete work: stay in execution mode.
- Both present, todo complete: ask plan-owner whether to close, deactivate, or
  replace; do not infer idea completion.
- If plan-owner rejects closure, it must return the exact unmet source
  criteria and classify them as an in-scope runbook repair or a separately
  scoped blocker. The supervisor owns the response: repair and continue the
  current route, or have plan-owner create and activate the blocker idea, then
  reactivate the parent after the blocker closes. Do not strand the rejected
  idea as open with a retired runbook.
- A disproven or no-change route may be archived as intentionally concluded
  without claiming capability completion. If durable required intent remains,
  the conclusion must name an open successor before the old idea is archived.
- Only one present: plan-owner repairs the inconsistent state.
- Neither present, open ideas exist: plan-owner activates an executable idea
  or resolves the earliest blocked dependency by repairing it or creating and
  activating a separately scoped successor. "No eligible idea" is not
  equivalent to an empty `ideas/open/` inventory.
- Neither present, no open ideas: print `WAIT_FOR_NEW_IDEA` and stop.

Role skills are the authoritative workflows. Use the explicit scripted marker,
not conversational tone, to choose between supervisor and user service.
