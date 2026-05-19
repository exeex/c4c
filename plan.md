# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md
Activated from: post-311 lifecycle state, with no active `plan.md` or `todo.md`

## Purpose

Use the main build's backend CTest regex as an umbrella inventory, classify the
remaining failures by semantic owner, and split the next focused repair idea
before any implementation work begins.

## Goal

Produce a current, classified backend-regex failure inventory after recently
closed focused AArch64 owners, then hand execution to one focused semantic
owner or record why no owner is ready.

## Core Rule

This runbook is inventory-only. Do not implement backend fixes, change test
expectations, alter unsupported classifications, edit runners, change timeout
policy, or adjust CTest registration while this umbrella plan is active.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- current `test_before.log` / `test_after.log`, if present
- generated AArch64 c-testsuite artifacts under
  `build/c_testsuite_aarch64_backend/`, when classification needs emitted
  assembly or diagnostics
- closed-owner boundaries summarized in the source idea for ideas 285 through
  311

## Current Targets

- Main build tree: `/workspaces/c4c/build`
- Backend regex command shape:
  `ctest -j10 -R backend --output-on-failure`
- AArch64 backend c-testsuite output tree:
  `build/c_testsuite_aarch64_backend/`
- Residual buckets parked by the source idea: direct-call shuffle, direct
  vararg, address-of-local, runtime nonzero, runtime mismatch/crash,
  timeout/output-storm, remaining machine-printer or prepared-node residuals,
  and semantic `lir_to_bir` residuals.

## Non-Goals

- Do not fix failures under this umbrella plan.
- Do not treat `ctest -R backend` as one monolithic failure family.
- Do not reopen closed owners 285 through 311 from pass counts alone.
- Do not prove progress by expectation rewrites, allowlists, unsupported
  downgrades, timeout changes, runner behavior, or CTest registration changes.
- Do not run broad runtime scans without stale-process cleanup and timeout
  hygiene.

## Working Model

The backend regex currently includes both local backend tests and external
AArch64 c-testsuite backend runtime tests. A useful inventory separates local
backend/unit/CLI failures from external AArch64 runtime, frontend, printer,
assembler, timeout, and `lir_to_bir` failures before selecting an owner.

Recently closed focused owners remain closed unless fresh generated-code,
diagnostic, or proof evidence contradicts their closure boundaries.

## Execution Rules

- Preserve routine packet progress in `todo.md`; do not churn this runbook for
  per-command observations.
- Prefer reconstructing from accepted canonical logs when they already cover
  the current question. Run a fresh broad backend regex only when the existing
  logs are stale or insufficient.
- If running the broad backend regex, use the main build tree and include
  stale-process cleanup and timeout safeguards suitable for the known
  c-testsuite runtime surface.
- Classify by semantic owner, not by filename shape alone.
- When a tractable owner is found, create or request a focused
  `ideas/open/*.md` owner and switch lifecycle state before code edits.
- If no owner is ready, record the exact missing evidence and recommended
  probe in `todo.md`.

## Ordered Steps

### Step 1: Establish Current Backend Regex Evidence

Goal: determine the freshest trustworthy backend-regex baseline after the
closed focused owners, especially idea 311.

Primary target: `test_before.log`, `test_after.log`, and the main build CTest
state.

Actions:

- Inspect existing canonical logs for selected count, pass/fail count, and
  whether they represent the broad backend regex or a narrow focused scope.
- If the existing logs are narrow or stale, prepare a safe fresh broad backend
  capture from `/workspaces/c4c/build`.
- Record in `todo.md` whether the inventory is reconstructed or freshly run.
- Preserve the exact command, working directory, selected count, passed count,
  failed count, and timeout/hang safeguards used.

Completion check:

- `todo.md` names the evidence source for the current inventory and records the
  current backend-regex selected/pass/fail counts or the blocker preventing a
  safe capture.

### Step 2: Classify Residual Failures

Goal: split the backend-regex failures into actionable buckets without
reopening closed owners by count alone.

Primary target: backend-regex failure output and generated artifacts under
`build/c_testsuite_aarch64_backend/`.

Actions:

- Separate local backend/unit/CLI failures from
  `c_testsuite_aarch64_backend_*` failures.
- Classify AArch64 c-testsuite failures by observed stage:
  frontend/prepared-node, `lir_to_bir`, machine-printer, assembler/linker,
  runtime nonzero, runtime mismatch/crash, timeout, and output storm.
- Compare diagnostics and generated artifacts against the closed-owner
  boundaries summarized in the source idea for ideas 285 through 311.
- Record ambiguous buckets with the exact missing fact needed to split them.

Completion check:

- `todo.md` contains a current classified inventory with counts, representative
  tests, diagnostics or artifact evidence, and explicit closed-owner
  non-reopen notes where applicable.

### Step 3: Select the Next Semantic Owner

Goal: identify the highest-value focused repair owner, or decide that no owner
is ready without another probe.

Primary target: the largest crisp residual bucket with shared semantic
evidence.

Actions:

- Prefer semantic backend capability families over singleton test names.
- Accept a singleton only when it is semantically crisp and backed by concrete
  generated-code, diagnostic, or proof evidence.
- Reject owners that would require expectation changes, unsupported
  downgrades, runner edits, or filename-shaped fixes.
- Define focused proof scope before any implementation starts.

Completion check:

- `todo.md` records either the selected owner candidate with scope and proof
  command, or the reason no owner is ready and the next narrow probe needed.

### Step 4: Split and Switch Lifecycle State

Goal: leave the umbrella inventory parked and move implementation to a focused
owner before code changes.

Primary target: a new focused `ideas/open/*.md` owner when Step 3 finds one.

Actions:

- Create a focused source idea only for the selected semantic owner.
- Include concrete in-scope and out-of-scope boundaries, acceptance criteria,
  proof scope, and reviewer reject signals.
- Add a durable deactivation note to
  `ideas/open/295_backend_regex_failure_family_inventory.md` summarizing the
  inventory decision, proof scope, remaining parked buckets, and active owner.
- Switch lifecycle state to the focused owner before implementation begins.

Completion check:

- Either a focused owner is active with matching `plan.md` and `todo.md`, or
  this umbrella remains active with `todo.md` explaining why the split is
  blocked.
