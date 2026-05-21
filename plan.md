# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Refresh and classify the main-build backend regex failure surface after the
recent focused AArch64 closures, then split or activate a semantic repair
owner before implementation work begins.

## Goal

Turn the current `ctest -R backend` residuals into focused lifecycle work,
without treating backend regex failures as one monolithic implementation
bucket.

## Core Rule

This plan is inventory and lifecycle-routing work only. Do not edit
implementation files, tests, expectations, runners, timeout policy, CTest
registration, or unsupported classifications under this umbrella.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- recent focused closure notes in `ideas/closed/`
- current canonical `test_before.log` and `test_after.log`, if present
- generated AArch64 artifacts under `build/c_testsuite_aarch64_backend/`

## Current Scope

- Main build backend regex results from `/workspaces/c4c/build`.
- Local backend/unit/CLI failures selected by the backend regex.
- External `c_testsuite_aarch64_backend_*` runtime, compile, assembler,
  linker, timeout, or output-storm failures.
- Recently parked or closure-deferred open ideas only when fresh evidence
  shows their exact owner has returned.

## Non-Goals

- Do not implement a backend repair directly under this plan.
- Do not reopen closed AArch64 owners from failing counts alone.
- Do not merge unrelated residual buckets into a broad catch-all owner.
- Do not use filename-specific, emitted-text-specific, expectation, runner, or
  timeout changes as progress.
- Do not run broad runtime scans without timeout discipline and stale-process
  cleanup.

## Working Model

The umbrella inventory owns classification and lifecycle routing. A focused
repair idea owns implementation only after a semantic owner is selected from
evidence such as diagnostics, generated assembly, prepared dumps, runtime
traces, or adjacent focused tests.

## Execution Rules

- Prefer existing canonical logs when they are fresh enough to classify the
  current surface.
- If a fresh backend regex run is needed, capture it from the main build tree:
  `ctest --test-dir build -j10 -R backend --output-on-failure`.
- Keep generated-code evidence tied to semantic owners, not test filenames.
- Record routine inventory progress in `todo.md`.
- Ask lifecycle authority to create, reactivate, close, or switch ideas once a
  focused owner decision is ready.

## Ordered Steps

### Step 1: Capture Current Backend Regex Surface

Goal: establish the current residual set after the latest focused closures.

Actions:

- Inspect existing canonical logs and their commands.
- If needed, run the main-build backend regex command with output captured to a
  canonical log chosen by the supervisor.
- Count selected, passed, failed, skipped, and timed-out tests.
- Separate local backend/unit/CLI failures from external
  `c_testsuite_aarch64_backend_*` failures.

Completion check:

- `todo.md` records the backend regex command or source log, pass/fail/timeout
  counts, and whether local backend tests are clean.

### Step 2: Classify Residual Failure Families

Goal: group failures by first bad fact and likely semantic owner.

Actions:

- Classify failures into compile/printer, semantic admission, assembler,
  linker, runtime nonzero/crash, runtime mismatch, timeout, and output-storm
  buckets.
- For the most promising bucket, inspect generated artifacts or focused dumps
  enough to identify the first bad fact.
- Compare candidate owners against recently closed or parked AArch64 ideas.
- Reject bucket groupings based only on test names or pass-count movement.

Completion check:

- `todo.md` records the classified buckets, candidate focused owner, evidence
  for the owner boundary, and rejected adjacent owners.

### Step 3: Route To Focused Lifecycle Work

Goal: leave the umbrella only after a focused semantic repair owner is ready.

Actions:

- If the candidate owner matches an existing open idea, prepare a lifecycle
  switch request for that idea.
- If the candidate owner is new, create or request a focused
  `ideas/open/*.md` contract with goal, scope, acceptance criteria, and
  reviewer reject signals.
- If no tractable owner is ready, record the blocker and the next evidence
  needed in `todo.md`.

Completion check:

- A focused idea is ready to activate, or `todo.md` explains why the residual
  set is not yet activatable.

### Step 4: Deactivate The Umbrella Before Coding

Goal: preserve durable inventory state and switch away before implementation.

Actions:

- Add a compact deactivation note to the source idea only when the focused
  owner decision is durable.
- Reset or replace `plan.md` and `todo.md` through lifecycle authority for the
  selected focused idea.
- Keep implementation files untouched in the umbrella commit.

Completion check:

- The active lifecycle state is no longer this umbrella plan before any code
  repair starts.
