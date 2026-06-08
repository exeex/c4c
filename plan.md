# AArch64 i128 Shift Support Completeness Runbook

Status: Active
Source Idea: ideas/open/129_aarch64_i128_shift_support_completeness.md

## Purpose

Complete or explicitly contract AArch64 i128 shift lowering support for
shift-by-64-or-more and variable-count shifts.

## Goal

Make each targeted i128 shift category either work through semantic AArch64
lowering/printing with focused backend proof, or fail through an explicit,
narrow unsupported diagnostic that matches the current capability contract.

## Core Rule

Keep this work target-local to AArch64 i128 shift completeness. Do not move
AArch64 shift opcode spelling, lane mechanics, register-pair spelling, helper
ABI/resource policy, carrier ownership, f128 behavior, or broad cleanup into
this route.

## Read First

- `ideas/open/129_aarch64_i128_shift_support_completeness.md`
- `src/backend/mir/aarch64/codegen/i128_ops.cpp`
- AArch64 machine instruction and printer paths used by i128 shifts
- Existing backend tests that cover i128 shift behavior

## Current Targets

- i128 constant shift counts, especially counts greater than or equal to 64
- i128 variable-count shifts
- AArch64 lowering, machine effects, register-pair spelling, and printer opcode
  spelling needed for those categories
- Focused backend tests that prove the supported behavior or explicit
  unsupported diagnostics

## Non-Goals

- Do not move AArch64 shift mechanics into shared BIR or prealloc code.
- Do not reopen i128 helper ABI/resource, preservation, selected-call, carrier,
  or scalar-result owner contracts.
- Do not modify f128 transport, helper, carrier, or printing behavior.
- Do not weaken supported-path expectations, mark supported cases unsupported,
  or reduce the test contract without explicit user approval.
- Do not perform broad `i128_ops.cpp` cleanup outside the targeted shift work.

## Working Model

- i128 shift construction consumes prepared i128 pair operands and prepared
  shift-count storage facts.
- AArch64 owns the target shift kind, lane mechanics, machine effects,
  register-pair spelling, and printer opcode spelling for this route.
- Tests should exercise shift categories, not one named testcase shortcut.
- If a category is intentionally unsupported, the diagnostic must be explicit,
  narrow, and covered.

## Execution Rules

- Make one semantic category change per implementation packet where practical.
- Prefer lowering and printing repair over diagnostics when the existing support
  contract says the case should be supported.
- If an unsupported diagnostic is chosen, document why it matches the current
  capability contract in `todo.md`.
- Keep proof commands and results in `todo.md`; use `test_after.log` for
  executor proof unless the supervisor delegates another artifact.
- Escalate to reviewer or plan-owner if the route starts depending on
  expectation rewrites, testcase-shaped matching, or shared-policy changes.

## Step 1: Map Current Shift Contract And Coverage

Goal: identify the existing behavior, coverage, and support contract for large
constant and variable i128 shifts before editing implementation code.

Primary targets:

- `src/backend/mir/aarch64/codegen/i128_ops.cpp`
- AArch64 instruction definitions and printer code reachable from i128 shifts
- Existing backend tests for i128 shifts

Actions:

- Inspect current lowering for left shift, logical right shift, and arithmetic
  right shift over i128 values.
- Identify how constant shift counts below 64, equal to 64, above 64, and
  variable counts are represented.
- Locate existing tests and classify gaps by shift category.
- Record the proposed narrow proof subset in `todo.md` for supervisor review.

Completion check:

- `todo.md` names the covered and uncovered i128 shift categories, the relevant
  files, and the first implementation category to fix or explicitly contract.

## Step 2: Complete Or Contract Shift-By-64-Or-More

Goal: make constant i128 shifts with counts greater than or equal to 64 match
the current support contract.

Primary target:

- `src/backend/mir/aarch64/codegen/i128_ops.cpp`

Actions:

- Repair lowering and printing for supported large constant shift counts.
- Add or enable focused backend tests for the affected constant-count
  categories.
- If a large-count category is intentionally unsupported, add a narrow
  diagnostic and focused coverage for that diagnostic.
- Avoid expectation downgrades or named-testcase shortcuts.

Completion check:

- Focused backend proof covers representative `== 64` and `> 64` i128 shift
  cases, or an explicit unsupported diagnostic is covered for the intentionally
  unsupported category.

## Step 3: Complete Or Contract Variable-Count Shifts

Goal: make variable-count i128 shifts match the current support contract.

Primary target:

- `src/backend/mir/aarch64/codegen/i128_ops.cpp`

Actions:

- Inspect how prepared shift-count storage facts arrive at AArch64 i128 shift
  lowering.
- Repair variable-count lowering and printing when the current support contract
  requires support.
- Add or enable focused backend tests that exercise variable counts by category.
- If variable-count i128 shifts are intentionally unsupported, add a narrow
  diagnostic and diagnostic coverage.

Completion check:

- Variable-count i128 shift behavior is proved with focused backend coverage or
  rejected through a narrow supported-contract diagnostic.

## Step 4: Consolidate Proof And Guard Against Drift

Goal: prove the completed route without expanding scope or accepting overfit.

Actions:

- Run the supervisor-selected narrow backend subset for the final packet.
- Escalate to a broader backend check if multiple shift categories changed or
  if instruction/printer changes affect shared AArch64 paths.
- Confirm no f128, helper ABI/resource, carrier ownership, or shared
  BIR/prealloc policy files were changed for this idea.
- Record final proof commands, logs, and any intentionally unsupported cases in
  `todo.md`.

Completion check:

- `todo.md` records green proof for the selected subset, canonical regression
  logs remain comparable, and the route satisfies the source idea acceptance
  criteria without reviewer reject signals.
