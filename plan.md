# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Use the main-build `ctest -R backend` result as an umbrella inventory, then
split focused repair ideas only when a residual failure group points to a
semantic backend capability instead of one testcase shape.

## Goal

Classify the post-301 backend-regex residual failures and either split the
next focused semantic owner or record why no focused owner is ready.

## Core Rule

This umbrella is for classification and lifecycle splitting only. Do not
implement fixes while this plan is active.

## Read First

- Source idea: `ideas/open/295_backend_regex_failure_family_inventory.md`
- Accepted broad proof baseline: `test_before.log`
- Latest accepted backend-regex state after idea 301: 352 selected, 300
  passed, 52 failed
- Main build tree: `/workspaces/c4c/build`
- Backend regex command:
  `cd /workspaces/c4c/build && ctest -j10 -R backend --output-on-failure`

## Current Scope

- Reconstruct or refresh the current backend-regex failure inventory.
- Separate local backend/unit failures from `c_testsuite_aarch64_backend_*`
  failures.
- Classify remaining frontend/backend diagnostics, runtime nonzero,
  runtime mismatch/crash, and timeout buckets.
- Compare any candidate owner against closed owners 285 through 301 before
  splitting.
- Create focused `ideas/open/*.md` files only for semantic repair families.
- Switch lifecycle state to a focused owner before any implementation starts.

## Non-Goals

- Do not edit implementation files or tests under this umbrella.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- Do not reopen closed owners 285 through 301 from counts alone.
- Do not treat all `ctest -R backend` failures as one repair bucket.
- Do not match exact testcase names, emitted instruction strings, or narrow
  diagnostics as a substitute for semantic ownership.
- Do not run broad runtime scans without timeout awareness and stale-process
  cleanup.

## Working Model

- The backend regex is an imprecise selector that includes local backend tests
  plus external AArch64 c-testsuite backend runtime tests.
- Counts are inventory evidence, not repair proof.
- A focused split needs a shared backend capability, a bounded test family,
  explicit out-of-scope residuals, and reviewer reject signals.
- Timeout or output-storm cases should be quarantined, deferred, or split into
  a hang-specific owner rather than mixed into ordinary runtime repair.

## Execution Rules

- Keep routine classification notes in `todo.md`.
- Edit this plan only if the umbrella route or step ordering needs repair.
- Edit the source idea only for durable deactivation notes or newly discovered
  source-intent changes.
- When a focused owner is split, switch lifecycle state away from this umbrella
  before code edits begin.
- Use `test_after.log` only when the supervisor explicitly asks for a fresh
  proof artifact; otherwise do not churn canonical logs during lifecycle-only
  classification.

## Steps

### Step 1: Reconstruct the Post-301 Inventory

Goal: establish the active residual set from accepted proof before choosing a
focused split.

Primary target: `test_before.log`

Actions:

- Parse the backend-regex proof summary and confirm the accepted state is 352
  selected, 300 passed, and 52 failed.
- Extract the failing test names and classify whether any failures are local
  backend/unit tests or all are `c_testsuite_aarch64_backend_*`.
- Preserve the current inventory in `todo.md` without editing the source idea.

Completion check:

- `todo.md` records the current failure list, pass/fail counts, and top-level
  source split.

### Step 2: Classify Residual Families

Goal: group the 52 residual failures by observable failure mode and likely
semantic backend owner.

Primary target: backend-regex failure output in `test_before.log`

Actions:

- Separate frontend/backend diagnostic failures from runtime nonzero,
  runtime mismatch/crash, and timeout cases.
- For each non-runtime diagnostic bucket, identify the shared backend
  capability if one is visible.
- For runtime buckets, record what extra generated-code or narrower proof
  would be needed before a semantic owner can be split.
- Keep standalone timeout cases separate unless there is direct evidence for a
  shared hang-specific owner.

Completion check:

- `todo.md` has a classified residual table with owner candidates, parked
  buckets, and evidence gaps.

### Step 3: Check Closed-Owner Boundaries

Goal: avoid reopening recently closed AArch64 owners without contradicting
proof evidence.

Primary target: closed-owner closure notes already summarized in idea 295

Actions:

- Compare candidate buckets against the closure boundaries for ideas 285
  through 301.
- Reject any candidate whose only evidence is a remaining count or testcase
  name that overlaps a closed owner.
- Require generated-code, diagnostic, or proof evidence before proposing a
  reopened owner.

Completion check:

- `todo.md` states whether the candidate owner is new, parked, or a legitimate
  reopen candidate with supporting evidence.

### Step 4: Split or Park the Next Owner

Goal: produce the next focused lifecycle target or explicitly defer when no
semantic owner is ready.

Primary target: `ideas/open/`

Actions:

- If a crisp semantic owner is found, create one focused
  `ideas/open/*.md` file with scope, out-of-scope residuals, acceptance
  criteria, and concrete reviewer reject signals.
- Add a durable deactivation note to idea 295 only when switching away from
  the umbrella.
- If no owner is ready, leave the umbrella active and record the blocker and
  needed probe in `todo.md`.

Completion check:

- Either a focused owner is ready for lifecycle switch, or `todo.md` explains
  why classification must continue before splitting.

### Step 5: Lifecycle Switch Before Implementation

Goal: ensure implementation starts only under a focused repair idea.

Primary target: `plan.md`, `todo.md`, and the selected focused idea under
`ideas/open/`

Actions:

- Deactivate the umbrella only after preserving durable owner decisions in
  idea 295.
- Activate the focused owner into a new `plan.md` and aligned `todo.md`.
- Do not include implementation edits in the lifecycle switch.

Completion check:

- Active lifecycle state points to the focused owner, not idea 295, before any
  code-changing executor packet is delegated.
