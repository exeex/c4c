# AArch64 C-Testsuite Failure Family Inventory

Status: Active
Source Idea: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Follows Closed Ideas:
- ideas/closed/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md
- ideas/closed/286_aarch64_scalar_call_value_semantics.md

## Purpose

Return to the AArch64 backend c-testsuite umbrella inventory after the first
focused repair routes, classify the remaining failure families, and split the
next semantic repair idea before implementation starts.

## Goal

Produce a current, evidence-backed inventory of the AArch64 backend c-testsuite
route and create or identify the next focused repair idea from a semantic
failure family.

## Core Rule

This is an inventory and lifecycle-splitting route. Do not implement compiler
or test repairs here, and do not improve counts by changing expectations,
allowlists, unsupported classifications, timeout policy, CTest behavior,
runner behavior, testcase filenames, or exact emitted-instruction matching.

## Read First

- `ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md`
- `ideas/closed/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md`
- `ideas/closed/286_aarch64_scalar_call_value_semantics.md`
- `todo.md`
- `/tmp/c4c_aarch64_backend_scan_212.log`, if still present
- `/tmp/c4c_aarch64_backend_scan_212.classified`, if still present

## Current Targets

- The 212-case AArch64 backend c-testsuite route after runtime-hang quarantine.
- Remaining `FRONTEND_FAIL`, `RUNTIME_NONZERO`, `RUNTIME_MISMATCH`, and
  `TIMEOUT` buckets after LR preservation and scalar call-value repairs.
- Semantic family boundaries suitable for focused follow-on ideas.

## Non-Goals

- Do not implement backend, frontend, runner, or test expectation changes.
- Do not treat this umbrella idea as an implementation route.
- Do not run broad runtime scans without an explicit timeout and stale-process
  cleanup checks.
- Do not absorb unrelated compiler initiatives into this inventory.
- Do not assume the old scan counts are still current after the closed focused
  repairs.

## Working Model

The original AArch64 backend scan had 212 registered cases and classified as
46 `PASS`, 49 `FRONTEND_FAIL`, 87 `RUNTIME_NONZERO`, 7 `RUNTIME_MISMATCH`, and
23 `TIMEOUT`. The non-leaf LR route and scalar call-value route repaired known
call-boundary owners; the scalar route's final 23-case boundary proof passed
all selected cases. The next useful inventory step is to refresh the current
classification, separate frontend-owned failures from backend/runtime-owned
families, and split the next focused semantic repair idea.

## Execution Rules

- Record current inventory and family findings in `todo.md`.
- Use representative files from each bucket; do not select one testcase as the
  repair route without identifying the broader owner.
- Keep timeout/hang cases separate and use explicit timeout behavior plus
  stale generated-runtime process checks if any runtime scan is run.
- Treat frontend failures as separate from backend/runtime failures.
- Create focused ideas only for semantic owners that remain visible after the
  closed LR and scalar call-value routes.
- Switch lifecycle state to a focused idea before implementation work begins.

## Steps

### Step 1: Refresh AArch64 Inventory Evidence

Goal: Establish the current AArch64 backend c-testsuite classification after
the closed LR preservation and scalar call-value routes.

Primary target: the 212-case AArch64 backend c-testsuite route.

Actions:

- Inspect any still-current scan logs named by the source idea.
- If the logs are absent or stale, rerun the AArch64 backend c-testsuite route
  with an explicit timeout and check for stale generated-runtime processes
  afterward.
- Record the command, log paths, total case count, and bucket counts in
  `todo.md`.
- Call out any difference from the old 46/49/87/7/23 inventory.

Completion check: `todo.md` contains a current inventory with counts, proof
command or log source, and any stale-process cleanup result.

### Step 2: Classify Representative Failure Families

Goal: Turn the current buckets into semantic families with clear ownership.

Primary target: representative files from each non-pass bucket.

Actions:

- Inspect representative `FRONTEND_FAIL` cases and separate frontend-owned
  parse/sema failures from backend/runtime failures.
- Inspect representative `RUNTIME_NONZERO` and `RUNTIME_MISMATCH` cases for
  shared semantic owners.
- Keep any `TIMEOUT` cases isolated from ordinary mismatch work.
- Compare call-boundary cases against the closed scalar call-value result so
  already-repaired owners are not rediscovered as new work.

Completion check: `todo.md` names the major semantic families, representative
cases, owner boundary, and any families that are intentionally deferred.

### Step 3: Select the Next Focused Repair Idea

Goal: Choose the next tractable semantic family for implementation outside
this umbrella inventory.

Primary target: one current backend/runtime-owned family, or a frontend-owned
family if it is the best next source idea.

Actions:

- Prefer a family with multiple representative cases and a plausible shared
  compiler owner.
- Reject named-case-only fixes, expectation rewrites, allowlist edits, and
  runner changes as progress.
- Decide whether timeout/hang cases need a dedicated safe idea or should stay
  deferred.
- If no family is ready, record the exact ambiguity and missing evidence in
  `todo.md`.

Completion check: `todo.md` identifies the selected family and explains why it
is ready for a focused idea, or explains why no family is ready.

### Step 4: Split and Switch Lifecycle State

Goal: Leave this umbrella route only after durable source intent exists for
the selected focused repair.

Primary target: lifecycle artifacts, not implementation files.

Actions:

- Ask the plan owner to create a focused `ideas/open/*.md` file for the
  selected semantic family, including concrete reviewer reject signals.
- Deactivate this inventory runbook with only durable summary and leftover
  notes preserved in the source idea.
- Activate the focused idea into `plan.md` and reset `todo.md` before any
  implementation packet starts.

Completion check: lifecycle state has switched from this umbrella idea to a
focused repair idea, or `todo.md` records the blocker that prevents the split.
