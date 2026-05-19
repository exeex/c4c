# Backend Regex Failure Family Inventory

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Classify the current main-build `ctest -R backend` failure set and split only
semantic repair owners from the umbrella inventory before any implementation
work starts.

## Goal

Produce a current backend-regex failure inventory that separates local backend
failures from AArch64 c-testsuite backend failures, preserves closed-owner
boundaries, and creates the next focused `ideas/open/*.md` owner only when the
failure evidence supports a semantic repair family.

## Core Rule

This is an inventory and lifecycle-splitting runbook. Do not implement fixes
under this umbrella idea, and do not claim progress through expectation
rewrites, unsupported downgrades, allowlist changes, timeout policy changes,
runner behavior changes, CTest registration changes, filename matching, or
instruction-string matching.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `test_before.log`
- Current main-build backend regex evidence from `/workspaces/c4c/build`
- Recently closed focused owners 285 through 297 when a bucket appears to
  overlap a closure boundary

## Current Targets

- Capture or refresh the main-build backend regex result:
  `ctest -j10 -R backend --output-on-failure` from `/workspaces/c4c/build`.
- Classify failures into:
  - local backend/unit/CLI tests;
  - AArch64 c-testsuite backend runtime tests;
  - frontend or prepared-module handoff failures reached through backend tests;
  - timeout/hang or runtime-output-storm cases.
- Compare current failures against the closed-owner boundaries for ideas 285
  through 297 before reopening or splitting any work.
- Preserve known residuals from the idea-297 closure as classification inputs,
  not as automatic new owners:
  - residual global, pointer, and aggregate projection GEP work;
  - `00216` pointer-parameter/flexible-array aggregate projection;
  - AArch64 printer residuals.

## Non-Goals

- Do not implement backend, frontend, lowering, printer, runtime, runner, or
  test-expectation changes.
- Do not treat `ctest -R backend` as one monolithic repair bucket.
- Do not reopen closed AArch64 owners 285 through 297 from failing counts alone.
- Do not create a focused idea for a named testcase without a semantic owner.
- Do not run broad runtime scans without timeout discipline and stale-process
  cleanup.
- Do not edit `test_before.log`, `test_after.log`, `test_baseline.log`, test
  expectations, allowlists, unsupported classifications, timeout policy, runner
  behavior, or CTest registration as part of this inventory.

## Working Model

Idea 295 is an umbrella inventory. Previous inventory passes split and closed
focused owners for fused compare-branch operand forms and local-memory
`lir_to_bir` admission. The current pass starts after idea 297 closure, so it
must refresh or read current backend-regex evidence, preserve the accepted
focused baseline in `test_before.log`, and decide whether remaining failures
justify one or more new focused source ideas.

## Execution Rules

- Keep routine packet findings in `todo.md`.
- Create or update `ideas/open/*.md` only when a durable focused owner is ready
  to split from the inventory.
- If a bucket is only a residual note or needs more evidence, record that in
  `todo.md` instead of creating a premature source idea.
- When a focused owner is split, the supervisor should switch lifecycle state
  away from this umbrella idea before implementation begins.
- If a timeout or hang appears, quarantine or split a hang-specific idea before
  asking an executor to run broad runtime proof.
- Treat testcase-overfit and expectation-only count improvements as blocking
  route failures.

## Steps

### Step 1: Capture Current Backend Regex Evidence

Inspect the available canonical baseline and, if the supervisor delegates a
fresh capture, run the main-build backend regex command from `/workspaces/c4c/build`:

```bash
ctest -j10 -R backend --output-on-failure
```

Record the selected, passed, and failed counts, and preserve the exact failing
test list in `todo.md`.

Completion check: `todo.md` records the current backend-regex command source,
counts, failing tests, and whether the data came from `test_before.log` or a
fresh delegated run.

### Step 2: Classify Failure Sources

Group each failure by failure source: local backend/unit/CLI, AArch64
c-testsuite backend runtime, frontend or prepared-module handoff,
timeout/hang/runtime-output-storm, machine-printer, `lir_to_bir` admission, or
another observed semantic source.

Completion check: `todo.md` contains a classified inventory with enough
evidence for a reviewer to reject filename-only grouping.

### Step 3: Compare Closed Owner Boundaries

Compare each meaningful bucket against closed ideas 285 through 297. Reopen or
split follow-up work only when generated-code, proof, or diagnostic evidence
contradicts a closure boundary or shows a distinct residual owner.

Completion check: `todo.md` records which closed owners remain valid, which
residuals are adjacent-but-new work, and which buckets lack enough evidence to
split.

### Step 4: Split Focused Repair Owners

For any tractable semantic bucket, create a focused `ideas/open/*.md` file with
goal, scope, non-goals, acceptance criteria, and reviewer reject signals. Keep
separate semantic families separate, especially global/pointer/aggregate GEP
projection, `00216` pointer-parameter/flexible-array aggregate projection,
printer residuals, runtime mismatches, and timeout/hang cases unless evidence
proves a shared owner.

Completion check: at least one focused owner exists for implementation, or
`todo.md` explains why no owner is ready to split.

### Step 5: Prepare Lifecycle Switch

Once a focused owner is ready, preserve a durable deactivation note in the
source idea only if the lifecycle switch needs it, then let the supervisor
activate the focused owner before any implementation edits happen.

Completion check: this umbrella runbook has either produced a focused source
idea for activation or recorded why the inventory cannot yet split. No
implementation files or test contracts were touched.
