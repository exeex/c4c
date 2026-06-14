# 263 Phase F3 full-suite timeout baseline regression diagnosis

Status: Active
Source Idea: ideas/open/263_phase_f3_full_suite_timeout_baseline_regression_diagnosis.md

## Purpose

Diagnose and repair the full-suite baseline regression where
`test_baseline.new.log` dropped from 3428/3428 to 3427/3428 because
`c_testsuite_aarch64_backend_src_00040_c` timed out.

## Goal

Determine whether the timeout is environmental/flaky or exposes a real
backend/runtime regression from recent Phase F3 convergence work, then produce
matching proof sufficient for supervisor baseline handling.

## Core Rule

Do not accept, delete, weaken, hide, or route around
`test_baseline.new.log` while it reports 3427/3428. Extract the timeline and
failure evidence first, then repair the underlying cause if one exists.

## Read First

- `ideas/open/263_phase_f3_full_suite_timeout_baseline_regression_diagnosis.md`
- `test_baseline.log`
- `test_baseline.new.log`
- recent `git log --oneline --decorate` entries around ideas 248 through 262
- closure notes for ideas 248 through 262
- any referenced `test_before.log` / `test_after.log` evidence
- `log/` baseline artifacts, if present, sorted by commit/time

## Current Targets

- Narrow failing test:
  `c_testsuite_aarch64_backend_src_00040_c`
- Accepted baseline:
  `test_baseline.log`
- Rejected candidate baseline:
  `test_baseline.new.log`
- Recent Phase F3 surfaces that could affect AArch64 runtime, c-testsuite
  execution, selected `LoadLocal`, compare-join, store-source, publication,
  module structural helpers, or target output

## Non-Goals

- Do not accept a 3427/3428 candidate baseline.
- Do not delete `test_baseline.new.log` before extracting evidence.
- Do not weaken timeout, labels, supported status, expected output, or test
  inclusion to claim progress.
- Do not broaden into prepared/BIR retirement or unrelated portability cleanup.
- Do not treat focused Phase F3 proof logs as sufficient full-suite baseline
  proof.

## Working Model

This is a diagnosis-first repair runbook. The timeline audit establishes the
last known 3428/3428 point and first known 3427/3428 point before code changes.
Only after that audit should execution reproduce the narrow timeout, inspect
likely Phase F3 changes, and patch a root cause if a real regression is found.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Preserve chronological evidence from logs and commit history before changing
  implementation files.
- Start narrow with the named c-testsuite timeout, then broaden only after
  diagnosis.
- If a patch is needed, prove it with matching before/after narrow evidence and
  supervisor-approved broader baseline or full-suite proof.
- Reject testcase-shaped fixes, expectation rewrites, supported-status changes,
  or blind timeout increases.

## Steps

### Step 1: Timeline Log Audit

Goal: Identify the last known 3428/3428 baseline and the first later
3427/3428 candidate in chronological order before any code changes.

Primary targets:

- `test_baseline.log`
- `test_baseline.new.log`
- recent `git log --oneline --decorate` entries around ideas 248 through 262
- closure notes for ideas 248 through 262
- referenced `test_before.log` / `test_after.log` evidence
- `log/` baseline artifacts, if present

Actions:

- Inspect the accepted and rejected baseline logs without modifying them.
- Review recent commits and closure notes around ideas 248 through 262.
- Trace any referenced canonical proof logs from those notes.
- Sort available `log/` baseline artifacts by commit or timestamp.
- Record in `todo.md` the time-ordered sequence and exact transition point, or
  state why available logs cannot identify it.

Completion check:

- `todo.md` records the last known 3428/3428 point, the first known 3427/3428
  point, the evidence paths inspected, and whether the transition point is
  exact or bounded by missing evidence.

### Step 2: Narrow Timeout Reproduction

Goal: Reproduce or classify the
`c_testsuite_aarch64_backend_src_00040_c` timeout without changing baseline
contracts.

Primary target:

- `ctest --test-dir build -R '^c_testsuite_aarch64_backend_src_00040_c$' --output-on-failure`

Actions:

- Run the narrow failing test under the supervisor-delegated proof command.
- Repeat if needed to distinguish deterministic failure from environmental
  flake.
- Capture whether timeout behavior is stable, intermittent, data-dependent, or
  stale-artifact related.
- Record the command, result, and reproduction classification in `todo.md`.

Completion check:

- The timeout is reproduced, or non-reproducibility is supported by rerun
  evidence strong enough for the supervisor to decide the next proof scope.

### Step 3: Root-Cause Inspection

Goal: Determine whether recent Phase F3 work introduced a real backend/runtime
design gap.

Primary targets:

- AArch64 runtime and c-testsuite execution surfaces
- selected `LoadLocal`
- compare-join behavior
- store-source and publication paths
- module structural helpers
- target output paths

Actions:

- Compare the bounded regression window from Step 1 against relevant Phase F3
  changes.
- Inspect only surfaces plausibly connected to the timeout.
- Avoid broad prepared/BIR retirement or unrelated portability cleanup.
- Record candidate root causes and rejected hypotheses in `todo.md`.

Completion check:

- `todo.md` identifies either a concrete root cause to fix, an environmental
  classification with evidence, or a blocker with the missing evidence needed.

### Step 4: Repair Real Regression

Goal: Fix the root cause if Step 3 identifies a real regression.

Actions:

- Patch the semantic/backend/runtime cause, not the test expectation or
  timeout policy.
- Keep the change narrowly scoped to the diagnosed failure family.
- Preserve Phase F3 ownership boundaries and avoid target-policy migration into
  BIR.

Completion check:

- The narrow timeout no longer reproduces under the matching proof command, and
  the diff does not weaken test contracts or hide the failure.

### Step 5: Matching Proof and Baseline Resolution

Goal: Produce proof sufficient for supervisor baseline handling.

Actions:

- Run matching before/after proof for the narrow failing test when a fix lands.
- Escalate to supervisor-approved broader baseline or full-suite proof before
  claiming 3428/3428 restoration.
- Leave `test_baseline.new.log` rejected if proof cannot restore 3428/3428,
  with the blocker documented in `todo.md`.
- Do not accept the candidate baseline outside the supervisor baseline flow.

Completion check:

- Either a fresh 3428/3428 candidate is ready for supervisor acceptance, or the
  rejected candidate remains with a documented blocker and evidence trail.
