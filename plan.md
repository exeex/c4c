# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Refresh and classify the main-build backend regex failure inventory after
focused AArch64 owner 339 closed, then split the next semantic repair owner
before implementation begins.

## Goal

Use the current `/workspaces/c4c/build` backend regex result to identify the
next tractable semantic failure family without reopening closed owners from
counts alone.

## Core Rule

Do not implement repairs under this umbrella. Capture evidence, classify
failures by semantic owner, and switch lifecycle state to a focused idea before
code changes begin.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- Recent closure context from the source idea:
  - idea 339 is closed as the scalar local storage/writeback sizing owner
  - closed focused owners 333 through 339 must stay closed unless fresh
    generated-code or proof evidence contradicts a closure boundary
- Current backend regex command:
  - `ctest --test-dir build -j10 -R backend --output-on-failure`

## Current Targets

- Main build tree: `/workspaces/c4c/build`
- Backend regex scope selected by `ctest -R backend`
- External AArch64 c-testsuite backend residuals:
  - `c_testsuite_aarch64_backend_*`
- Local backend/unit/CLI failures if any appear in the refreshed regex result

## Non-Goals

- Do not edit implementation code.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or proof-log contents.
- Do not treat `ctest -R backend` as one monolithic failure bucket.
- Do not reopen closed focused owners from pass/fail counts alone.
- Do not create filename-only, instruction-string-only, diagnostic-string-only,
  or c-testsuite-number-specific repair routes.

## Working Model

`ctest -R backend` is an imprecise umbrella selector. It can include local
backend tests, AArch64 external runtime tests, frontend/prepared handoff
failures, timeout/output-storm cases, and residual families that need separate
owners. The job of this runbook is to classify the current failure surface and
split the next focused owner only when evidence points to a semantic backend
capability.

## Execution Rules

- Keep progress notes and proof details in `todo.md`.
- Use `test_before.log` and `test_after.log` only as canonical proof logs when
  the supervisor delegates matching regression-log preparation.
- Broad runtime scans should use timeout/stale-process cleanup when needed.
- If a focused owner is split, write the new idea under `ideas/open/` with
  concrete reviewer reject signals, then switch lifecycle state before any
  implementation.
- If no owner is ready, record the classified buckets and blocker in `todo.md`
  instead of forcing a weak split.

## Steps

### Step 1: Capture Current Backend Regex Inventory

Goal: establish the post-339 backend-regex failure set from the main build.

Primary target: `ctest --test-dir build -j10 -R backend --output-on-failure`

Actions:

- Confirm the build tree exists and is suitable for backend CTest execution.
- Run the supervisor-delegated backend regex command, with timeout cleanup if
  the supervisor requires it for runtime/output-storm safety.
- Record selected, passed, failed, timeout, and skipped counts in `todo.md`.
- Preserve the raw canonical proof log path selected by the supervisor.

Completion check:

- `todo.md` records a current backend regex inventory and the exact command
  used to produce it.

### Step 2: Classify Residual Failures

Goal: split the current failures into actionable semantic buckets.

Primary target: failures selected by Step 1.

Actions:

- Separate local backend/unit/CLI failures from
  `c_testsuite_aarch64_backend_*` failures.
- Classify each failure by first bad stage:
  - frontend/prepared-node or machine-printer diagnostics
  - semantic `lir_to_bir` admission or prepared-module handoff
  - assembler/linker legality
  - runtime nonzero/crash
  - runtime mismatch
  - timeout or output-storm
- Compare candidate buckets against closed focused owners through idea 339
  using generated-code or proof evidence, not counts alone.
- Record remaining parked buckets and any unsafe timeout/output-storm handling
  in `todo.md`.

Completion check:

- `todo.md` contains a classified inventory with closed-owner boundaries
  respected and no implementation route chosen from counts alone.

### Step 3: Select The Next Focused Owner

Goal: choose one focused semantic repair family, or explain why none is ready.

Primary target: the highest-value bucket from Step 2 with concrete evidence.

Actions:

- Prefer a bucket with multiple related failures and a shared generated-code,
  diagnostic, ABI, or lowering owner.
- Allow a singleton only when evidence proves a semantic capability rather
  than a named testcase shortcut.
- Define the candidate owner's scope, non-goals, representative tests, and
  proof ladder.
- Reject buckets that would require expectation, runner, timeout, or CTest
  changes to claim progress.

Completion check:

- `todo.md` names the selected focused owner and evidence, or explains why no
  focused owner is ready to split.

### Step 4: Split And Switch Lifecycle State

Goal: leave the umbrella after classification and hand implementation to a
focused idea.

Primary target: `ideas/open/<new-focused-owner>.md`

Actions:

- Create a focused source idea with goal, scope, non-goals, acceptance
  criteria, and concrete reviewer reject signals.
- Add a durable deactivation note to
  `ideas/open/295_backend_regex_failure_family_inventory.md` recording the
  inventory count, selected owner, proof scope, and remaining buckets.
- Replace this umbrella runbook with a focused `plan.md` and reset `todo.md`
  through the plan-owner lifecycle workflow.
- Do not edit implementation code before the lifecycle switch is complete.

Completion check:

- A focused idea is active in `plan.md` and `todo.md`, or the umbrella remains
  active only with a documented blocker in `todo.md`.
