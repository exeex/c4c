# Prepared-MIR Join-Source Identity Completion Runbook

Status: Active
Source Idea: ideas/open/717_prepared_mir_join_source_identity_completion.md
Resumed after completing: ideas/closed/719_bir_cfg_edge_publication_source_identity_completion.md

## Purpose

Finish acceptance of the repaired common prepared-MIR join-source identity now
that the independently resolved BIR CFG edge-publication prerequisite is
closed.

## Goal

Revalidate complete typed join-source authority across supported and negative
move shapes, prove the route remains general and fail closed, and complete the
broader backend acceptance required before idea 716 can resume.

## Core Rule

Join-source identity comes from exact prepared publication, move, producer,
and freshness facts. Do not infer authority from row order, display names,
route vocabulary, prepared-call facts, or target behavior.

## Read First

- `ideas/open/717_prepared_mir_join_source_identity_completion.md`
- `ideas/closed/719_bir_cfg_edge_publication_source_identity_completion.md`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `src/backend/mir/prepared/`
- `src/backend/prealloc/prepared_lookups.cpp`

## Current Scope

- Re-run the focused prepared-MIR direct-edge join-source contract after idea
  719 restored the independent downstream CFG oracle.
- Confirm named, immediate, stack, and unsupported moves preserve their exact
  typed publication, producer, freshness, and move authority.
- Keep the aggregate BIR semantic join view unavailable when required
  publication authority is missing, stale, duplicate, mismatched, or
  unsupported.
- Obtain independent route-quality review and a matching broader backend
  before/after comparison before closure.

## Non-Goals

- Do not change prepared-call plan production or call lookup.
- Do not reopen block-entry or BIR CFG edge-publication identity.
- Do not change target materialization, x86 joined-branch emission, move
  scheduling, register allocation, or diagnostic vocabulary.

## Working Model

- Steps 1 through 3 of the original runbook completed the idea-owned typed
  prepared-MIR join-source repair and focused positive/negative coverage.
- Idea 719 was split out because the next focused failure belonged to an
  independent legacy BIR CFG oracle; that prerequisite is now closed.
- This resumed runbook owns revalidation and closure evidence, not a replay of
  the completed repair unless a focused failure identifies a genuine idea-717
  defect.

## Execution Rules

- Establish the first failing assertion and classify ownership before editing.
- Preserve supported facts even when another move is unsupported, while
  keeping aggregate BIR semantic identity fail closed when required.
- Reject fixture-row, vector-order, display-name, route, prepared-call, and
  target-output shortcuts.
- Do not weaken supported expectations or absorb downstream idea-716 work.
- Use supervisor-delegated build and proof commands and record execution state
  in `todo.md`.

## Ordered Steps

### Step 1: Revalidate the focused join-source boundary

Goal: prove the completed idea-717 repair now runs through its owned assertions
with the idea-719 prerequisite restored.

Actions:

- Inspect the focused prepared-MIR/BIR join-source agreement assertions and
  their supported and fail-closed matrix.
- Run the supervisor-delegated build and focused prepared lookup/join-source
  proof.
- Record the first failing assertion or exception and classify ownership
  before changing code.
- If an idea-717-owned defect remains, repair the general typed join-source
  rule and extend nearby proof without testcase-shaped matching.

Completion check:

- Focused named, immediate, stack, unsupported, missing, stale, duplicate, and
  mismatch assertions pass, or an exact owned semantic defect and bounded
  repair packet are documented in `todo.md`.

### Step 2: Audit route quality and focused acceptance

Goal: prove the implementation still matches the source contract and has not
retained incomplete or testcase-shaped authority.

Actions:

- Review the idea-717 implementation from its activation history point through
  `HEAD` against the source idea and this resumed runbook.
- Reject row-order, display-name, route, expectation, prepared-call, or target
  shortcuts and any weakening of aggregate fail-closed behavior.
- Run any supervisor-delegated focused before/after proof required for an
  owned correction.

Completion check:

- Independent review reports no blocking source-alignment, overfit, or proof
  finding, and focused acceptance is green without expectation changes.

### Step 3: Run broader acceptance proof and close

Goal: satisfy the source idea's broader regression requirement and unblock
idea 716.

Actions:

- Run the supervisor-selected matching broader backend before/after comparison
  using canonical regression logs.
- Confirm no new failures and no loss of covered passes.
- Request lifecycle closure only after reviewer and regression acceptance.

Completion check:

- Broader acceptance is green, reviewer reject signals are absent, and idea
  717 can close before resuming idea 716.
