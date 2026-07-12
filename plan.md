# Block-Entry Publication Identity Completion Runbook

Status: Active
Source Idea: ideas/open/718_block_entry_publication_identity_completion.md
Activated after parking: ideas/open/719_bir_cfg_edge_publication_source_identity_completion.md

## Purpose

Restore exact agreement between available prepared block-entry publication
facts and the BIR semantic publication identity view.

## Goal

Preserve exact successor, destination, type, PHI instruction, and proof
attribution across the prepared-to-BIR semantic publication boundary.

## Core Rule

Resolve publication identity from attributed semantic evidence. Never recover
it by source order, nearest PHI, display name alone, or target-emission facts.

## Read First

- `ideas/open/718_block_entry_publication_identity_completion.md`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `src/backend/mir/query.h`
- `src/backend/mir/query.cpp`

## Current Scope

- Prepared current-block entry publication readiness.
- `find_bir_block_entry_publication_identity` and its semantic evidence path.
- Exact successor, destination value ID/name/type, PHI instruction index, and
  proof attribution.
- Positive and fail-closed proof across multiple publication shapes.

## Non-Goals

- Do not change prepared-call plans, argument materializability, or ABI policy.
- Do not change join-source or edge-publication identity.
- Do not change target materialization, storage hooks, register spelling, move
  order, or emitted publication policy.
- Do not restore route numbers or alter printer/debug vocabulary.

## Execution Rules

- Establish the earliest prepared-to-BIR identity divergence before repair.
- Generalize beyond the currently failing call-contract fixture.
- Preserve typed unavailable results for incomplete or inconsistent evidence.
- Use only supervisor-delegated build, focused, and broader proof commands.

## Ordered Steps

### Step 1: Localize the prepared-to-BIR publication divergence

Goal: identify the earliest point where available prepared publication facts
lose or change semantic successor, destination, instruction, or attribution
identity.

Actions:

- Trace prepared current-block entry readiness into
  `find_bir_block_entry_publication_identity`.
- Map successor, destination ID/name/type, PHI instruction index, and proof
  attribution to their owning facts.
- Classify missing, wrong-successor, wrong-destination, wrong-type, stale,
  duplicate, and unattributed paths before selecting a repair seam.

Completion check:

- `todo.md` records the first divergence, owning helper, authority map, and a
  general repair rule without an implementation change.

### Step 2: Restore exact semantic publication identity

Goal: carry exact available publication identity through the smallest common
prepared-to-BIR semantic boundary.

Actions:

- Repair the smallest common boundary at the first incorrect fact.
- Preserve exact successor, destination ID/name/type, PHI instruction index,
  and proof attribution.
- Reject incomplete or inconsistent evidence without synthesizing identity.
- Keep prepared-call and target publication behavior outside the repair.

Completion check:

- Available attributed rows expose exact semantic identity and all negative
  rows fail closed without source-order, nearest-PHI, display-name-only, or
  target-emission selection.

### Step 3: Prove publication identity across nearby shapes

Goal: lock the semantic publication contract beyond the first call-contract
fixture.

Actions:

- Add focused positive assertions across multiple publication shapes.
- Add missing, wrong-successor, wrong-destination, wrong-type, stale,
  duplicate, and unattributed proof.
- Run the supervisor-delegated build and focused publication subset.

Completion check:

- Focused tests prove exact available identity and typed fail-closed behavior
  without expectation downgrade or fixture-shaped recovery.

### Step 4: Run acceptance proof and resume parked work

Goal: establish closure-quality evidence and return to idea 719 acceptance.

Actions:

- Audit the diff for fixture, name, source-order, nearest-PHI, proof-bit, and
  target-emission shortcuts.
- Run the supervisor-selected broader backend before/after comparison.
- Record whether idea 719 can resume its focused and broader acceptance checks
  before routing downstream idea 717 or idea 716.

Completion check:

- Focused and broader proof are green, reviewer reject signals are absent, and
  lifecycle routing can return to idea 719 acceptance.
