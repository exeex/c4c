# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md
Activated After: ideas/closed/365_aarch64_signed_remainder_lowering.md

## Purpose

Reactivate the umbrella backend inventory after the signed remainder owner
closed, so the next focused repair owner is selected from current evidence.

## Goal

Classify the current backend failure surface, separate unrelated buckets, and
split or select exactly one focused semantic owner before implementation work
continues.

## Core Rule

This umbrella is for classification and lifecycle routing only. Do not make
implementation edits under this plan, and do not claim progress from pass
counts, expectation changes, runner changes, or unsupported reclassification.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `test_baseline.log`
- `test_after.log`
- current `ctest -R backend` output if the supervisor delegates a fresh run
- generated AArch64 artifacts for any selected representative failures
- existing open ideas before creating a new focused owner

## Current Scope

- current main-build backend-regex inventory after idea 365 closure
- c-testsuite AArch64 backend failures still present in the accepted baseline
- residual buckets from the latest inventory notes that remain unassigned
- evidence-based selection of one focused owner under `ideas/open/`

## Non-Goals

- Do not edit implementation files, tests, expectations, runners, timeout
  policy, unsupported classifications, CTest registration, or proof-log
  policy.
- Do not reactivate parked owners solely because their files remain under
  `ideas/open/`.
- Do not reopen closed ideas or mutate `ideas/closed/*`.
- Do not continue idea 364 synthetic-label work or idea 365 signed-remainder
  work unless fresh evidence contradicts their closure or parked status.

## Working Model

The previous focused owner, idea 365, closed after the full-suite guard
resolved `c_testsuite_aarch64_backend_src_00143_c` with no new failures.
Several open idea files are parked or closure-deferred because their source
scope was satisfied but archival close was blocked by regression-guard policy.
The next useful action is an umbrella classification pass against current
baseline evidence, followed by activation or creation of one focused owner.

## Execution Rules

- Start from the accepted current baseline and recent full-suite guard logs;
  request a fresh backend-regex run only if those logs are insufficient for
  classification.
- Classify by semantic owner, not by testcase name or emitted-text shape.
- Before creating a new idea, check whether an existing open idea already owns
  the current first bad fact and is not merely parked for closure.
- If a focused owner is selected, update lifecycle state to that idea before
  any implementation packet begins.
- Leave durable inventory findings in `todo.md` first; update the source idea
  only for a durable deactivation or split note.

## Steps

### Step 1: Capture Current Backend Inventory

Goal: establish the current failure surface after idea 365 closed and the
accepted full-suite baseline improved.

Primary target: `test_baseline.log`, `test_after.log`, and, if delegated by
the supervisor, a fresh `ctest --test-dir build -j --output-on-failure -R
backend` log.

Actions:

- Record the baseline commit, selected scope, pass/failure count, and failed
  test names.
- Separate local backend/unit failures from external AArch64 c-testsuite
  backend failures.
- Note which recently active representatives now pass, especially `00143`.
- Identify any failures that require generated assembly, dumps, or timeout
  quarantine before classification.

Completion check:

- `todo.md` contains the current classified inventory source, counts, failed
  tests, and any evidence gaps that block owner selection.

### Step 2: Classify Residual Buckets

Goal: group the current failures into semantic owners and reject stale parked
handoffs.

Primary target: failed AArch64 backend representatives and nearby generated
artifacts or dump tests.

Actions:

- Compare each candidate bucket against existing open ideas.
- Mark parked closure-deferred ideas as non-active unless fresh evidence shows
  their exact owner has returned.
- For each viable bucket, name the first bad fact, owning boundary, and proof
  artifact needed for a focused packet.
- Quarantine timeout-only cases unless there is a safe bounded reproducer.

Completion check:

- `todo.md` records the leading focused owner candidate, rejected adjacent
  owners, and the evidence for the selection.

### Step 3: Split Or Select One Focused Owner

Goal: leave the umbrella plan only after one implementation-ready owner exists.

Primary target: `ideas/open/*.md`, plus `plan.md` / `todo.md` lifecycle state.

Actions:

- If an existing open idea owns the selected bucket, switch lifecycle state to
  that idea.
- If no existing idea owns it, create a new focused idea with concrete scope,
  acceptance criteria, and reviewer reject signals.
- Preserve a compact deactivation note in idea 295 naming the selected owner,
  proof basis, and remaining parked buckets.
- Do not leave implementation instructions under this umbrella once a focused
  owner is selected.

Completion check:

- Active lifecycle state points to one focused source idea under
  `ideas/open/`, or `todo.md` states the exact classification blocker.
