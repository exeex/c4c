# AArch64 Scalar Cast Stack-Homed Register Source Publication Closure Runbook

Status: Active
Source Idea: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md

## Purpose

Refresh and close, if the close gate permits, the parked scalar-cast
stack-homed register source-publication idea.

## Goal

Confirm the repaired selected scalar cast path still has no live first bad
fact, then either close the source idea through the lifecycle gate or leave a
precise parked handoff.

## Core Rule

Do not reopen scalar cast source-publication implementation work unless fresh
generated or focused backend evidence shows the old structured register-source
diagnostic, or an equivalent stack-homed scalar cast source publication
failure, is live again.

## Read First

- `ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md`
- `test_baseline.log`
- `scripts/c4c_regression_guard.py` or the repo-local regression guard entry
  used by `c4c-regression-guard`
- Focused backend tests named in the source idea lifecycle notes

## Current Scope

- Refresh the focused scalar-cast source-publication proof on the current tree.
- Compare the observed first bad fact, if any, with the source idea scope.
- Run the close-time regression gate if the source idea remains satisfied.
- Update lifecycle state according to the gate result.

## Non-Goals

- Do not edit implementation files.
- Do not change expectations, unsupported classifications, CTest
  registration, runner behavior, timeout policy, or proof-log policy.
- Do not broaden into local storage/writeback sizing, runtime value
  correctness, frame layout, compare lowering, aggregates, variadics, or
  semantic `lir_to_bir` admission without a separate source idea.
- Do not reactivate the just-failed block-label emission closure route under
  this plan.

## Working Model

The source idea repaired publication of structured register sources for
selected scalar casts when the original source is stack-homed and a prepared
consumer stack-to-register move exists. Latest lifecycle notes say the focused
proof and `00143` representative were green, while closure was deferred only
because the strict monotonic close gate rejected matching green logs.

## Execution Rules

- Treat `todo.md` as the packet scratchpad and keep this runbook stable unless
  the route itself changes.
- Preserve canonical log names when producing regression evidence:
  `test_before.log` and `test_after.log`.
- If a fresh failure is outside this source idea, do not absorb it here; record
  the classification for supervisor handoff.
- If closure is rejected only by the monotonic guard, park rather than
  inventing implementation work.

## Step 1: Refresh Scalar Cast Source-Publication Proof

Goal: verify whether the stack-homed scalar cast structured source diagnostic
is still absent.

Primary target: scalar cast focused backend coverage and
`c_testsuite_aarch64_backend_src_00143_c`.

Actions:

- Build the current tree.
- Run the focused scalar-cast subset from the latest lifecycle note:
  `backend_aarch64_scalar_cast_records`,
  `backend_aarch64_prepared_scalar_cast_records`, and
  `c_testsuite_aarch64_backend_src_00143_c`.
- If any test fails, classify the first bad fact against this source idea's
  scope before proposing implementation work.

Completion check:

- Focused proof is recorded in `todo.md`, including whether the old scalar
  cast structured register-source printer diagnostic is absent.

## Step 2: Run Close Gate Or Park With Classification

Goal: decide whether the source idea can close from the current evidence.

Primary target: lifecycle state for
`ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md`.

Actions:

- If Step 1 is green and no in-scope first bad fact remains, run the required
  close-time regression guard using matching before/after logs.
- If the guard accepts closure, move the source idea through the lifecycle
  close flow.
- If the guard rejects closure without a new in-scope failure, deactivate the
  runbook and leave a compact parked note in the source idea only if needed.
- If Step 1 exposes an out-of-scope first bad fact, stop and report the
  lifecycle handoff target instead of expanding this idea.

Completion check:

- Exactly one of these is true: the idea is closed, the runbook is deactivated
  with source intent preserved, or a blocker is reported with the exact
  lifecycle ambiguity.
