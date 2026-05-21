# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md
Reactivated After: ideas/closed/377_aarch64_external_libc_call_result_publication.md

## Purpose

Refresh the backend-regex residual inventory after the focused external/libc
call-result publication owner was closed, then select the next focused
semantic owner before any implementation work starts.

## Goal

Classify the current backend-matching failure surface after idea 377 and split
or select one tractable repair owner from the remaining parked buckets.

## Core Rule

This is an umbrella classification plan. Do not implement fixes here, do not
change expectations or runners, and do not treat pass/fail counts alone as
owner evidence.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `todo.md`
- `test_before.log`
- `test_after.log`
- generated artifacts under `build/c_testsuite_aarch64_backend/`

## Current Scope

- backend-regex residual classification after closed idea 377
- separation of local backend/unit failures from external
  `c_testsuite_aarch64_backend_*` residuals
- reclassification of the parked residual buckets after `00187` was removed
  from the active implementation surface
- selection or creation of the next focused semantic owner

## Non-Goals

- Do not implement code under this inventory plan.
- Do not reopen closed focused ideas from failing counts alone.
- Do not change expectations, unsupported classifications, allowlists,
  timeout policy, CTest registration, runner behavior, or proof-log policy.
- Do not run broad runtime scans without bounded commands and stale-process
  awareness.
- Do not fold unrelated residual families into one monolithic fix.
- Do not activate a parked residual idea merely because it exists; require
  current first-bad-fact evidence that it is the next owner.

## Working Model

The latest durable umbrella handoff selected `00187` as the external/libc
call-result publication owner. That focused owner is now closed. The next
inventory pass should treat `00187` as removed from the active residual
surface unless fresh generated-code or proof evidence contradicts the closure
boundary.

The remaining ranked buckets from the umbrella are expected to include scalar
FP expression or constant materialization (`00174`), aggregate
initializer/compound relocation/function-pointer-table behavior (`00216`),
dynamic stack/VLA fixed-slot timeout (`00207`), and shift/type-promotion
timeout (`00200`). Treat this as a starting hypothesis, not as proof; the
active executor must classify the current surface before selecting a focused
owner.

## Execution Rules

- Prefer current canonical logs when they already cover the needed surface.
- If a fresh backend-regex capture is needed, use the main build tree and a
  bounded command such as
  `ctest --test-dir build -j10 -R backend --output-on-failure`.
- Classify before splitting: local backend/unit, AArch64 external runtime,
  frontend/prepared-node diagnostics, semantic handoff, assembler/linker,
  runtime mismatch, runtime nonzero/crash, and timeout/output-storm cases.
- Compare candidate owners against open idea scopes before choosing a route.
- Keep timeout-only cases quarantined unless a safe timeout-specific owner is
  the best next split.
- When a focused owner is ready, create or select that idea and switch
  lifecycle state before implementation begins.

## Steps

### Step 1: Capture Current Backend Surface

Goal: establish the current backend-regex residual surface after idea 377.

Primary target: `test_before.log`, `test_after.log`, and, if necessary, a
fresh bounded `ctest -R backend` capture.

Actions:

- Verify whether existing logs are sufficient to reconstruct the current
  backend-regex residual list.
- If not sufficient, request or run the supervisor-approved backend-regex
  command into `test_after.log`.
- Record selected, passed, failed, and timeout counts.
- Identify whether local backend/unit tests fail or whether residuals remain
  only external `c_testsuite_aarch64_backend_*`.
- Confirm that `00187` is absent from the current residual list before ranking
  the next bucket.

Completion check:

- `todo.md` records the current backend-regex residual counts and whether the
  surface is local-backend clean.

### Step 2: Classify Residual Buckets

Goal: group residuals by first bad fact and backend owner boundary.

Primary target: failing test names, failure excerpts, generated assembly, and
prepared/semantic dumps for representative cases.

Actions:

- Split failures into compile/printer, semantic handoff, assembler/linker,
  runtime nonzero/crash, runtime mismatch, timeout, and output-storm buckets.
- Re-check the parked ranked buckets after the `00187` closure.
- For each plausible leading bucket, inspect one or two representative
  generated artifacts or diagnostics.
- Reject owners already satisfied by recent focused ideas unless fresh proof
  contradicts their closure boundary.
- Keep timeout-only cases quarantined unless a safe timeout-specific owner is
  the best next split.

Completion check:

- `todo.md` records classified buckets and the candidate semantic owner
  ranking.

### Step 3: Select Or Split Focused Owner

Goal: choose one focused owner for implementation outside this umbrella plan.

Primary target: the highest-signal classified bucket with semantic shared
ownership.

Actions:

- Select an existing open idea if it exactly owns the leading bucket by
  current evidence.
- Otherwise create a new `ideas/open/*.md` focused owner with concrete
  acceptance criteria and reviewer reject signals.
- Keep the owner narrow enough for implementation and proof.
- Preserve remaining parked buckets in `todo.md` for supervisor handoff.

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
