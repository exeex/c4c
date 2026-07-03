# Prepared Move-Bundle Widening Stack Authority Runbook

Status: Active
Source Idea: ideas/open/565_prepared_move_bundle_widening_stack_authority.md

## Purpose

Repair the prepared move-bundle classifier authority for conversion-adjacent
integer widening moves between stack slots so downstream RV64 lowering is only
attempted after prepared facts are explicit.

## Goal

Move `src/20010224-1.c` and `src/pr87623.c` off the
`unsupported_prepared_move_bundle_classification` path where stack-slot source
to stack-slot destination widening moves currently report `authority=none`.

## Core Rule

Fix the prepared classifier boundary. Do not route these rows to RV64 object
emission while `prepared_move_bundle_classifier` still lacks authority, and do
not change diagnostics, unsupported markers, allowlists, expected outputs, or
pass/fail accounting as progress.

## Read First

- `ideas/open/565_prepared_move_bundle_widening_stack_authority.md`
- `build/agent_state/548_step4_move_bundle_classification/classification.md`
- `build/rv64_gcc_c_torture_backend/src_20010224-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_pr87623.c/case.log`
- Prepared move-bundle classifier code under `src/backend/prepared/`
- Existing backend tests that assert prepared move-bundle authority

## Scope

- Conversion-adjacent integer widening moves from one stack slot to another.
- Source widths represented by the current evidence, including `i16 -> i32`
  and `i8 -> i32`.
- Prepared authority facts or an explicit prepared split into conversion plus
  stack-destination move facts.
- Representative RV64 proof after focused prepared/backend coverage is in
  place.

## Non-Goals

- Do not implement RV64 consumption before prepared authority is published.
- Do not generalize to arbitrary memory-to-memory copies unrelated to
  conversion-adjacent stack-slot widening.
- Do not work on F128, long-double, F64 global memory, callee-saved FPR frame
  slots, or unrelated scalar/FPR residuals.
- Do not special-case representative filenames, exact source widths, event
  names, or diagnostic strings.

## Working Model

The representatives reach the prepared move-bundle classifier with coherent
stack-slot source and destination shapes, but the classifier rejects the
conversion-adjacent widening form with `authority=none`. The first repair
target is prepared authority publication or a prepared-level split that makes
the conversion semantics explicit before RV64 sees the bundle.

## Execution Rules

- Start by confirming the current representative diagnostics and the exact
  classifier branch that emits `authority=none`.
- Add focused coverage before or with the repair so the semantic widening
  authority shape is checked directly.
- Keep RV64 changes out of scope unless inspection proves prepared authority
  already exists and the first owner has moved.
- Preserve fail-closed behavior for unsupported move-bundle shapes.
- Record proof commands, log paths, and any downstream residual owner in
  `todo.md`.

## Steps

### Step 1: Inspect Widening Move-Bundle Boundary

Goal: Reconfirm the current first bad fact for both representatives and locate
the prepared classifier branch that rejects stack-slot to stack-slot widening
moves.

Primary targets:

- `src/20010224-1.c`
- `src/pr87623.c`

Actions:

- Re-run or inspect the representative backend logs and capture the current
  `unsupported_prepared_move_bundle_classification` evidence.
- Trace the stack-slot source, stack-slot destination, and width facts into the
  prepared move-bundle classifier.
- Identify whether the correct repair is one semantic widening authority shape
  or an explicit prepared split into conversion plus stack-destination facts.
- Record whether the boundary is still prepared producer-owned or has moved.

Completion check:

- `todo.md` records current diagnostics, log paths, owning code boundary, and a
  concrete next packet for focused coverage or repair.

### Step 2: Add Focused Widening Authority Coverage

Goal: Add tests that prove prepared move-bundle authority for conversion-
adjacent integer widening moves between stack slots.

Actions:

- Add focused prepared/backend coverage for stack-slot source to stack-slot
  destination widening using semantic width and storage facts.
- Cover both current representative width families, or a width-general rule
  that demonstrably subsumes `i8 -> i32` and `i16 -> i32`.
- Keep expected rejection behavior for unsupported move-bundle shapes
  unchanged.

Completion check:

- Focused tests fail before the repair or directly prove the repaired prepared
  authority contract, and backend proof is recorded in `todo.md`.

### Step 3: Repair Prepared Widening Authority

Goal: Publish explicit prepared authority for the supported stack-slot
widening shape, or split it into explicit prepared conversion and
stack-destination facts before RV64 consumption.

Actions:

- Implement the minimal prepared classifier repair at the verified owner.
- Avoid RV64 inference of missing move authority and avoid broad move-bundle
  rewrites.
- Preserve existing behavior for non-stack destinations, unsupported source
  kinds, non-integer conversions, and unrelated memory-to-memory moves.
- Run focused tests and the backend subset selected by the supervisor.

Completion check:

- Backend tests pass, focused widening authority coverage passes, and
  `todo.md` records the files changed plus proof commands.

### Step 4: Reconcile Representatives And Residual Owners

Goal: Prove whether both representatives moved off the prepared move-bundle
classification rejection and classify any later failure.

Actions:

- Re-run both representative allowlist rows with verbose failure logs.
- Confirm the old `authority=none` classifier rejection no longer appears.
- If either row advances to a downstream owner, record the exact owner and
  leave distinct work to a separate source idea.
- Recommend close, split, or continue for this source idea.

Completion check:

- `todo.md` records representative outcomes, log paths, residual owners if any,
  and the lifecycle recommendation for this active idea.
