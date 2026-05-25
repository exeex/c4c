# AArch64 Prior Preservation Baseline Drift Runbook

Status: Active
Source Idea: ideas/open/04_aarch64_prior_preservation_baseline_drift.md

## Purpose

Repair the baseline drift introduced by `11b33c8d0586` without reopening the
old broad prior-preservation fallback.

## Goal

Restore the missing AArch64 call argument source-selection behavior for the
37-test c_testsuite regression family.

## Core Rule

Fix the semantic lowering rule. Do not special-case test names, weaken
expectations, or restore fallback behavior that the regression commit was
trying to retire.

## Read First

- `log/baseline_08a7f725a3f5820506a517f58ae9b9012bc20b7e.log`
- `log/baseline_11b33c8d0586b44d163de6b74bff9c33957aab0b.log`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

## Current Targets

- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- Focused AArch64 dispatch/c_testsuite tests needed to prove the fix

## Non-Goals

- Do not perform broad call-plan refactoring.
- Do not edit unrelated AArch64 lowering families.
- Do not rewrite baseline logs as the fix.

## Steps

### Step 1: Reproduce And Inspect

- Inspect the `11b33c8d0586` diff in `calls_moves.cpp`.
- Identify which explicit source-selection cases still need to fall back to a
  prepared prior-preservation value and which must not.
- Run a narrow failing representative from the 37-test list.

Completion check: one failing test or narrower unit reproduces the drift, and
the missing lowering path is identified.

### Step 2: Repair Prior-Preservation Source Selection

- Update `calls_moves.cpp` so complete explicit selections can consume the
  prepared prior-preservation source they require.
- Preserve rejection of incomplete explicit stack prior-preservation selections.
- Avoid rederiving unrelated prior homes when the selection kind is not a
  prior-preservation request.

Completion check: focused dispatch test coverage passes and the representative
  c_testsuite failure is fixed.

### Step 3: Validate Regression Family

- Run the narrow proof for the representative failing test(s).
- Run a broader AArch64 backend c_testsuite subset if the narrow proof passes.
- Record proof in `todo.md`.

Completion check: proof logs show no regression for the repaired family.
