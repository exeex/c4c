# Supervisor Execution Control

Use this workflow when an active plan needs implementation, proof, review, or
a commit. The supervisor selects packets, owns regression logs and acceptance,
and creates commits. `c4c-executor` edits implementation files and updates its
assigned `todo.md` section.

The supervisor does not edit implementation code when the executor role is
available.

## Required Flow

1. Read `plan.md`, `todo.md`, and the linked source idea.
2. Confirm the active files name the same source and current step.
3. Select one bounded incomplete packet; executor suggestions are advisory.
4. Choose the exact build and proving command from
   `references/test-subset-routing.md`.
5. Prepare the matching baseline before implementation when needed.
6. Delegate one packet to `c4c-executor`.
7. Review the handoff, actual diff, proof logs, and build freshness.
8. Accept and commit a coherent slice promptly, or reject it with the exact
   repair/blocker.

Do not dispatch new work while an accepted coherent slice awaits validation or
commit.

## Executor Packet

Begin with exact line `to_subagent: c4c-executor` and name:

- objective and active plan step
- owned files, including `todo.md` when progress must change
- do-not-touch files
- tooling guidance
- exact build and proving command
- observable done condition
- blocker behavior: stop and report; do not pivot

Executors do not choose lifecycle, broader validation, or commit boundaries.

## Anti-Drift And Scope Gate

Reject work that does not directly prove the current packet, widens source
scope, weakens contracts, or follows a testcase-shaped shortcut. Tests are
proof artifacts, not the work queue.

If the first bad fact requires work outside the active idea, stop execution and
read `lifecycle-operations.md`. Do not silently implement the prerequisite.

## Proof And Canonical Logs

- Canonical root logs are only `test_before.log` and `test_after.log`.
- Roll accepted after into before before starting another code packet.
- Before and after commands must match exactly; compare them with
  `c4c-regression-guard`.
- Require a fresh build/compile plus narrow proof for code.
- Add broader/full proof for shared compiler paths, accumulated packets,
  milestones, or explicit user requests.
- On guard failure, retain both logs and reject acceptance until diagnosed.

## Review Returned Work

1. Read the executor handoff and inspect `git status --short` and `git diff`.
2. Compare the diff with both active runbook and source idea.
3. Reject scope expansion, overfit, expectation downgrades, unsupported
   classification changes, or missing nearby same-feature coverage.
4. Confirm owned-file boundaries and preserve unrelated dirty work.
5. Run required supervisor-side proof and inspect matching logs.
6. Accept/commit the slice or return an exact bounded repair.

Reviewer use is off by default. Invoke `c4c-reviewer` only when the user asks,
an unresolved material ambiguity remains, or the active acceptance gate
explicitly requires it. Ordinary status, drift, scope, git history, or reminder
work is not a reviewer gate.

## Reminders

For `你該做code review了`, perform the route/code review directly. Use
plan-owner only if the runbook step needs structural repair.

For `你該做test baseline review了`, inspect `test_baseline.new.log` against
`test_baseline.log`, check stale test/runtime processes, then use
`scripts/plan_review_state.py accept-baseline` or `reject-baseline`. Do not move
hook baseline candidates by hand.

## Commit

- The supervisor creates every final commit except the documented user-service
  approved-idea intake exception.
- Prefer code plus matching `todo.md` progress in one routine slice.
- Stage only owned coherent files; never sweep unrelated changes.
- Let hooks add lifecycle scope tags.
- Use a subject describing the concrete action.

Finish only after the right owner handled mutations, validation is sufficient,
post-delegation state is known, and the slice is committed or explicitly
incomplete.
