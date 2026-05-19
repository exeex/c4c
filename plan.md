# Backend Regex Failure Family Inventory

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Use the main-build `ctest -R backend` result as an umbrella inventory and split
focused repair owners only when a failure group points to a semantic backend
capability.

## Goal

Classify the remaining backend-regex failures after the completed fused
compare-branch owner and decide the next focused repair idea, without turning
the umbrella inventory into implementation work.

## Core Rule

Inventory and split semantic owners. Do not implement fixes in this umbrella
runbook, and do not claim progress through expectation, allowlist,
unsupported-classification, timeout, runner, or CTest-registration changes.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `todo.md`
- `ideas/closed/296_aarch64_fused_compare_branch_operand_forms.md`
- Current close proof in `test_before.log`

## Current Targets

- Main build backend-regex failures from `/workspaces/c4c/build`.
- Remaining AArch64 c-testsuite backend buckets after focused owner 296.
- Residuals currently known from focused owner 296:
  `00200` runtime mismatch, and `00207`, `00214`, `00215` scalar add/xor
  immediate printer limits.

## Non-Goals

- Do not implement backend fixes in this umbrella plan.
- Do not treat `ctest -R backend` as one monolithic repair bucket.
- Do not reopen closed AArch64 owners 285 through 296 from failing counts alone.
- Do not match exact c-testsuite filenames, local test names, or emitted
  instruction strings instead of identifying semantic owners.
- Do not run broad runtime tests without stale-process cleanup and timeout
  precautions.

## Working Model

Idea 295 is the umbrella inventory for the backend-regex surface. Focused idea
296 closed the fused compare-branch operand-form owner after improving the
focused scope to 23/27. The next useful step is to refresh or classify the
remaining backend-regex buckets and split a new focused owner only if the
evidence supports a real semantic capability.

## Execution Rules

- Keep classification findings in `todo.md` unless a durable focused split or
  deactivation note is needed.
- Create a new `ideas/open/*.md` before implementation starts for any focused
  semantic owner.
- Preserve closed-owner boundaries unless generated-code or proof evidence
  contradicts them.
- Keep canonical proof logs as `test_before.log` and `test_after.log`; do not
  leave extra root-level `.log` files.

## Steps

### Step 1: Refresh The Remaining Backend Surface

Capture or review the supervisor-selected backend-regex evidence from the main
build tree, with timeout and stale-process precautions if broad runtime tests
are run.

Completion check: `todo.md` records the command or accepted existing evidence,
the selected count, pass/fail count, and current failing test list.

### Step 2: Classify Remaining Failures

Separate local backend/unit failures from external AArch64 c-testsuite
failures, then group the external failures by failure source: machine-printer,
`lir_to_bir` admission, runtime mismatch, timeout/hang, or another observed
owner.

Completion check: `todo.md` records the grouped inventory and explicitly notes
whether any evidence contradicts closed owners 285 through 296.

### Step 3: Choose The Next Focused Owner

Select the highest-value tractable semantic owner, or record why no focused
owner is ready to split.

Completion check: either a new `ideas/open/*.md` focused owner exists with
reviewer reject signals, or `todo.md` explains why the umbrella inventory is
blocked or needs another evidence pass.

### Step 4: Switch Away Before Implementation

If a focused owner is split, deactivate this umbrella runbook and activate the
focused owner before any implementation work starts.

Completion check: lifecycle state points at the focused source idea, and
`ideas/open/295_backend_regex_failure_family_inventory.md` has a compact
durable deactivation note preserving the owner decision and remaining buckets.
