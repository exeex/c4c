# Backend Regex Post-Timeout Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Refresh the backend-regex inventory after the timeout-focused owners 381 and
382 have been closed, then route any remaining backend residual to a focused
semantic idea before implementation begins.

## Goal

Classify the current backend-regex residual surface from fresh or
supervisor-approved proof artifacts, identify the next tractable semantic
owner, and leave the umbrella plan before code edits start.

## Core Rule

This umbrella plan is classification and lifecycle routing only. Do not edit
implementation, tests, expectations, unsupported lists, timeout policy,
runners, CTest registration, or proof-log policy under this plan.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- Current `test_before.log` / `test_after.log` only if the supervisor says
  they are the active baseline artifacts.
- Generated artifacts under `build/c_testsuite_aarch64_backend/` for any
  residual representatives confirmed by Step 1.
- Source files under `tests/c/external/c-testsuite/src/` for any residual
  representatives confirmed by Step 1.

## Current Targets

- Confirm the post-382 backend-regex selected count, pass count, and failing
  tests.
- Separate local backend/unit/CLI failures from external
  `c_testsuite_aarch64_backend_*` failures if any local failures reappear.
- Classify each remaining external residual by first bad fact and semantic
  owner candidate.

## Non-Goals

- Do not implement a backend fix while this umbrella is active.
- Do not reopen closed focused owners 381 or 382 unless fresh generated-code
  or proof evidence contradicts their closure boundary.
- Do not broaden into stale parked ideas only because their files remain under
  `ideas/open/`.
- Do not claim progress through pass-count movement, expectation changes,
  runner behavior, timeout policy, unsupported classifications, or CTest
  registration changes.
- Do not split an idea that is only "make one named c-testsuite case pass"
  without a semantic owner.

## Working Model

- Idea 381 closed the `00200` shift/type-promotion backend codegen
  scalability timeout owner.
- Idea 382 closed the `00207` dynamic-stack/VLA fixed-slot addressing timeout
  owner.
- The remaining backend-regex surface must be re-inventoried because older
  parked notes may describe residuals that have since been closed, split, or
  displaced.
- If the remaining failures share no semantic owner, choose the highest-value
  focused owner and leave the rest parked under this umbrella.

## Execution Rules

- Prefer a supervisor-approved baseline command or fresh backend-regex proof
  over stale logs.
- Record routine classification progress in `todo.md`; do not mutate the
  source idea unless a lifecycle switch needs a durable deactivation note.
- Use bounded commands for any timeout or runtime investigation, and clean up
  stale child processes before trusting logs.
- For every candidate owner, compare source, semantic/prepared artifacts, and
  generated AArch64 evidence before splitting implementation work.
- If a focused owner is found, create or select a focused `ideas/open/*.md`
  with concrete reviewer reject signals, then request a lifecycle switch before
  implementation.

## Steps

### Step 1: Confirm the Current Residual Surface

Goal: establish the post-382 backend-regex baseline before choosing the next
route.

Actions:
- Inspect current supervisor-provided proof logs, if present, for selected
  count, pass count, failing tests, and command scope.
- If logs are stale or missing, ask the supervisor to provide or approve the
  backend-regex proof command before broad runtime execution.
- Classify failures at the top level as local backend/unit/CLI, external
  AArch64 c-testsuite, admission, machine-printer, runtime mismatch/nonzero,
  or timeout.
- Record the exact residual list and evidence freshness in `todo.md`.

Completion Check:
- `todo.md` names the current residual tests, their coarse buckets, and whether
  the evidence is fresh enough for focused classification.

### Step 2: Localize Candidate First Bad Facts

Goal: turn each current residual from a test name into a concrete first bad
fact.

Actions:
- For each current residual, inspect the source representative and generated
  artifacts from the confirmed build.
- Identify the first failing observable: compile diagnostic, assembler error,
  runtime nonzero, runtime mismatch, timeout mechanism, or crash site.
- Compare against the closure boundaries of recent focused owners before
  reopening any owner.
- Record candidate semantic owners and evidence paths in `todo.md`.

Completion Check:
- Each current residual has a first-bad-fact note, or `todo.md` states the
  exact ambiguity blocking classification.

### Step 3: Select Or Split The Next Focused Owner

Goal: pick the next implementation target without overfitting a testcase.

Actions:
- Group residuals only when generated-code evidence points to the same
  semantic backend capability.
- Prefer a focused owner that can be proved with backend coverage independent
  of the external representative.
- If no existing open idea exactly owns the candidate, prepare a new
  `ideas/open/*.md` idea with goal, scope, acceptance criteria, and reviewer
  reject signals.
- Keep parked or already-satisfied open ideas out of the route unless fresh
  evidence makes them active again.

Completion Check:
- A focused idea is ready for lifecycle switch, or `todo.md` explains why no
  focused owner is currently activatable.

### Step 4: Lifecycle Handoff

Goal: leave the umbrella before implementation begins.

Actions:
- If a focused owner was selected or created, preserve a concise durable
  deactivation note in idea 295 only as part of the lifecycle switch.
- Reset `plan.md` and `todo.md` to the focused owner during the switch.
- Do not start implementation under this umbrella runbook.

Completion Check:
- Active lifecycle state points at exactly one focused implementation idea, or
  the umbrella remains active with a documented blocker in `todo.md`.
