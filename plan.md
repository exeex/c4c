# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md
Activated From: post-302 closure, with no remaining focused open owner

## Purpose

Re-enter the umbrella backend-regex inventory after focused owner 302 and its
split follow-up owners have closed, then decide whether another semantic
repair owner is ready to split or whether the inventory should wait for new
evidence.

## Goal

Produce a current classified backend-regex residual inventory and either split
the next focused `ideas/open/*.md` owner or record why no actionable owner is
ready.

## Core Rule

This is classification and lifecycle work, not implementation. Do not treat
`ctest -R backend` as one monolithic failure, do not reopen closed owners from
counts alone, and do not claim progress through expectations, allowlists,
unsupported classifications, timeout policy, runner behavior, proof-log
policy, or CTest registration changes.

## Read First

- Source idea:
  `ideas/open/295_backend_regex_failure_family_inventory.md`
- Recently closed focused owners:
  - `ideas/closed/302_aarch64_scalar_machine_node_operand_forms.md`
  - `ideas/closed/303_aarch64_sign_extension_assembler_legality.md`
  - `ideas/closed/304_aarch64_ctestsuite_00205_timeout_residual.md`
  - `ideas/closed/305_aarch64_ctestsuite_00205_value_materialization_residual.md`
- Previous source note: the post-301 split identified idea 302 as the next
  focused owner and parked assembly-legality, call-boundary move, `lir_to_bir`,
  runtime, timeout, and output-storm buckets for later classification.

## Current Targets

- Current main-build backend-regex result, including its actual selected-test
  count.
- Residual local backend/unit/CLI failures, if any.
- Residual `c_testsuite_aarch64_backend_*` compile-stage, runtime nonzero,
  runtime mismatch/crash, timeout, or output-storm failures.
- Evidence needed to decide whether the next focused owner is semantic and
  tractable.

## Non-Goals

- Do not implement fixes inside this umbrella idea.
- Do not reopen closed owners 285 through 305 from failure counts alone.
- Do not fold unrelated buckets into one focused idea because they share a
  testcase name or broad failure class.
- Do not ask an executor to run broad runtime tests without timeout and
  stale-process cleanup.
- Do not edit implementation files, closed ideas, test logs, expectations,
  allowlists, unsupported classifications, timeout policy, runner behavior, or
  CTest registration as lifecycle progress.

## Working Model

Idea 295 is the durable inventory owner. It has repeatedly split focused
owners, parked itself, then reactivated after closures to classify the next
residual family. With focused owner 302 now closed, the next step is to refresh
or reconstruct the backend-regex residual set, compare it against the closed
owner boundaries, and split another focused owner only if generated-code,
diagnostic, or proof evidence identifies a real semantic family.

## Execution Rules

- Treat `todo.md` as the live inventory scratchpad.
- Prefer a fresh main-build backend-regex capture when the supervisor selects
  that proof command. If only existing canonical logs are available, first
  record their exact scope and why they are or are not suitable.
- Separate local backend tests from external AArch64 c-testsuite backend tests.
- Classify by failure mechanism, not by filename or pass-count movement.
- Preserve closed owner boundaries unless generated-code or diagnostic
  evidence contradicts them.
- If a tractable semantic owner is found, create a new `ideas/open/*.md`
  source idea with concrete in-scope, out-of-scope, acceptance, and reviewer
  reject signals, then switch lifecycle state to that focused owner before
  implementation.
- If no owner is ready, record the exact evidence gap in `todo.md` and leave
  implementation unstarted.

## Steps

### Step 1: Capture Or Reconstruct Post-302 Backend Inventory

Goal: establish the current backend-regex residual set after focused owner 302
closed.

Primary target: main build backend-regex test output or existing canonical
logs with verified scope

Actions:

- Identify whether current `test_before.log` / `test_after.log` cover the
  backend-regex inventory scope or only a smaller close-gate scope.
- If the supervisor delegates a fresh inventory run, use the main build
  backend selector with timeout/stale-process safeguards appropriate for broad
  runtime scans.
- Record selected count, pass/fail count, local backend/unit failures, and
  `c_testsuite_aarch64_backend_*` failures separately.
- Compare failures against closed owners 285 through 305 and mark any
  suspected contradiction as requiring generated-code proof before reopening.

Completion check:

- `todo.md` records a current or explicitly scoped backend-regex residual
  inventory and names any scope/log gaps.

### Step 2: Classify Remaining Failure Families

Goal: find the next tractable semantic owner, if one exists.

Primary target: residual diagnostics, generated assembly/probes, and failure
categories from Step 1

Actions:

- Group failures into compile-stage machine-printer/frontend diagnostics,
  `lir_to_bir` admission failures, runtime nonzero, runtime mismatch/crash,
  timeout/hang, and output-storm buckets.
- For each candidate group, identify the semantic capability it would repair
  and the evidence that keeps it distinct from closed owners.
- Reject testcase-shaped groups that only share filenames, exact diagnostic
  text, or pass-count movement.
- Quarantine timeout/hang or output-storm cases unless the supervisor selects
  a safe proof flow.

Completion check:

- `todo.md` records either the next focused semantic owner candidate or the
  exact reason no owner is ready to split.

### Step 3: Split Focused Owner Or Park Inventory

Goal: leave lifecycle state ready for implementation only when a focused owner
exists.

Primary target: `ideas/open/*.md`, `plan.md`, and `todo.md`

Actions:

- If Step 2 finds a focused owner, create a new source idea under
  `ideas/open/` with goal, why, in-scope, out-of-scope, acceptance criteria,
  and concrete reviewer reject signals.
- Add a durable deactivation note to idea 295 summarizing the inventory result,
  owner decision, proof/log scope, and remaining buckets.
- Switch active lifecycle state from this umbrella to the new focused owner.
- If no owner is ready, keep `todo.md` as the evidence record and report that
  no implementation packet should be delegated yet.

Completion check:

- The lifecycle state either points to a new focused owner ready for execution,
  or `todo.md` explains why idea 295 remains parked with no activatable
  implementation owner.
