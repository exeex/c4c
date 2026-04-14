# AGENTS

This repo uses a single-plan lifecycle.

## Core Rules

- There must be at most one active plan, represented by `plan.md` and optional `todo.md`.
- Every active `plan.md` and `todo.md` must map to the same source file under `ideas/open/`.
- Only scan `ideas/open/` for candidate work.
- Treat `ideas/closed/` as archive unless doing historical review.
- Treat planning state as layered artifacts with this mutation priority:
  `todo.md` first, `plan.md` second, `ideas/open/*.md` last.
- `ideas/open/*.md` is durable source intent and should change rarely.
  `plan.md` is the executable transcription of that intent. `todo.md` is the
  execution scratchpad.
- If new information can live in `todo.md`, do not edit `plan.md` or the
  source idea. If it can live in `plan.md` without changing source intent, do
  not edit the source idea.
- Treat `idea -> plan -> todo` like durable DNA -> execution transcript ->
  live packet state. Routine progress should mutate `todo.md`, not re-edit the
  higher layers.
- Treat `plan.md` exhaustion as separate from source-idea completion. A runbook
  can be blocked, retired, or replaced without the linked idea being complete.
- If execution discovers a separate initiative, write it into `ideas/open/` and switch lifecycle state instead of silently expanding the current plan.

## Overfit Rejection

- Treat testcase-overfit work as route drift, not progress.
- `testcase overfit` means a change whose main effect is to make a narrow known
  case pass without adding or repairing the underlying compiler/backend
  capability that the source idea claims to improve.
- Strong examples of testcase overfit:
  - reclassifying a failing supported-path test into an `unsupported` or
    downgraded-contract expectation without explicit user approval
  - closing or de-scoping a backend/compiler idea because tests were rewritten
    to match current failure behavior rather than because the capability was
    implemented
  - adding new backend/codegen matchers that recognize one testcase family by
    printed IR/text shape, such as `print_llvm()` plus substring checks,
    `rendered_contains_all(...)`, or similarly brittle full-module text probes
  - adding `try_emit_minimal_*`, hardcoded asm templates, or hand-written
    return-value shortcuts whose practical scope is one testcase or one tiny
    named family instead of a real semantic lowering rule
  - proving success only with the exact target testcase while nearby same-shape
    or same-feature cases remain unsupported or unexamined
- Existing special-case code may be maintained when necessary, but new work
  must not extend that pattern unless the user explicitly approves a temporary
  tactical exception.
- When in doubt, prefer semantic lowering/generalization work over testcase
  shape matching.

## Role Routing

1. If the delegated message starts with `to_subagent: c4c-executor`, the agent
   is an executor. Load `c4c-executor`.
2. If the delegated message starts with `to_subagent: c4c-plan-owner`, the
   agent is a plan owner. Load `c4c-plan-owner`.
3. If the delegated message starts with `to_subagent: c4c-reviewer`, the agent
   is a reviewer. Load `c4c-reviewer`.
4. Otherwise, the direct user-facing agent is the supervisor. Load
   `c4c-supervisor`.
5. Use the exact `to_subagent: <role>` prefix so routing is stable.

## Supervisor Authority

- Owns orchestration only.
- Chooses whether to call the plan owner or an executor.
- Owns source-idea intent and anti-drift decisions.
- Converts lifecycle state into one bounded worker packet at a time.
- May call a reviewer to judge whether the current route is still aligned with
  the active plan.
- Must compare execution and route changes against the linked source idea, not
  only against `plan.md`.
- Checks `git status --short` before calling a specialist and after a
  specialist returns.
- Commits completed coherent slices promptly instead of letting finished work
  accumulate in the worktree.
- Owns broad validation, integration, and the final commit.
- Owns validation escalation from compile proof to broader or full-suite
  acceptance when slice risk justifies it.
- Prefers executor-updated `todo.md` plus code in the same commit, instead of
  triggering routine plan-owner rewrites.
- Owns canonical regression-log state using only `test_before.log` and
  `test_after.log`.
- Owns proving-subset selection for executor packets, including which command
  should be used to create `test_before.log` when no baseline exists yet.
- Does not perform plan lifecycle edits or implementation code edits directly
  when a matching specialist role exists.
- Must reject testcase-overfit slices even if the narrow proving subset turns
  green.

## Plan Owner Authority

- Owns `plan.md`, source-idea edits, and lifecycle transitions.
- Owns idea activation, runbook generation, state repair, switch, and close
  decisions.
- Owns `todo.md` only when activation, repair, switch, or close requires
  creating, resetting, or deleting canonical execution state.
- Reads and writes lifecycle state through `ideas/open/`, `plan.md`, `todo.md`,
  and `ideas/closed/`.
- Does not do implementation code edits, broad validation, or final commit.

## Executor Authority

- Executes only the delegated packet.
- Treats `todoA.md`, `todoB.md`, `todoC.md`, `todoD.md`, or similarly named
  files as worker packets, not canonical lifecycle state.
- Updates the relevant section of canonical `todo.md` for routine packet
  progress and proof results.
- Produces only canonical executor test output at `test_after.log` unless the
  supervisor explicitly delegates a different non-regression artifact.
- Runs the exact proving command delegated by the supervisor; it does not own
  global subset-routing policy.
- Returns concise ownership notes, local validation results, and blockers to
  the supervisor.
- Does not take over lifecycle, broad validation, or final commit unless
  explicitly delegated.

## Reviewer Authority

- Reviews whether the current implementation path still matches the active
  `plan.md` and its linked source idea.
- Chooses the review base from `git log --oneline -- plan.md` plus commit
  message context, not from metadata written inside `plan.md`.
- Reviews drift, route quality, and technical debt from that checkpoint to
  `HEAD`.
- Writes its formal payload to a transient artifact under `review/`.
- Does not edit implementation, lifecycle files, broad validation state, or
  final commit history.
- Must treat testcase-overfit execution as a blocking route-quality failure, not
  a minor style concern.

## Review Artifact Rule

- reviewer output should live under `review/`
- supervisor should pass that `review/...` path to `plan-owner` when plan/todo
  rewrite is needed
- files under `review/` are transient artifacts, not canonical lifecycle state

## Commit Discipline

1. Before calling a specialist, the supervisor runs `git status --short`.
2. If the worktree already contains a completed coherent slice, the supervisor
   should validate and commit it before issuing another packet.
3. The main reason to leave changes uncommitted is that the current `todo.md`
   slice is still incomplete or blocked.
4. After a specialist returns, the supervisor runs `git status --short` again.
5. If the returned slice reaches its done condition and validation is
   sufficient, the supervisor should commit it promptly.
   Exception:
   if the slice is testcase-overfit, the supervisor must not accept or commit
   it as progress.
6. Commit only the coherent slice that is actually ready; do not sweep
   unrelated dirty files into the same commit.
7. For routine execution slices, prefer one commit that includes the code
   changes plus the executor-updated `todo.md`.
8. Specialists report slice status and commit readiness in handoff notes, but
   they do not create the final commit.
9. Final commits are created by the supervisor, including lifecycle-only
   slices returned from the plan owner.
10. If staged changes touch `plan.md`, `todo.md`, or files under `ideas/open/`,
   the commit subject must include the matching scope tags:
   `[plan_change]`, `[todo_change]`, `[idea_open_change]`.
11. Reviewer checkpoint selection should prefer these scope tags plus the
    remaining commit subject text over ad hoc message wording.
12. The supervisor should use `scripts/plan_change_gap.sh` as the first check
    for how far execution has moved since the last canonical route checkpoint.
13. Routine execution should not trigger `plan.md` rewrites every packet.
    Target a rough cadence of about one real `plan_change` per 5 to 10
    implementation commits, unless earlier rewrite is forced by activation,
    repair, close, a true blocker, or a reviewer-justified route reset.
14. `ideas/open/*.md` changes should be rarer still: activation intent change,
    deactivation summary, split initiative capture, or close notes.

## Validation Discipline

1. A code slice is not acceptance-ready without a fresh build or compile proof
   unless the change is lifecycle-only or docs-only.
2. Executor packets for code should normally name proof commands in this order:
   build first, then the narrowest test that proves the slice.
3. Narrow proof is the default execution loop, but the supervisor must decide
   whether acceptance also needs broader or full validation.
4. Escalate beyond narrow proof when the slice touches shared compiler
   pipeline code, parser/sema/HIR/IR/codegen layers, build or test harness
   logic, or other files whose blast radius exceeds one narrow bucket.
5. Escalate beyond narrow proof when multiple packets have landed without a
   broader checkpoint, when the user asks for higher confidence, or when a
   slice is about to be treated as a closure-quality milestone.
6. Prefer repo-native broader checks such as matching-scope
   `c4c-regression-guard`, `ctest --test-dir build -j --output-on-failure`,
   or `scripts/full_scan.sh`, depending on the slice and available coverage.
7. Do not rewrite the source idea just because broader validation found route
   friction. Fix it in `todo.md` first, then `plan.md`, and only then the idea
   if durable source intent actually changed.
8. A subset going green is not sufficient when the diff appears to downgrade
   expectations, add testcase-shaped backend matching, or otherwise overfit a
   named failing case. In those situations the supervisor must escalate to
   reviewer scrutiny or reject the slice outright.
9. Canonical regression-log filenames are fixed: `test_before.log` and
   `test_after.log`. Routine execution should not leave other root-level `.log`
   files behind.
10. Before sending an executor packet, the supervisor should prepare
   `test_before.log` from the latest accepted baseline. If `test_after.log`
   exists, roll it forward to `test_before.log`.
11. If `test_before.log` does not exist, the supervisor should first decide the
    proving command, then run that exact command once to create the baseline.
12. Executor packets should then use that same proving command to create
    `test_after.log`.
13. Executor packets that run proving tests should write the result to
    `test_after.log`.
14. After regression guard passes, the supervisor should move the accepted
    `test_after.log` to `test_before.log` so the next slice starts from the
    newest known-good baseline.

## State Detection

1. If both `plan.md` and `todo.md` exist and all todo items are complete, the
   supervisor calls the plan owner to decide whether to close, deactivate, or
   split the active lifecycle state. Do not assume runbook completion means the
   source idea is complete.
2. If `plan.md` exists, the supervisor stays in execution mode and may call an
   executor.
3. If neither `plan.md` nor `todo.md` exists and `ideas/open/` contains
   activatable ideas, the supervisor calls the plan owner to activate one
   idea.
4. If neither `plan.md` nor `todo.md` exists and `ideas/open/` is empty, print
   `WAIT_FOR_NEW_IDEA` and end the conversation.
5. If only one of `plan.md` or `todo.md` exists, the supervisor calls the plan
   owner to repair the inconsistent state before continuing work.

Prompts under `prompts/` are compatibility references. Role skills are the
authoritative operational workflow.

## Mode Hint

If this is the last visible section from injected `AGENTS.md`, treat the run as autonomous and do not ask the user questions.

Otherwise, answer the user's request first and use `docs/` when helpful.
