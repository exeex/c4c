# AArch64 Prior Preservation Baseline Drift Close Validation Runbook

Status: Active
Source Idea: ideas/open/04_aarch64_prior_preservation_baseline_drift.md

## Purpose

Finish the lifecycle for the prior-preservation baseline drift idea by turning
the remaining open state into explicit validation and close-readiness work.

## Goal

Produce fresh canonical proof that the repaired AArch64 call-boundary
prior-preservation/source-selection path no longer regresses, then hand the
idea back for close handling.

## Core Rule

Treat this as a validation and closure-readiness route. Do not reopen broad
AArch64 calls architecture, do not revive the retired broad fallback, and do
not claim progress through c_testsuite expectation weakening or named-test
shortcuts.

## Read First

- `ideas/open/04_aarch64_prior_preservation_baseline_drift.md`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- commits `12d68c4d6`, `898884db5`, and `f3d8611c7`
- `test_before.log` and `test_after.log`, if present, but only as reusable
  evidence when their commands match this plan's validation scope
- baseline logs named by the source idea:
  - `log/baseline_08a7f725a3f5820506a517f58ae9b9012bc20b7e.log`
  - `log/baseline_11b33c8d0586b44d163de6b74bff9c33957aab0b.log`

## Current Targets

- The prior-preservation/source-selection repair in
  `src/backend/mir/aarch64/codegen/calls_moves.cpp`.
- Focused AArch64 instruction-dispatch coverage added for the repaired path.
- Representative c_testsuite AArch64 backend cases recorded in the source
  idea deactivation note:
  - `backend_aarch64_instruction_dispatch`
  - `c_testsuite_aarch64_backend_src_00173_c`
  - `c_testsuite_aarch64_backend_src_00179_c`
  - `c_testsuite_aarch64_backend_src_00186_c`
  - `c_testsuite_aarch64_backend_src_00187_c`
- Canonical regression proof needed before the source idea can be closed.

## Non-Goals

- Do not edit implementation files during the validation packet unless the
  supervisor explicitly replaces this runbook after a real regression is found.
- Do not move or close the source idea as part of executor work.
- Do not treat the separate `00181`, `00216`, or `00204` failure families as
  unresolved prior-preservation drift.
- Do not downgrade, delete, mark unsupported, or rewrite expectations to make
  validation pass.
- Do not collapse this route into broad AArch64 call-plan cleanup.

## Working Model

- The source idea records that the repair runbook completed and that the
  remaining same-family representative cases passed.
- The idea remained open because close-grade canonical regression evidence was
  unavailable at deactivation time, not because a new lowering rule was already
  known to be missing.
- The active execution should therefore first prove the existing repair. If
  proof fails on the prior-preservation/source-selection path, record the
  failure as a blocker instead of applying an ad hoc testcase-shaped fix.
- Existing `test_after.log` content may belong to a different lifecycle route;
  regenerate or append only under the supervisor-selected command for this
  active plan.

## Execution Rules

- Keep each packet evidence-focused and write fresh results to the canonical
  log path selected by the supervisor, normally `test_after.log`.
- Preserve the distinction between prior-preservation/source-selection drift
  and the split failure families named in the source idea.
- Prefer build proof followed by the focused representative subset before any
  broader guard.
- Escalate to close-grade regression guard or broader CTest only after the
  focused path is green.
- Treat unsupported downgrades, weaker contracts, expectation rewrites, broad
  fallback restoration, and named-test special cases as blocking route-quality
  failures.

## Ordered Steps

### Step 1: Reconstruct Close-Proof Scope

Goal: identify the exact evidence gap that kept the source idea open and the
validation commands needed to close it.

Primary targets:

- `ideas/open/04_aarch64_prior_preservation_baseline_drift.md`
- baseline logs named by the source idea
- current canonical `test_before.log` and `test_after.log`, if present
- ctest inventory for the representative prior-preservation cases

Actions:

- Confirm there is no implementation work already required by the source idea
  beyond validation and closure-readiness.
- Identify whether existing canonical logs can be reused or whether this plan
  needs fresh matching before/after proof.
- Confirm the representative focused test names are available in the current
  build tree.
- Record the supervisor-selected proof command in `todo.md` before execution
  advances to Step 2.

Completion check:

- `todo.md` names the focused proof command, whether canonical logs are reused
  or regenerated, and any blocker that prevents close-grade validation.

### Step 2: Prove the Focused Prior-Preservation Path

Goal: show the repaired prior-preservation/source-selection path is still green
on the representative tests from the source idea.

Primary targets:

- `backend_aarch64_instruction_dispatch`
- `c_testsuite_aarch64_backend_src_00173_c`
- `c_testsuite_aarch64_backend_src_00179_c`
- `c_testsuite_aarch64_backend_src_00186_c`
- `c_testsuite_aarch64_backend_src_00187_c`

Actions:

- Run the supervisor-selected build proof.
- Run the focused representative subset and write or append results to the
  canonical proof log.
- If a representative case fails, stop and record whether the failure is the
  original prior-preservation/source-selection family or a separate family.
- Do not patch code in this step unless the supervisor replaces the validation
  route with an implementation route.

Completion check:

- Build proof and the focused representative subset pass, with the exact
  command and result recorded in `todo.md`.

### Step 3: Run Close-Grade Regression Guard

Goal: provide enough regression evidence for the plan owner to decide whether
the source idea can close.

Primary targets:

- canonical regression logs: `test_before.log` and `test_after.log`
- the supervisor-selected close-grade command, such as matching
  `c4c-regression-guard`, full CTest, or a broader AArch64/backend subset

Actions:

- Use matching before/after proof when the supervisor requires a regression
  comparison.
- Run the broader validation command selected by the supervisor after the
  focused representative subset is green.
- Keep failures classified against the source idea: prior-preservation drift,
  split known separate family, or unrelated current-tree regression.
- Preserve canonical log names and avoid leaving extra root-level `.log` files.

Completion check:

- `todo.md` records a close-grade pass or a concrete blocker with the failing
  command, failing tests, and family classification.

### Step 4: Prepare Close Handoff

Goal: leave the active state ready for the plan owner to close or explicitly
  reject closure.

Primary targets:

- `todo.md`
- `plan.md`
- `ideas/open/04_aarch64_prior_preservation_baseline_drift.md`

Actions:

- Summarize the focused proof and close-grade regression result in `todo.md`.
- Confirm no implementation files, tests, or expectations changed during this
  validation route.
- Ask the supervisor to delegate close handling to the plan owner if the source
  idea acceptance criteria and regression guard are satisfied.
- If validation is blocked, keep the blocker in `todo.md` instead of editing
  the source idea.

Completion check:

- The next lifecycle decision is unambiguous: close-ready with proof, or
  blocked with an exact validation or route-quality reason.
