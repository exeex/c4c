# AArch64 Prepared Authority Regression Recovery Runbook

Status: Active
Source Idea: ideas/open/58_aarch64_prepared_authority_regression_recovery.md

## Purpose

Recover the current AArch64 regression set introduced after the prepared
authority repair sequence without abandoning the prepared-fact ownership model.

## Goal

Make these four failing tests pass again and prove the repair does not add new
failures in the chosen AArch64 backend scope:

- `backend_aarch64_instruction_dispatch`
- `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`
- `c_testsuite_aarch64_backend_src_00196_c`
- `c_testsuite_aarch64_backend_src_00207_c`

## Core Rule

Repair the semantic AArch64 lowering or prepared-fact consumption fault. Do not
key behavior to testcase names, exact labels, temporary names, or fixed stack
offsets, and do not weaken any failing test contract without explicit approval.

## Read First

- `ideas/open/58_aarch64_prepared_authority_regression_recovery.md`
- `log/baseline_83d5470731cc32247c364d73ad03d8ddc478ec90.log`
- `log/baseline_db1581b3833ae7d45fb379623428490cf60a4b36.log`
- `log/baseline_c9270862701eed736e54a77234bd8060ec318e64.log`
- `log/baseline_78730af2f15da3f272aaa7d138bffd886908cbd0.log`

## Current Targets

- AArch64 instruction dispatch expectations.
- AArch64 dynamic stack fixed-slot FP anchoring.
- AArch64 c-testsuite runtime behavior for `00196` and `00207`.
- Prepared ALU/load-local and variadic register-save publication or consumption
  paths only where they explain the failures.

## Non-Goals

- Do not re-open unrelated architecture backends.
- Do not broad-rewrite ALU, memory, dispatch, calls, comparison, or variadic
  lowering unless regression evidence proves the shared prepared authority
  contract is wrong.
- Do not continue unrelated prepared-authority cleanup after the four
  regressions are repaired.
- Do not mark any target test unsupported or weaken its expected behavior.

## Working Model

The baseline history points to two likely regression boundaries:

- `c92708627` first introduced persistent failures for instruction dispatch,
  dynamic-stack fixed-slot FP anchoring, and `00207`.
- `78730af2f` first introduced the persistent `00196` failure and produced the
  same four-test failure shape that remains in the latest baseline.

Treat these as behavior boundaries for investigation, not as proof that each
commit is independently responsible.

## Execution Rules

- Keep each implementation packet narrowly tied to one failure family or shared
  prepared-fact contract.
- Prefer shared prepared facts or a small semantic prepared lookup over
  restoring duplicate local scans as permanent authority.
- Add or adjust focused coverage only when it captures the repaired semantic
  capability, not the exact failing testcase shape.
- Record any unrelated full-suite failures in `todo.md`; do not expand this
  plan to own them.
- For code-changing steps, use the validation ladder selected by the
  supervisor: build proof, focused failing tests, then broader AArch64 backend
  or regression-guard proof when the slice is acceptance-ready.

## Steps

### Step 1: Reproduce And Classify The Four Failures

Goal: establish the current failing shape and group failures by semantic family.

Primary target: the four tests named in the goal.

Actions:

- Run the supervisor-selected focused proof command for the four failures.
- Inspect current failure output and the latest baseline log.
- Classify each failure as instruction dispatch, dynamic-stack fixed-slot FP
  anchoring, c-testsuite runtime behavior, or shared prepared-fact consumption.
- Identify the smallest nearby same-feature tests that should be checked to
  guard against testcase overfit.

Completion check:

- `todo.md` records the focused command, observed failure shape, preliminary
  family mapping, and nearby same-feature probes for the next repair packet.

### Step 2: Compare The `c92708627` Boundary

Goal: explain the regression family first visible at `c92708627`.

Primary target: instruction dispatch, dynamic-stack fixed-slot FP anchoring,
and `00207`.

Actions:

- Compare behavior and relevant AArch64 lowering/prepared-fact diffs before
  and after `c92708627`.
- Identify which prepared fact, helper, or lowering path changed authority for
  the affected family.
- Avoid committing a reversal unless the semantic owner is understood.

Completion check:

- The responsible boundary behavior is documented in `todo.md`, including the
  implementation surface that should own the repair.

### Step 3: Repair The First Boundary Failure Family

Goal: fix the semantic fault responsible for the `c92708627` family.

Primary target: the AArch64 lowering or prepared-fact consumption path
identified in Step 2.

Actions:

- Implement the smallest semantic repair that restores correct authority.
- Prefer consuming existing prepared facts where they are already the intended
  owner.
- If a new lookup is needed, give it one semantic owner and consume it from the
  relevant lowering path.
- Run build proof and the focused target tests selected by the supervisor.

Completion check:

- The affected focused tests pass or have a reduced, documented failure shape
  that points to a separate remaining family.
- The diff contains no testcase-name matching, expectation downgrade, or broad
  duplicate local scan as the durable fix.

### Step 4: Compare The `78730af2f` Boundary

Goal: explain the `00196` failure first visible at `78730af2f`.

Primary target: AArch64 prepared ALU/load-local or related c-testsuite runtime
behavior for `00196`.

Actions:

- Compare behavior and relevant diffs before and after `78730af2f`.
- Determine whether `00196` shares the repaired Step 3 root cause or exposes a
  second prepared authority gap.
- Check nearby same-feature behavior before treating `00196` as complete.

Completion check:

- `todo.md` records whether `00196` is fixed by the first repair or identifies
  the remaining semantic owner and proof target for the next packet.

### Step 5: Repair The Remaining `00196` Family

Goal: fix any remaining semantic fault responsible for `00196`.

Primary target: the AArch64 lowering or prepared-fact consumption path
identified in Step 4.

Actions:

- Implement the smallest semantic repair needed for the remaining family.
- Preserve the prepared-authority direction.
- Run build proof, the `00196` target, and any nearby same-feature checks
  selected by the supervisor.

Completion check:

- `00196` and its selected nearby same-feature probes pass without weakening
  contracts or adding testcase-shaped matching.

### Step 6: Prove The Focused Regression Set

Goal: prove all four original failures are repaired together.

Primary target: the four-test focused failure set.

Actions:

- Run the matching focused command for all four target tests.
- Inspect output for hidden expectation downgrades or unsupported-path drift.
- Record proof in `test_after.log` if delegated as the executor proof log.

Completion check:

- All four focused failures pass in the same proof run, and `todo.md` records
  the command and result.

### Step 7: Run Broader AArch64 Regression Guard

Goal: show the repairs did not introduce new failures in the chosen AArch64
backend scope.

Primary target: supervisor-selected regression guard or equivalent matching
before/after proof for AArch64 backend coverage.

Actions:

- Run the broader proof chosen by the supervisor, preferably a matching
  before/after regression guard when acceptance is being evaluated.
- Compare failures against the pre-repair baseline.
- Record unrelated remaining failures separately in `todo.md`.

Completion check:

- The broader proof shows no new failures in scope, or any remaining red tests
  are explicitly classified as unrelated to this source idea.

### Step 8: Closure Readiness Review

Goal: prepare the source idea for closure only if its acceptance criteria are
actually satisfied.

Actions:

- Confirm the four target tests pass.
- Confirm the broader AArch64 proof is acceptable.
- Confirm `todo.md` explains which boundary was responsible for each failing
  family: `c92708627`, `78730af2f`, or a shared later interaction.
- Confirm no reject signal from the source idea applies to the final diff.

Completion check:

- The supervisor can route to plan-owner close review with complete proof, or
  `todo.md` clearly names the remaining blocker.
