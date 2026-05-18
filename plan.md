# AArch64 C-Testsuite Failure Family Inventory

Status: Active
Source Idea: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md

## Purpose

Refresh the AArch64 backend c-testsuite failure inventory after the completed
focused repairs, classify the remaining failures by semantic owner, and split
the next tractable repair into its own focused idea before implementation.

## Goal

Create or update a current inventory, choose the next semantic failure family,
and hand execution off to a focused repair idea instead of coding under this
umbrella plan.

## Core Rule

This is inventory and lifecycle-routing work only. Do not implement backend,
frontend, runner, expectation, allowlist, unsupported-classification, timeout,
or CTest behavior changes under this plan.

## Read First

- `ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md`
- `todo.md`
- Prior focused closures under `ideas/closed/285_*`,
  `ideas/closed/286_*`, and `ideas/closed/287_*` when historical context is
  needed.

## Current Targets

- Refresh or verify the current AArch64 backend c-testsuite scan after the
  completed LR preservation, scalar call-value, and string/global external-call
  repairs.
- Classify remaining failures into semantic families without treating one
  c-testsuite filename as the owner.
- Keep timeout/hang cases separate from ordinary runtime mismatch work unless
  a timeout-specific route is intentionally created.
- Create the next focused `ideas/open/*.md` repair idea when a tractable owner
  is identified.

## Non-Goals

- Do not implement compiler/backend repairs in this umbrella plan.
- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Do not run broad runtime scans without an explicit timeout and stale process
  cleanup.
- Do not fold frontend-owned failures into backend runtime repair work.
- Do not use `src/00132.c` or any timeout-sensitive case as ordinary mismatch
  proof without a timeout-specific plan.

## Working Model

The original 212-case AArch64 backend route has already produced focused
repair ideas for non-leaf LR preservation, scalar call-value semantics, and
string/global address external-call lowering. With those completed, the next
inventory pass should re-scan or verify current evidence, classify the
remaining failures, and split one focused semantic owner into its own idea
before implementation resumes.

## Execution Rules

- Keep inventory commands explicit about timeout behavior.
- Check for stale generated-runtime processes after broad runtime scans.
- Store transient classification details in `todo.md`.
- Preserve durable source intent; edit the source idea only for true
  lifecycle notes or durable inventory findings.
- If a focused semantic owner is identified, create a separate source idea
  under `ideas/open/` with concrete reviewer reject signals and request a
  lifecycle switch.
- Reject pass-count claims based on expectation rewrites, allowlist changes,
  unsupported reclassification, runner changes, timeout changes, or CTest
  contract changes.

## Steps

### Step 1: Refresh Current AArch64 Backend Inventory

Goal: Establish current post-repair evidence for the AArch64 backend
c-testsuite route.

Primary target: `build-aarch64-scan` c-testsuite AArch64 backend tests.

Actions:

- Verify the AArch64 scan build is current enough for inventory work, or ask
  the supervisor for the exact refresh command.
- Run or consume a scan using an explicit timeout, such as:
  `ctest --test-dir build-aarch64-scan -L '^aarch64_backend$' -j 8 --timeout 5 --output-on-failure`.
- Check for stale generated-runtime processes after the scan.
- Record the current pass/fail/timeout bucket counts and log locations in
  `todo.md`.

Completion check: `todo.md` contains a current inventory snapshot with command,
log location, bucket counts, and stale-process status.

### Step 2: Classify Remaining Failure Families

Goal: Group remaining failures by semantic owner without overfitting to named
testcases.

Primary target: remaining non-passing AArch64 backend c-testsuite cases.

Actions:

- Separate frontend failures from backend/runtime failures.
- Keep timeout/hang cases in a distinct bucket with timeout-specific handling.
- Inspect representative source files and generated behavior for each
  backend/runtime bucket.
- Record candidate owners, representative cases, and excluded adjacent cases
  in `todo.md`.

Completion check: `todo.md` names at least one tractable semantic owner with
representative cases, or explains why no family is ready to split.

### Step 3: Split the Next Focused Repair Idea

Goal: Convert the best semantic family into a focused source idea before
implementation starts.

Primary target: one tractable backend/runtime owner from Step 2.

Actions:

- Write a new `ideas/open/*.md` focused repair idea only after the semantic
  owner, scope, non-goals, proof cases, and reject signals are clear.
- Ensure the new idea rejects testcase-shaped shortcuts, expectation/allowlist
  changes, and broad rewrites outside the selected owner.
- Preserve timeout-sensitive or frontend-owned adjacent failures as separate
  notes instead of absorbing them into the focused idea.
- Ask for lifecycle switch/activation to the focused idea before any
  implementation work begins.

Completion check: a focused repair idea exists under `ideas/open/`, or
`todo.md` explains why the umbrella inventory could not safely split one.

### Step 4: Deactivate or Switch Lifecycle State

Goal: Leave the umbrella inventory only after durable routing state is clear.

Primary target: lifecycle artifacts.

Actions:

- If Step 3 created a focused idea, request plan-owner lifecycle switch from
  this umbrella plan to that focused idea.
- If no focused route is ready, keep the umbrella active with precise next
  inventory work in `todo.md`.
- Update the source idea only if durable findings or lifecycle notes must be
  preserved beyond `todo.md`.

Completion check: active lifecycle state either switches to a focused repair
idea, or this umbrella plan remains active with a precise next inventory step.
