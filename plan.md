# Backend Regex Timeout Residual Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Refresh the backend-regex inventory after idea 380 and decide whether the
remaining timeout-only residuals are ready for a focused semantic repair idea.

## Goal

Classify the current `00200` and `00207` timeout-only residuals without turning
timeout policy, runner behavior, or filename-specific probes into claimed
backend progress.

## Core Rule

This umbrella plan is classification and lifecycle routing only. Do not edit
implementation, tests, CTest registration, expectations, unsupported lists,
timeout policy, runners, or proof-log policy under this plan.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- Existing `test_before.log` / `test_after.log` only if the supervisor says
  they are the current baseline artifacts.
- Generated artifacts for the residual representatives under
  `build/c_testsuite_aarch64_backend/` when present.
- Source representatives:
  - `tests/c/external/c-testsuite/src/00200.c`
  - `tests/c/external/c-testsuite/src/00207.c`

## Current Targets

- `c_testsuite_aarch64_backend_src_00200_c`
- `c_testsuite_aarch64_backend_src_00207_c`

The supervisor reports that the recent full-suite baseline removed `00216`.
Treat `00216` as an adjacency check only unless fresh evidence reintroduces it.

## Non-Goals

- Do not implement a timeout fix while this umbrella is active.
- Do not broaden this pass back into closed non-timeout owners unless current
  generated-code or proof evidence contradicts a closure boundary.
- Do not classify a timeout by increasing limits, weakening contracts, killing
  only the symptom, or changing runner behavior.
- Do not split an idea that is only "make `00200` pass" or "make `00207`
  pass" without a semantic owner.

## Working Model

- `00200` is currently treated as a shift/type-promotion timeout bucket.
- `00207` is currently treated as a dynamic stack/VLA fixed-slot timeout
  bucket.
- Either bucket may need a separate focused owner, or both may share one owner
  only if generated-code evidence shows the same semantic failure mechanism.

## Execution Rules

- Use bounded commands for timeout investigation and clean up stale child
  processes before trusting runtime logs.
- Prefer existing fresh logs when they match the supervisor's baseline command.
- Classify the first bad fact from source, semantic/prepared dumps, generated
  AArch64, and bounded runtime behavior.
- If a focused owner is found, create or update the appropriate
  `ideas/open/*.md` with concrete reviewer reject signals, then request a
  lifecycle switch before implementation.

## Steps

### Step 1: Confirm the Current Residual Surface

Goal: verify the post-380 baseline shape before choosing a timeout route.

Actions:
- Inspect current supervisor-provided proof logs, if present, for the exact
  selected count, pass count, and failing tests.
- Confirm `00216` is no longer a current backend-regex residual.
- Confirm whether the only remaining backend-regex failures are
  `c_testsuite_aarch64_backend_src_00200_c` and
  `c_testsuite_aarch64_backend_src_00207_c`.
- Record any mismatch in `todo.md` rather than mutating the source idea.

Completion Check:
- `todo.md` records the current residual list and whether `00200`/`00207` are
  still timeout-only.

### Step 2: Classify `00200` Timeout Mechanism

Goal: decide whether `00200` has a semantic timeout owner.

Actions:
- Inspect the source and generated artifacts for shift, promotion, loop, and
  runtime-output behavior that could explain the timeout.
- Compare semantic/prepared state to generated AArch64 around the first
  suspicious recurrence or unbounded loop.
- Determine whether the owner is shift/type-promotion lowering, scalar value
  publication, loop update state, or another concrete backend boundary.

Completion Check:
- `todo.md` names the first bad fact for `00200`, the suspected owner, and the
  evidence path, or explains why it remains unclassified.

### Step 3: Classify `00207` Timeout Mechanism

Goal: decide whether `00207` has a semantic timeout owner.

Actions:
- Inspect the source and generated artifacts for dynamic stack, VLA, fixed
  stack slot, loop, and runtime-output behavior that could explain the timeout.
- Compare semantic/prepared state to generated AArch64 around the first
  suspicious stack or VLA address calculation.
- Determine whether the owner is dynamic stack/VLA slot assignment, address
  scaling, stack-frame publication, loop update state, or another concrete
  backend boundary.

Completion Check:
- `todo.md` names the first bad fact for `00207`, the suspected owner, and the
  evidence path, or explains why it remains unclassified.

### Step 4: Route to Focused Lifecycle Work

Goal: leave the umbrella before implementation begins.

Actions:
- If one or both timeout buckets have a semantic owner, create or select a
  focused `ideas/open/*.md` for that owner.
- Include reviewer reject signals for testcase-specific shortcuts, timeout
  policy changes, runner changes, expectation weakening, and count-only claims.
- Preserve a concise durable deactivation note in
  `ideas/open/295_backend_regex_failure_family_inventory.md` only if the
  supervisor asks for a lifecycle switch.
- Do not start code edits under this umbrella plan.

Completion Check:
- A focused idea is ready for activation, or `todo.md` explains why no focused
  owner can be split yet.
