# Pre-existing Baseline Failure-Family Decomposition Runbook

Status: Active
Source Idea: ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md

## Purpose

Finish the baseline provenance blocker for 830 by rerunning the unchanged
comparable full-suite gate after the ordered repair/decomposition successors.

## Goal

Determine whether the current tree clears the accepted 3038/3038 comparable
baseline problem, then either return 830 to unchanged Step 3 or classify the
new current failure family without weakening the baseline contract.

## Core Rule

Do not clear this blocker from focused proof alone. Baseline clearance requires
a current comparable full-suite candidate accepted against `test_baseline.log`
with no pass-count regression or new failure family.

## Read First

- `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`
- `ideas/closed/832_hir_aggregate_owner_function_parameter_crash_repair.md`
- `ideas/closed/833_lir_truthiness_lhs_parameter_authority_completion.md`
- `ideas/closed/834_lir_owned_type_spec_module_owner_canonicalization_blocker.md`
- `ideas/closed/836_lir_remaining_aggregate_owner_rejection_decomposition_blocker.md`
- `scripts/plan_review_state.py`

## Current Targets And Scope

- Resume at Step 4 after closed 836 intentionally concluded with no current
  residual aggregate-owner reproduction.
- Run the unchanged full-suite comparable-baseline gate against the accepted
  `test_baseline.log`.
- If accepted, return only 830 to unchanged Step 3.
- If rejected, classify the current first-owner failure family before creating
  or switching to any successor.

## Non-Goals

- Do not edit implementation code in this route.
- Do not change tests, expectations, unsupported markers, allowlists, filters,
  harness behavior, or baseline parsing.
- Do not reopen 832, 833, 834, 836, or 837 from stale evidence.
- Do not perform 830 direct-call work, 829 authority work, Raw-BIR, or generic
  call work.

## Working Model

Steps 1 through 3 are historical and complete. The only active work is the
comparable full-suite gate that decides whether the parent 830 baseline problem
is cleared on the current tree.

## Execution Rules

- Preserve `test_baseline.log` as the accepted comparison baseline unless the
  helper accepts a non-regressing candidate.
- Generate `test_baseline.new.log` from the current tree.
- Use the repo baseline helper to compare or accept/reject; do not hand-edit
  baseline logs.
- If the gate rejects, update `todo.md` with exact counts, representative
  first diagnostics, and the next lifecycle decision. Do not implement a mixed
  repair inside 831.

## Steps

### Step 4 - Obtain Comparable Baseline Proof And Return 830

Goal: run the current full-suite comparable-baseline gate and decide whether
831 can close or must split a new current blocker.

Primary target: `test_baseline.log`, `test_baseline.new.log`, and the repo
baseline comparison helper.

Actions:

- Generate a current full-suite baseline candidate.
- Compare it with the accepted `test_baseline.log`.
- If non-regressing, accept the baseline candidate and record return to
  `ideas/open/830_lir_direct_call_structured_argument_identity_prerequisite.md`
  at unchanged Step 3.
- If regressing, reject the candidate, classify the current failure family,
  and route lifecycle to the smallest separate successor instead of returning
  830.

Completion check:

- The supervisor has accepted either comparable-baseline clearance and a 830
  return, or an explicit rejected-gate classification with a named lifecycle
  successor.
