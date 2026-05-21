# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md
Reactivated After: ideas/closed/370_aarch64_short_circuit_control_publication_regression.md

## Purpose

Refresh the backend-regex residual inventory after the recent AArch64 focused
closures and select the next focused owner before any implementation work.

## Goal

Classify the current backend-matching failure surface, preserve the accepted
baseline context, and split or select one semantic repair owner.

## Core Rule

This is an umbrella classification plan. Do not implement fixes here, do not
change expectations or runners, and do not treat counts alone as owner
evidence.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `todo.md`
- `test_baseline.log`
- `test_after.log`
- recent closed ideas:
  - `ideas/closed/366_aarch64_string_literal_pointer_null_comparison.md`
  - `ideas/closed/370_aarch64_short_circuit_control_publication_regression.md`
- generated artifacts under `build/c_testsuite_aarch64_backend/`

## Current Scope

- current accepted full-suite baseline: `3347` passed, `28` failed,
  `3375` total
- backend-regex residual classification after closed ideas 366 and 370
- separation of local backend/unit failures from external
  `c_testsuite_aarch64_backend_*` residuals
- selection or creation of the next focused semantic owner

## Non-Goals

- Do not implement code under this inventory plan.
- Do not reopen closed focused ideas from failing counts alone.
- Do not change test expectations, unsupported classifications, allowlists,
  timeout policy, CTest registration, runner behavior, or proof-log policy.
- Do not run broad runtime scans without bounded commands and stale-process
  awareness.
- Do not fold unrelated residual families into one monolithic fix.

## Working Model

The accepted full-suite baseline after idea 370 is clean relative to the
current lifecycle guard: `3347` passed, `28` failed, and no pending baseline
review. The backend inventory parent should now reclassify the remaining
backend surface because the previous focused owners 366 and 370 have closed
and the residual ordering may have changed.

## Execution Rules

- Prefer current canonical logs when they already cover the needed surface.
- If a fresh backend-regex capture is needed, use the main build tree and a
  bounded command such as
  `ctest --test-dir build -j10 -R backend --output-on-failure`.
- Classify before splitting: local backend/unit, AArch64 external runtime,
  frontend/prepared-node diagnostics, semantic handoff, runtime mismatch,
  runtime nonzero/crash, and timeout/output-storm cases.
- Compare candidate owners against open and closed idea scopes before choosing
  a route.
- When a focused owner is ready, create or select that idea and switch
  lifecycle state before implementation begins.

## Steps

### Step 1: Capture Current Backend Surface

Goal: establish the current backend-regex residual surface after ideas 366 and
370.

Primary target: `test_baseline.log`, `test_after.log`, and, if necessary, a
fresh bounded `ctest -R backend` capture.

Actions:

- Verify whether existing logs are sufficient to reconstruct the current
  backend-regex residual list.
- If not sufficient, request or run the supervisor-approved backend-regex
  command into `test_after.log`.
- Record selected, passed, failed, and timeout counts.
- Identify whether any local backend/unit tests fail, or whether residuals are
  only external `c_testsuite_aarch64_backend_*` tests.

Completion check:

- `todo.md` records the current backend-regex residual counts and whether the
  surface is local-backend clean.

### Step 2: Classify Residual Buckets

Goal: group the residuals by first bad fact and backend owner boundary.

Primary target: failing test names, failure excerpts, generated assembly, and
prepared/semantic dumps for representative cases.

Actions:

- Split failures into compile/printer, semantic handoff, assembler/linker,
  runtime nonzero/crash, runtime mismatch, timeout, and output-storm buckets.
- For each plausible leading bucket, inspect one or two representative
  generated artifacts or diagnostics.
- Reject owners already satisfied by closed ideas unless fresh proof
  contradicts their closure boundary.
- Keep timeout-only cases quarantined unless a safe timeout-specific owner is
  the best next split.

Completion check:

- `todo.md` records classified buckets and the candidate semantic owner
  ranking.

### Step 3: Select Or Split Focused Owner

Goal: choose one tractable focused owner for implementation outside this
umbrella plan.

Primary target: the highest-signal classified bucket with semantic shared
ownership.

Actions:

- Select an existing open idea if it exactly owns the leading bucket by
  current evidence.
- Otherwise create a new `ideas/open/*.md` focused owner with concrete
  acceptance criteria and reviewer reject signals.
- Keep the owner narrow enough for implementation and proof.
- Preserve remaining parked buckets in `todo.md` for the supervisor handoff.

Completion check:

- A focused source idea exists or is selected, and `todo.md` explains why it
  is the next owner.

### Step 4: Handoff Lifecycle

Goal: leave the umbrella inventory parked and activate the focused owner
before code edits begin.

Primary target: `plan.md`, `todo.md`, and the selected focused idea.

Actions:

- Record a durable deactivation note in the inventory source idea only if the
  owner decision needs to survive beyond `todo.md`.
- Ask plan-owner to switch lifecycle state to the focused owner.
- Do not delegate implementation while this umbrella plan remains active.

Completion check:

- The active lifecycle state no longer points at idea 295 before any
  implementation work starts.
