# AArch64 Variadic HFA And Floating Residual Refresh Runbook

Status: Active
Source Idea: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Activated from: remaining open candidate after ideas 360 and 362 were closed

## Purpose

Refresh the still-open AArch64 variadic HFA/floating residual owner without
reopening adjacent repairs or chasing unrelated current failures.

## Goal

Confirm whether a current first bad fact still exists inside the source idea's
HFA/floating, composite variadic call-boundary, or structured f128/q-register
authority scope, then either close the idea or hand off a freshly localized
residual.

## Core Rule

Do not implement under this idea unless fresh generated-code evidence shows
the current first bad fact belongs to the original HFA/floating or composite
variadic call-boundary owner.

## Read First

- `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`
- Parked handoff notes in related AArch64 variadic ideas named by the source
  idea, especially fixed-formal entry publication, byval aggregate lane
  publication, stdarg cursor/format, and MOVI zero-extension.
- Existing focused backend coverage for `00204.c` variadic/HFA/generated-code
  guardrails.
- Generated BIR, prepared BIR, and AArch64 artifacts for
  `c_testsuite_aarch64_backend_src_00204_c` only as evidence, not as a
  testcase-specific repair target.

## Current Targets

- `c_testsuite_aarch64_backend_src_00204_c`
- Focused `backend_cli_dump_(bir|prepared_bir).*00204` coverage
- AArch64 variadic/HFA/composite call-boundary paths that can still produce a
  standalone in-scope first bad fact

## Non-Goals

- Do not reopen fixed-formal entry publication, byval aggregate lane
  publication, stdarg cursor/format byte materialization, MOVI
  zero-extension, local/value-home publication, frame/formal publication, or
  aggregate helper lowering without fresh first-bad-fact evidence and a
  lifecycle split.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, proof-log policy, or CTest registration.
- Do not special-case `00204.c`, `myprintf`, one HFA shape, one float value,
  one register lane, one stack offset, one format string, or one emitted
  instruction sequence.

## Working Model

The source idea has repeatedly served as the HFA/floating variadic owner for
`00204.c`, but its latest lifecycle note says the current focused subset
passed and no in-scope first bad fact was visible. The file remains under
`ideas/open/` because closure was rejected by the strict monotonic regression
guard, not because the latest note identified an executable repair. This
runbook therefore starts with a refresh and only proceeds to implementation if
that refresh proves the source scope is live again.

## Execution Rules

- Treat `ideas/open/326_aarch64_variadic_hfa_floating_residual.md` as the
  durable scope contract.
- Prefer read-only generated artifact inspection before code edits.
- If the representative and focused dump subset remain green, request
  lifecycle closure instead of implementation work.
- If the representative fails for an out-of-scope owner, record the handoff at
  the lifecycle layer and split or activate the appropriate source idea.
- For any code-changing step, require build proof plus the
  supervisor-selected focused CTest subset before acceptance.
- Reject expectation downgrades, testcase-shaped matching, or helper renames
  claimed as capability progress.

## Step 1: Refresh The Current Variadic First Bad Fact

Goal: Determine whether the current tree still has an in-scope HFA/floating,
composite variadic call-boundary, or structured f128/q-register authority
failure.

Primary Target: `00204.c` generated BIR, prepared BIR, generated AArch64, and
focused backend dump tests.

Actions:

- Build the current tree.
- Run the focused supervisor-selected subset for `00204.c` and its BIR /
  prepared-BIR dump coverage.
- If anything fails, inspect generated artifacts to locate the first bad fact
  before choosing an owner.
- Classify the first bad fact as in scope for this source idea or explicitly
  out of scope.

Completion Check:

- The current first bad fact is either absent, or classified with concrete
  generated-code evidence and an owner boundary.

## Step 2: Close Or Hand Off If The Owner Is Not Live

Goal: Avoid expanding a parked source idea when the current failure is absent
or belongs elsewhere.

Primary Target: lifecycle state for this active plan.

Actions:

- If Step 1 is green and no in-scope first bad fact remains, request
  plan-owner close using matching focused before/after logs or the current
  supervisor-approved closure mode.
- If Step 1 finds an out-of-scope first bad fact, write the lifecycle handoff
  at the lowest correct layer and split or activate the appropriate separate
  source idea.
- Do not modify implementation files in this step.

Completion Check:

- The idea is closed, or lifecycle state is redirected to the correct
  out-of-scope owner without mutating this source idea's scope.

## Step 3: Repair Only A Returned In-Scope Owner

Goal: Implement a bounded repair only if Step 1 proves this source idea is
again the active owner.

Primary Target: the concrete AArch64 variadic/HFA/composite call-boundary path
localized by Step 1.

Actions:

- Delegate a narrow executor packet naming the localized owner, target files,
  focused proof command, and adjacent guardrails.
- Add or update focused backend coverage that fails without the repair and
  passes with it.
- Preserve completed adjacent repairs named in the source idea's handoff
  notes.

Completion Check:

- The focused owner coverage passes, `00204.c` advances or passes, and any
  remaining first bad fact is reclassified before this idea is considered for
  closure.
