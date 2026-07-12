# BIR CFG Edge-Publication Source Identity Completion Runbook

Status: Active
Source Idea: ideas/open/719_bir_cfg_edge_publication_source_identity_completion.md
Resumed after completing: ideas/closed/718_block_entry_publication_identity_completion.md

## Purpose

Finish acceptance of the independently resolved BIR CFG edge-publication
source identity now that its block-entry publication prerequisite is closed.

## Goal

Revalidate exact prepared/BIR CFG source agreement across supported producer
shapes, prove that the repaired route remains independent and fail closed, and
complete broader acceptance without absorbing downstream join or call work.

## Core Rule

Resolve the BIR CFG identity from `BirCfgEdgePublicationSourceRequest` and BIR
CFG authority before comparing it with prepared identity. Never use prepared
facts, vector order, display names, or target output as the BIR-side oracle.

## Read First

- `ideas/open/719_bir_cfg_edge_publication_source_identity_completion.md`
- `ideas/closed/718_block_entry_publication_identity_completion.md`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `src/backend/mir/query.cpp`

## Current Scope

- Re-run the focused source-family contract now that idea 718 is complete.
- Confirm load, cast, binary, select, memory-source, and bounded-authority
  assertions still use two independently established identities.
- Preserve typed failure for missing, stale, duplicate, mismatched, and
  unavailable authority.
- Obtain independent route-quality review and a matching broader backend
  before/after comparison before closure.

## Non-Goals

- Do not change prepared-MIR join-source preparation or prepared-call plans.
- Do not reopen block-entry publication identity.
- Do not change target materialization, route vocabulary, register allocation,
  or move scheduling.
- Do not treat the later idea-721 x86 prepared-core abort as idea-719 scope.

## Execution Rules

- Keep the BIR request live through exact predecessor, successor, destination,
  source, producer, and memory identity resolution.
- Reject source-order, nearest-instruction, display-name, prepared-fact, or
  target-output recovery.
- Do not weaken supported expectations or mark covered shapes unsupported.
- Use supervisor-delegated build and proof commands; record packet results in
  `todo.md` and canonical regression logs only as delegated.

## Ordered Steps

### Step 1: Revalidate the focused independent-identity boundary

Goal: establish that idea 718's completion allows the existing idea-719
source-family contract to run through its owned assertions without exposing a
new idea-719 defect.

Actions:

- Inspect the focused agreement helper and its current positive and fail-closed
  matrix for load, cast, binary, select, memory, and bounded authority shapes.
- Run the supervisor-delegated build and focused
  `backend_prepared_lookup_helper` proof.
- Record the first failing assertion or exception, if any, and classify its
  ownership before changing code.
- If an idea-719-owned failure remains, repair the general BIR CFG identity
  rule and extend nearby proof without testcase-shaped matching.

Completion check:

- The focused idea-719 source-family assertions pass with independent BIR CFG
  authority, or a precise owned semantic defect and bounded repair packet are
  documented in `todo.md`.

### Step 2: Audit route quality and focused acceptance

Goal: prove the completed implementation matches the source contract and has
not retained a circular prepared-fact oracle behind a new helper.

Actions:

- Review the implementation and focused tests from the idea-719 activation
  history point through `HEAD`.
- Reject fixture, row-order, display-name, prepared-fact, expectation, or
  target-output shortcuts.
- Run any supervisor-delegated focused before/after comparison needed to prove
  an owned correction.

Completion check:

- Independent review reports no blocking source-alignment, overfit, or proof
  finding, and focused acceptance is green without expectation changes.

### Step 3: Run broader acceptance proof and close

Goal: complete the source idea's broader regression requirement and return to
the dependency chain that resumes idea 717 and then idea 716.

Actions:

- Run the supervisor-selected matching broader backend before/after comparison
  using canonical regression logs.
- Confirm no new failure and no loss of covered passes.
- Request lifecycle closure only after reviewer and regression acceptance.

Completion check:

- Broader acceptance is green, reviewer reject signals are absent, and idea
  719 can close before resuming idea 717.
