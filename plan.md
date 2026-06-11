# Phase E Route 7 Comparison View Consumer Migration Runbook

Status: Active
Source Idea: ideas/open/188_phase_e_route7_comparison_view_consumer_migration.md

## Purpose

Move one AArch64 comparison or branch operand provenance reader onto the Route
7 comparison/materialized-condition view while keeping target branch and ALU
policy in AArch64 lowering.

## Goal

Select one bounded comparison consumer, make it read Route 7 provenance where
available, and prove parity plus fallback behavior without weakening existing
prepared comparison coverage.

## Core Rule

Use Route 7 as a semantic provenance input only. Do not move branch spelling,
fused legality, condition-code selection, hazard handling, emitted-register
state, branch/compare record ordering, ALU result storage, or return-chain
lookup into BIR.

## Read First

- `ideas/open/188_phase_e_route7_comparison_view_consumer_migration.md`
- `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`
- Existing Route 7 comparison provenance records, facades, and tests
- The selected AArch64 fused-compare, materialized-condition, branch operand,
  or scalar comparison consumer

## Current Scope

- One AArch64 fused-compare, materialized-condition, branch operand, or scalar
  comparison provenance reader.
- Route 7 condition/comparison provenance records or a validated facade keyed
  by the selected consumer.
- Prepared comparison helpers, scalar producer/select-chain fallbacks, Route 7
  facade validation, and fused-compare oracle tests as public comparison
  surfaces.

## Non-Goals

- Do not change branch spelling, fused legality, condition-code selection,
  hazard handling, emitted-register state, final branch/compare record
  ordering, ALU result storage, or return-chain operand recovery.
- Do not delete prepared comparison helpers.
- Do not fold return-chain lookup into Route 7.
- Do not claim route-wide comparison helper contraction from one selected
  consumer.
- Do not add broad BIR scans, predecessor rescans, name matching, or
  testcase-shaped shortcuts.

## Working Model

- Route 7 provenance is available when the selected consumer can key into a
  validated comparison/materialized-condition view.
- Prepared comparison helpers and scalar producer/select-chain fallbacks remain
  the compatibility path when Route 7 is absent or invalid for the selected
  consumer.
- AArch64 lowering remains the owner of target policy and emitted machine
  records.

## Execution Rules

- Keep each step behavior-preserving except for the intended provenance source
  switch at the selected consumer.
- Prefer small, reviewable changes that expose semantic Route 7 lookup paths
  instead of spreading new ad hoc lookup logic.
- Preserve invalid-reference and absent-route fallback behavior.
- Keep tests strong: do not downgrade supported-path expectations or remove
  existing comparison/fallback coverage.
- Fresh build or compile proof is required for every code-changing step; the
  supervisor owns broader validation selection.

## Ordered Steps

### Step 1: Select and Map One Comparison Consumer

Goal: identify the bounded AArch64 consumer that will move to Route 7.

Primary target: one fused-compare, materialized-condition, branch operand, or
scalar comparison provenance reader.

Actions:

- Inspect existing Route 7 comparison provenance records, validation facade
  entry points, and public tests.
- Inspect nearby prepared comparison helper and scalar producer/select-chain
  fallback use at the selected consumer.
- Record in `todo.md` which consumer was selected, why it is bounded, and which
  existing tests cover its prepared and fallback behavior.
- Do not edit implementation during this mapping-only step unless the executor
  is explicitly delegated a code-changing packet.

Completion check:

- `todo.md` names the selected consumer, its Route 7 lookup key, its fallback
  path, and the narrow proof command the supervisor should delegate next.

### Step 2: Thread Route 7 Provenance Into the Selected Consumer

Goal: make the selected consumer read Route 7 provenance where available.

Primary target: the selected AArch64 comparison consumer and its nearest
existing Route 7 facade or validated record access point.

Actions:

- Add or reuse the smallest facade/query needed for the selected consumer to
  read Route 7 comparison/materialized-condition provenance.
- Preserve prepared comparison helper and scalar producer/select-chain fallback
  behavior for absent or invalid Route 7 data.
- Keep AArch64 branch policy, ALU result policy, and machine-record formation
  target-owned.
- Avoid broad BIR scans, predecessor rescans, name matching, and target-specific
  Route 7 shortcuts.

Completion check:

- The selected consumer prefers valid Route 7 provenance and falls back through
  the existing prepared/scalar paths when Route 7 is absent or invalid.
- A fresh build or compile proof is recorded in `todo.md`.

### Step 3: Prove Parity, Fallback, and Invalid-Reference Cases

Goal: prove the selected consumer's Route 7 behavior across the required
comparison surfaces.

Primary target: Route 7 facade validation, prepared comparison helper tests,
scalar producer/select-chain fallback tests, and fused-compare oracle tests.

Actions:

- Add or update tests for fused-compare provenance equality.
- Add or update tests for materialized-condition provenance equality when the
  selected consumer covers materialized conditions.
- Add or update tests for unfused fallback, absent-route fallback, and invalid
  Route 7 reference handling.
- Preserve or strengthen existing prepared comparison and fallback contracts.

Completion check:

- Tests prove provenance equality for the selected fused/materialized path and
  preserve unfused fallback, absent-route, and invalid-reference behavior.
- The delegated narrow proof command is green and recorded in `todo.md`.

### Step 4: Final Route-Quality Check

Goal: make the slice acceptance-ready for supervisor review.

Primary target: the diff, `todo.md` proof notes, and the selected comparison
consumer's tests.

Actions:

- Check that return-chain lookup remains outside Route 7.
- Check that target branch policy and ALU machine-record formation did not move
  into BIR.
- Check that no tests were weakened and no invalid-reference or fallback case
  was removed.
- Ask the supervisor to run any broader proof needed for the touched surface.

Completion check:

- `todo.md` records the final proof command and result.
- The diff demonstrates one selected consumer migration, not a route-wide
  contraction claim.
