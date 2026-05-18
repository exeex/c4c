# AArch64 C-Testsuite Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md

## Purpose

Use the current AArch64 backend c-testsuite failures as an umbrella inventory,
then split exactly one focused semantic repair idea before implementation work
continues.

## Goal

Refresh the post-292 AArch64 failure map, classify remaining backend/runtime
families, and select the next focused owner or document why no owner is ready.

## Core Rule

This runbook is inventory-only. Do not implement compiler/backend repairs under
this umbrella idea.

## Read First

- `ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md`
- `ideas/closed/292_aarch64_scalar_expression_control_value_authority.md`
- Latest broad scan artifacts if still present:
  - `/tmp/c4c_aarch64_scalar_step4_broad.log`
  - `/tmp/c4c_aarch64_scalar_step4_summary.txt`
  - `/tmp/c4c_aarch64_scalar_step4_new_passes.txt`

## Current Targets

- Remaining AArch64 backend c-testsuite failures after closed focused ideas
  285-292.
- Known unresolved buckets: pointer/aggregate, timeout, printer/admission,
  floating/conversion/string, and side-effect control.
- Closed owners must stay closed unless fresh generated-code evidence
  contradicts their closure.

## Non-Goals

- Do not touch implementation files, tests, CTest registration, runner
  behavior, expected outputs, allowlists, or unsupported classifications.
- Do not treat timeout/hang cases as ordinary runtime mismatches.
- Do not create a named-testcase repair idea without a broader semantic owner.
- Do not reopen closed AArch64 owners from residual failure counts alone.

## Execution Rules

- Use timeout-protected broad scans and check for stale generated-runtime
  processes afterward.
- Inspect representatives by semantic family, not by filename count alone.
- Separate frontend-owned failures from backend/runtime-owned failures.
- Write durable next-owner information to `todo.md` while investigating.
- If a coherent focused owner emerges, create a new `ideas/open/*.md` source
  idea with concrete reviewer reject signals and switch lifecycle state to it.

## Steps

### Step 1: Refresh Post-292 Inventory

Goal: establish the current AArch64 backend failure map after scalar
expression/control-value authority closed.

Primary target: timeout-protected AArch64 backend c-testsuite scan.

Actions:

- Prefer the latest post-292 broad artifacts if they are still current.
- Otherwise run:
  `ctest --test-dir build-aarch64-scan -L '^aarch64_backend$' -j 8 --timeout 5 --output-on-failure`
- Record pass/fail totals and classify failures into frontend, runtime
  nonzero, runtime mismatch, timeout, and compile/printer/admission buckets.
- Check that no generated-runtime process remains after the scan.

Completion check:

- `todo.md` records current totals, representative cases, log paths, and any
  stale-process cleanup result.

### Step 2: Select the Next Semantic Owner

Goal: choose the smallest remaining backend/runtime family that can become a
focused repair idea.

Primary target: representatives from non-frontend, non-timeout buckets unless
fresh evidence shows a timeout-specific owner is now safest.

Actions:

- Inspect generated output and source shape for representative failures.
- Compare candidates against closed owners 285-292 to avoid reopening closed
  work without contradictory evidence.
- Prefer a semantic family that can be proven with a small starter subset and
  broader nearby sampling.
- Defer buckets that are primarily frontend, environment-sensitive timeout,
  or compile-stage printer/admission work unless they are clearly the next
  safest owner.

Completion check:

- `todo.md` names the selected owner, starter representatives, rejected or
  deferred buckets, and the proof shape expected for the focused idea.

### Step 3: Split or Close Inventory

Goal: leave lifecycle state consistent after the inventory decision.

Primary target: `ideas/open/` plus active lifecycle files.

Actions:

- If a focused owner is ready, create a new `ideas/open/*.md` file with intent,
  scope, acceptance criteria, and concrete reviewer reject signals.
- Switch active lifecycle state from this umbrella inventory to the focused
  idea using a new `plan.md` and reset `todo.md`.
- If no owner is ready, keep this umbrella active only with a narrow next
  classification step or record the exact ambiguity that blocks selection.

Completion check:

- There is at most one active plan.
- `plan.md` and `todo.md`, if present, point to the same `ideas/open/*.md`.
- This umbrella is not used for implementation work.
