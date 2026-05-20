# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md
Activated After: ideas/closed/345_aarch64_scalar_select_result_publication.md

## Purpose

Reactivate the backend regex umbrella after the focused scalar-select owner
closed, classify the current residual backend failures, and split the next
focused semantic repair idea before implementation work begins.

## Goal

Produce a current, classified backend-regex residual inventory and either
create/switch to one focused repair owner or record why no owner is ready.

## Core Rule

This is an inventory and lifecycle-splitting runbook. Do not implement backend
repairs under this umbrella, and do not claim progress through expectation,
unsupported, timeout, runner, proof-log, or CTest-registration changes.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `ideas/closed/345_aarch64_scalar_select_result_publication.md`
- Current canonical proof logs, especially `test_before.log` and
  `test_after.log`
- Main build backend test registration under `/workspaces/c4c/build`

## Current Targets

- Primary inventory command:
  - `ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Historical umbrella command, if a broader regex comparison is needed:
  - `cd /workspaces/c4c/build && ctest -j10 -R backend --output-on-failure`
- Current known checkpoint:
  - idea 345 close gate improved the canonical backend-focused scope from
    `141/142` to `142/142`
  - the scalar-select `00143` residual is closed and should not be reopened
    without fresh generated-code evidence

## Non-Goals

- Do not edit implementation files or tests while this umbrella is active.
- Do not reopen closed focused owners 285 through 345 from counts alone.
- Do not fold parked `00204.c` variadic/HFA/byval ideas into the backend
  regex inventory unless fresh evidence shows they are the current backend
  residual owner being classified.
- Do not treat `ctest -R backend` as one monolithic failure bucket.
- Do not run broad runtime scans without timeout awareness and stale-process
  cleanup.
- Do not change expectations, allowlists, unsupported classifications, runner
  behavior, timeout policy, proof-log policy, or CTest registration.

## Working Model

The umbrella exists to keep broad backend-regex failures classified before
implementation starts. After a focused owner closes, the correct next move is
to refresh or reconstruct the residual inventory, separate local backend tests
from external AArch64 c-testsuite runtime failures, compare failures against
closed owner boundaries, and split a focused semantic owner only when evidence
identifies a tractable capability gap.

## Execution Rules

- Keep transient inventory notes, counts, and command results in `todo.md`.
- Prefer existing canonical logs when they are fresh enough for classification;
  otherwise run the supervisor-delegated backend-regex inventory command.
- Classify failures by stage and owner evidence, not by filename alone.
- A focused split must name concrete generated-code, diagnostic, runtime, or
  prepared-state evidence tying the cases together.
- Once a focused owner is created, lifecycle should switch away from this
  umbrella before any implementation edits begin.

## Steps

### Step 1: Capture Or Reconstruct Current Backend Inventory

Goal: establish the current backend-regex residual set after idea 345 closed.

Primary target: current canonical backend proof logs and the main build backend
regex scope.

Actions:

- Inspect `test_before.log` and `test_after.log` to determine whether they
  provide a fresh enough post-345 backend-regex inventory.
- If existing logs are insufficient for classification, run the
  supervisor-approved backend-regex inventory command from the main build tree.
- Record selected, passed, failed, timed-out, and skipped counts in `todo.md`.
- Separate local backend/unit/CLI tests from
  `c_testsuite_aarch64_backend_*` failures.

Completion check:

- `todo.md` records the current backend-regex inventory basis and the exact
  residual failure list or states why a fresh run is still needed.

### Step 2: Classify Residual Failures Against Closed Boundaries

Goal: group the residual failures by semantic owner without reopening closed
owners from counts alone.

Primary target: the residual list from Step 1.

Actions:

- Bucket residuals by compile-stage diagnostics, assembler/linker failures,
  runtime nonzero/crash, runtime mismatch, timeout/output-storm, and local
  backend/unit failures.
- Compare each promising bucket against closed focused owners through 345.
- For any bucket that appears to contradict a closed boundary, require fresh
  generated-code, prepared-state, diagnostic, or runtime evidence before
  proposing a reopened/split owner.
- Record buckets that are too broad, stale, or environment-sensitive in
  `todo.md` rather than forcing an implementation route.

Completion check:

- `todo.md` identifies the best next semantic owner candidate, or explicitly
  explains why no owner is ready to split.

### Step 3: Split Or Park The Next Lifecycle State

Goal: move from umbrella classification to the next lifecycle state.

Primary target: one focused owner candidate from Step 2, if evidence supports
one.

Actions:

- If a tractable semantic owner is found, create a focused `ideas/open/*.md`
  source idea with concrete scope, acceptance criteria, and reviewer reject
  signals.
- Add a durable deactivation note to this umbrella only if needed to preserve
  the inventory decision and remaining buckets.
- Switch lifecycle state to the focused idea by replacing this runbook and
  resetting `todo.md` through plan-owner authority.
- If no owner is activatable, leave `todo.md` with the classified blockers and
  report that no focused split is ready.

Completion check:

- Either a focused source idea is ready for activation and implementation
  routing, or the umbrella remains parked with a concrete inventory reason no
  implementation packet should start.
