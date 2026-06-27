# RV64 gcc_torture Prepared Module Shape Classification Closeout Runbook

Status: Active
Source Idea: ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md
Activated after: ideas/closed/404_prepared_wide_rematerialized_immediate_admission.md

## Purpose

Perform the final lifecycle audit for the reopened RV64 gcc_torture prepared
module shape umbrella now that the reopened child chain has closed through
404 and its split follow-ups.

## Goal

Confirm whether idea 354's umbrella acceptance criteria are satisfied: prepared
module shape failures were classified from scan data, repairable backend
buckets were given child owners, non-backend buckets were documented, and all
generated child ideas are closed or intentionally superseded.

## Core Rule

This is a classification and lifecycle closeout plan. Do not implement backend
repairs, weaken the RV64 gcc_torture runner, mark cases unsupported, or create
testcase-shaped shortcuts from this umbrella.

## Read First

- `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
- `review/354_prepared_shape_classification.md`
- `review/354_reopen_classification_20260626.md`
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- Closed child ideas 395 through 411 when checking reopened residual owners.

## Current Scope

- Source umbrella: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
- Current open inventory: only 354 is open.
- Reopened child set from the source idea: 395, 396, 397, 398, 399, 400,
  401, and 402.
- Split/follow-up closures observed after reopened classification: 403, 404,
  405, 406, 407, 408, 409, 410, and 411.

## Non-Goals

- Do not run implementation packets from 354.
- Do not edit backend/compiler code.
- Do not mutate source ideas other than 354 closure notes if lifecycle review
  later accepts closure.
- Do not rerun the full 1467-case scan unless the supervisor explicitly asks
  for a broader refresh.
- Do not claim new compiler capability from classification-only changes.

## Working Model

Idea 354 was reopened after a fresh scan found 1256 failures. It produced
new child owners for RV64 prepared-object buckets and runtime mismatches. The
last remaining actionable child, 404, is now closed. The active task is to
verify whether the reopened umbrella can close or whether a residual owner is
still missing.

## Execution Rules

- Prefer existing scan and proof artifacts for the audit unless they are
  missing or inconsistent.
- Check only `ideas/open/` for live work; treat `ideas/closed/` as archive
  for closure audit evidence.
- Preserve evidence at the lowest layer: audit findings in `todo.md`, closure
  notes in 354 only if the source idea closes.
- If a live unowned residual is found, create or request a separate open idea
  rather than expanding 354 into implementation work.
- Before closing 354, run the plan-owner close gate with matching canonical
  regression logs.

## Step 1: Audit Reopened Child Closure

Goal: prove every reopened 354 child or split follow-up is closed or
intentionally superseded.

Actions:

- List the current `ideas/open/` inventory and confirm whether any child
  owners remain open besides 354.
- Audit 395 through 402 from the reopened classification pass.
- Audit split/follow-up closures 403 through 411 and note which parent bucket
  each resolved.
- Record any missing or ambiguous child owner in `todo.md`.

Completion check:

- `todo.md` records the closed/superseded status of the reopened child chain.
- Any still-open or missing child owner is named with the precise lifecycle
  blocker.

## Step 2: Verify Residual Classification Coverage

Goal: decide whether the remaining scan failures are outside the original
opaque prepared-module-shape umbrella or already routed.

Actions:

- Review `review/354_reopen_classification_20260626.md` and current scan
  summary artifacts if present.
- Confirm the original opaque prepared-module-shape buckets and the reopened
  RV64 prepared-object/runtime buckets have child ownership.
- Confirm semantic `lir_to_bir` handoff failures and timeouts remain
  documented as outside this RV64 prepared-shape umbrella unless fresh
  evidence proves otherwise.
- Do not create implementation work from 354; split only if a concrete unowned
  residual owner is discovered.

Completion check:

- `todo.md` records whether 354 acceptance is closure-ready or blocked by a
  specific residual owner.
- Any requested follow-up idea has a narrow source and reviewer reject signals.

## Step 3: Close Or Defer 354

Goal: perform final lifecycle handling for the umbrella.

Actions:

- If Step 1 and Step 2 prove source completion, run the required close gate
  and ask plan-owner close handling to move 354 to `ideas/closed/`.
- If completion is blocked, preserve the exact blocker in `todo.md` and keep
  354 active or route to the correct new owner.
- Do not close merely because the current runbook is exhausted.

Completion check:

- Either 354 is closed by lifecycle rules with passing close gate evidence, or
  `todo.md` clearly states the blocker and next owner.
