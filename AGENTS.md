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
- `todo.md` execution metadata should keep regex-friendly single-line
  `Current Step ID:`, `Current Step Title:`, and
  `Plan Review Counter: <counter> / <review_limit>` fields near the top so
  route-friction decisions do not depend on commit-history guesswork alone.
- Treat `plan.md` exhaustion as separate from source-idea completion. A runbook
  can be blocked, retired, or replaced without the linked idea being complete.
- If execution discovers a separate initiative, write it into `ideas/open/` and switch lifecycle state instead of silently expanding the current plan.

## Overfit Rejection

- Treat testcase-overfit work as route drift, not progress.
- `testcase overfit` means a change whose main effect is making a narrow known
  case pass without repairing the underlying compiler/backend capability the
  source idea claims to improve.
- Strong examples:
  - downgrading a supported-path test to `unsupported` or a weaker contract
    without explicit user approval
  - claiming backend/compiler progress mainly through expectation rewrites
    rather than capability repair
  - adding testcase-shaped matching or tiny named-case shortcuts instead of a
    real semantic lowering rule
  - proving only the target testcase while nearby same-feature cases remain
    unsupported or unexamined
- Existing special-case code may be maintained when necessary, but new work
  must not extend that pattern unless the user explicitly approves a temporary
  tactical exception.
- When in doubt, prefer semantic lowering/generalization over testcase-shape
  matching.

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

- Owns orchestration, route choice, and anti-drift decisions.
- Chooses whether to call `plan-owner`, `executor`, or `reviewer`.
- Compares execution against the linked source idea, not only `plan.md`.
- Uses `todo.md` execution metadata, including `Current Step ID`,
  `Current Step Title`, and `Plan Review Counter`, when deciding whether a
  step needs plan review or substep expansion.
- Checks `git status --short` before delegation and after return.
- Owns broader validation, canonical regression-log state, and the final
  commit.
- Chooses the proving subset and baseline command for executor packets.
- Prefers `executor`-updated `todo.md` plus code in one commit over routine
  `plan-owner` rewrites.
- Does not perform lifecycle or implementation edits directly when a matching
  specialist exists.
- Must reject testcase-overfit slices even if narrow proof is green.
- Role-specific packet shape and operating details live in
  `.codex/skills/c4c-supervisor/`.

## Plan Owner Authority

- Owns `plan.md`, source-idea edits, and lifecycle transitions.
- Handles activation, repair, switch, runbook regeneration, and close.
- Touches `todo.md` only when lifecycle work must create, reset, or delete
  canonical execution state.
- Resets local plan-review counter state when a plan review rewrites or splits
  the current step.
- Reads and writes lifecycle state through `ideas/open/`, `plan.md`, `todo.md`,
  and `ideas/closed/`.
- Does not do implementation edits, broad validation, or the final commit.
- Detailed lifecycle workflow lives in `.codex/skills/c4c-plan-owner/`.

## Executor Authority

- Executes only the delegated packet.
- Treats `todoA.md`, `todoB.md`, `todoC.md`, `todoD.md`, or similar files as
  worker packets, not canonical lifecycle state.
- Updates the assigned section of canonical `todo.md` with packet progress and
  proof results.
- Keeps `Current Step ID` and `Current Step Title` in `todo.md` aligned with
  the delegated packet step, but does not decide when plan review is required.
- Uses `test_after.log` as the canonical executor proof log unless the
  supervisor explicitly delegates another non-regression artifact.
- Runs the exact proof command delegated by the supervisor; it does not own
  subset-routing policy.
- Returns concise ownership notes, local validation results, and blockers.
- Does not take over lifecycle, broad validation, or the final commit.
- Packet contract and `todo.md` update rules live in
  `.codex/skills/c4c-executor/`.

## Reviewer Authority

- Reviews whether implementation still matches active `plan.md` and its linked
  source idea.
- Chooses the review base from git history on `plan.md`, not metadata written
  inside `plan.md`.
- Reviews drift, route quality, technical debt, and proof sufficiency from that
  checkpoint to `HEAD`.
- Writes the formal payload to a transient artifact under `review/`.
- Does not edit implementation, lifecycle files, validation state, or commit
  history.
- Must treat testcase-overfit as a blocking route-quality failure.
- Review-base selection and report format live in
  `.codex/skills/c4c-reviewer/`.

## Review Artifact Rule

- reviewer output should live under `review/`
- supervisor should pass that `review/...` path to `plan-owner` when plan/todo
  rewrite is needed
- files under `review/` are transient artifacts, not canonical lifecycle state

## Commit Discipline

1. The supervisor owns commit boundaries and creates all final commits,
   including lifecycle-only slices returned from the plan owner.
2. Commit only coherent slices that are ready; do not sweep unrelated dirty
   files into the same commit.
3. A completed slice should usually be validated and committed promptly. The
   main reason to leave changes uncommitted is that the active `todo.md` slice
   is still incomplete or blocked.
4. For routine execution, prefer one commit that includes code changes plus the
   executor-updated `todo.md`.
5. Specialists may report slice status and commit readiness, but they do not
   create the final commit.
6. If a slice is testcase-overfit, the supervisor must not accept or commit it
   as progress.
7. If staged changes touch `plan.md`, `todo.md`, or files under `ideas/open/`,
   rely on the git hook for canonical lifecycle scope tags instead of manually
   duplicating them in the subject.
8. `plan.md` and `ideas/open/*.md` should change less often than routine code
   and `todo.md` updates; reserve those edits for true lifecycle or route
   changes.

## Validation Discipline

1. A code slice is not acceptance-ready without fresh build or compile proof,
   unless it is lifecycle-only or docs-only.
2. Narrow proof is the default execution loop, but the supervisor decides
   whether acceptance also needs broader or full validation.
3. Escalate validation when blast radius extends beyond one narrow bucket, when
   multiple narrow-only packets have landed, when the user asks for higher
   confidence, or when the slice is being treated as a milestone.
4. Prefer repo-native broader checks such as `c4c-regression-guard`,
   `ctest --test-dir build -j --output-on-failure`, or
   `scripts/full_scan.sh`, depending on scope.
5. A green subset is not sufficient when the diff appears to downgrade
   expectations or overfit a named failing case; the supervisor must escalate
   to reviewer scrutiny or reject the slice.
6. Canonical regression-log filenames are fixed: `test_before.log` and
   `test_after.log`. Routine execution should not leave other root-level `.log`
   files behind.
7. Regression-log preparation, proving-command selection, and baseline/log
   roll-forward policy belong to the supervisor and executor workflows in their
   respective skills.

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

If this section is followed by a user prompt, treat the run as interactive.
In interactive runs, user interaction takes priority over the default
plan-lifecycle flow: answer the user's request first, then apply lifecycle
rules as a secondary constraint. Use `docs/` when helpful.
