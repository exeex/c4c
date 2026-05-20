# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Reactivate the umbrella backend-regex inventory after the current focused
routes have either closed, parked, or become stale. The goal is to identify the
next current semantic backend owner from fresh evidence before any
implementation work starts.

## Goal

Capture and classify the current main-build `backend` CTest surface, then
split or activate a focused repair idea only when a tractable semantic owner is
supported by current artifacts.

## Core Rule

Do not implement fixes under this umbrella. Use it to inventory, classify, and
split focused owners. Do not reopen parked or closed owners from counts alone;
require current generated-code, diagnostic, or proof evidence that contradicts
their boundary.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- current `todo.md`
- current canonical `test_before.log` / `test_after.log`, if the supervisor
  says they are the relevant backend-regex baseline
- current `ideas/open/` statuses, especially parked ideas 316, 328, 329, and
  332
- closed-owner boundaries only when comparing a current failure against an
  already completed route

## Current Targets

- Main build backend-regex surface selected by `ctest -R backend` or the
  supervisor-selected equivalent.
- Local backend/unit/CLI tests versus external
  `c_testsuite_aarch64_backend_*` tests.
- Current runtime failures, runtime mismatches, compile/printer diagnostics,
  semantic handoff failures, timeouts, and output-storm cases.
- Open inactive idea records that may be closable, parked, stale, or
  activatable only with fresh evidence.

## Non-Goals

- Do not edit implementation files or tests.
- Do not treat the whole backend regex as one repair bucket.
- Do not change expectations, unsupported classifications, allowlists,
  runners, timeout policy, proof-log policy, or CTest registration.
- Do not reactivate parked ideas 316, 328, 329, or 332 without fresh evidence
  that their exact owner has returned.
- Do not continue stale `00204.c` or `00216.c` evidence when current artifacts
  show a different first bad fact or no bad fact.

## Working Model

The previous focused routes have advanced many AArch64 residuals, and some
open idea files remain parked because strict close-gate logs were unavailable
or the representative moved outside their source scope. The next useful move is
a fresh inventory pass, not speculative repair from stale notes.

## Execution Rules

- Keep packet progress and inventory tables in `todo.md`.
- Preserve canonical proof-log discipline: use `test_after.log` for a delegated
  capture only when the supervisor selects that as the proof artifact.
- Classify before splitting. Split a focused owner only when multiple facts or
  one crisp singleton point to a semantic backend capability.
- Quarantine timeouts or output-storm cases separately from normal runtime
  mismatches.
- When a focused owner is created or chosen, return to plan-owner for the
  lifecycle switch before implementation.

## Ordered Steps

### Step 1: Capture Fresh Backend Regex Inventory

Goal: obtain a current backend-regex result from the main build tree.

Primary target: supervisor-selected backend-regex CTest command and canonical
log path.

Actions:

- Capture the selected backend-regex output without editing implementation.
- Record total selected, passed, failed, skipped, timeout, and incomplete
  counts.
- List failing tests by exact CTest name and failure mode.
- Separate local backend/unit/CLI failures from
  `c_testsuite_aarch64_backend_*` failures.
- Identify whether any failures are stale relative to current generated
  artifacts.

Completion check:

- `todo.md` contains a current backend-regex inventory with counts, failing
  tests, and first-pass buckets.

### Step 2: Classify Failure Families

Goal: group current failures by semantic owner candidate rather than by test
number.

Primary targets: current logs, generated artifacts, prepared dumps, and
focused command output for representative failures.

Actions:

- Classify failures into compile/printer, semantic handoff, assembler/linker,
  runtime nonzero/crash, runtime mismatch, timeout, and output-storm buckets.
- Compare current symptoms against parked open ideas and relevant closed-owner
  boundaries.
- Reject any classification that depends only on a filename, literal offset,
  expected-output line, or stale lifecycle note.
- Record representative evidence for each tractable family.

Completion check:

- `todo.md` names the best next semantic owner candidate, or explains why no
  focused owner is ready to split.

### Step 3: Split Or Select One Focused Owner

Goal: produce exactly one implementation-ready lifecycle target when evidence
supports it.

Primary target: one focused `ideas/open/*.md` source idea, either existing or
new.

Actions:

- If an existing open idea directly matches the current owner and is not
  parked/stale, select it for activation.
- If no existing idea matches, create a minimal focused source idea with goal,
  scope, acceptance criteria, and reviewer reject signals.
- If the best candidate is parked, document the fresh evidence required to
  reactivate it before switching.
- Keep unrelated residual buckets parked under idea 295.

Completion check:

- A single focused source idea is ready for activation, or `todo.md` records
  `WAIT_FOR_NEW_IDEA` / no activatable focused owner with the exact ambiguity.

### Step 4: Deactivate Umbrella And Hand Off

Goal: leave lifecycle state pointing at the focused owner before code changes.

Primary target: `plan.md`, `todo.md`, and the selected focused source idea.

Actions:

- Preserve the inventory summary and remaining buckets in idea 295.
- Switch active lifecycle state to the focused source idea.
- Ensure the new `plan.md` and `todo.md` point to the same focused source.
- Do not perform implementation edits as part of the switch.

Completion check:

- The umbrella inventory is inactive, exactly one focused plan is active, and
  the next executor packet has a concrete localization or repair target.
