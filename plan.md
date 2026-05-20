# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Reactivate the umbrella backend-regex inventory after focused AArch64 owners
334 and 335 closed, so the next current semantic backend owner is selected
from fresh evidence before any implementation work starts.

## Goal

Capture and classify the current main-build backend CTest surface, then split
or select exactly one focused repair idea only when a tractable semantic owner
is supported by current logs, generated artifacts, or diagnostics.

## Core Rule

Do not implement fixes under this umbrella. Use it to inventory, classify, and
split focused owners. Do not reopen parked or closed owners from counts alone;
require current generated-code, diagnostic, or proof evidence that contradicts
their boundary.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- current `todo.md`
- current canonical `test_before.log` / `test_after.log`, if the supervisor
  selects them as the backend-regex baseline artifacts
- current `ideas/open/` statuses, especially parked or close-deferred ideas
  316, 326, 327, 328, 329, 331, and 332
- recently closed focused owners 333, 334, and 335 only when comparing a
  current failure against an already completed route

## Current Targets

- Main build backend surface selected by the supervisor, normally
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` or the
  broader `ctest -R backend` equivalent when explicitly requested.
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
- Do not reactivate parked or close-deferred ideas 316, 326, 327, 328, 329,
  331, or 332 without fresh evidence that their exact owner has returned.
- Do not continue stale `00204.c`, `00216.c`, `00164.c`, or `00214.c`
  evidence when current artifacts show a different first bad fact or no bad
  fact.

## Working Model

The latest focused route closed idea 335 after repairing the local-slot
runtime residual. Idea 295 remains the durable umbrella for re-inventorying the
backend-regex surface after such focused closures. Several open ideas are
parked or close-deferred because their representative moved outside source
scope or closure-grade logs were unavailable; they should not be reactivated
without current evidence.

## Execution Rules

- Keep packet progress and inventory tables in `todo.md`.
- Preserve canonical proof-log discipline: use `test_after.log` for a
  delegated capture only when the supervisor selects that as the proof
  artifact.
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
  parked or stale, select it for activation.
- If no existing idea matches, create a focused source idea with goal, scope,
  acceptance criteria, and reviewer reject signals.
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
