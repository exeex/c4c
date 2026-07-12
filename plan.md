# Prepared-MIR Join-Source Identity Completion Runbook

Status: Active
Source Idea: ideas/open/717_prepared_mir_join_source_identity_completion.md
Activated after parking: ideas/open/716_prepared_call_plan_cursor_complete_production.md

## Purpose

Repair the common prepared-to-MIR join-source identity boundary that currently
blocks focused prepared-call acceptance after the call assertions pass.

## Goal

Expose complete typed authority for supported direct-edge publication sources,
retain explicit unsupported states, and keep aggregate BIR join identity fail
closed when required authority is incomplete.

## Core Rule

Join-source identity comes from exact prepared publication, move, producer,
and freshness facts. Do not infer it from row order, names, routes, or target
behavior.

## Read First

- `ideas/open/717_prepared_mir_join_source_identity_completion.md`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `src/backend/mir/prepared/`
- `src/backend/prealloc/prepared_lookups.cpp`

## Current Scope

- Prepared current-block join facts and prepared-MIR direct-edge source views.
- BIR semantic join identity adaptation and typed negative states.
- Focused common query proof.

## Non-Goals

- Do not change prepared-call production or lookup.
- Do not change block-entry publication identity.
- Do not change x86 or other target materializers.
- Do not weaken unsupported or incomplete join authority.

## Execution Rules

- Establish the first incorrect fact before editing behavior.
- Generalize across named, immediate, stack, and unsupported move shapes.
- Preserve supported facts even when another row is unsupported, while keeping
  the aggregate BIR semantic view fail closed where required.
- Use the supervisor-delegated build and focused tests for code steps.

## Ordered Steps

### Step 1: Localize the prepared-to-MIR join identity divergence

Goal: identify the earliest incorrect fact across preparation, prepared-MIR
adaptation, and the BIR semantic join view.

Actions:

- Trace all four fixture facts through the exact publication/move/freshness
  keys and typed statuses.
- Compare named, immediate, stack, and unsupported-move handling.
- Inspect nearby join shapes to select a semantic repair boundary rather than
  a fixture-shaped patch.

Completion check:

- The first incorrect fact, owning helper, and general repair rule are recorded
  in `todo.md`, with no implementation change required for localization.

### Step 2: Repair typed join-source identity propagation

Goal: preserve complete supported authority and explicit negative states.

Actions:

- Repair the owning common layer at the first incorrect fact.
- Preserve exact predecessor/successor, destination/source, producer,
  freshness, publication, and move identity.
- Keep unsupported, missing, stale, duplicate, and mismatched evidence typed
  and fail closed.
- Do not select authority by vector position or diagnostic vocabulary.

Completion check:

- Supported source shapes expose exact typed authority and negative shapes
  reject without weakening aggregate BIR join semantics.

### Step 3: Prove the generalized contract

Goal: lock positive and negative behavior beyond the original fixture.

Actions:

- Add focused assertions for multiple supported source kinds and identity
  keys.
- Add unsupported, missing, stale, duplicate, and mismatch rejection proof.
- Run the supervisor-delegated focused prepared lookup/join subset.

Completion check:

- Focused join-source tests are green without expectation downgrade or
  target-local synthesis.

### Step 4: Run acceptance proof and return to blocker routing

Goal: establish closure-quality evidence for this prerequisite.

Actions:

- Audit the diff for testcase, name, row-order, route, and target shortcuts.
- Run the supervisor-selected broader backend before/after comparison.
- Record whether idea 718 remains the only common prerequisite before idea 716
  can resume Step 3 acceptance.

Completion check:

- Focused and broader proof are green, reviewer reject signals are absent, and
  lifecycle routing can proceed to idea 718 or resume idea 716 as appropriate.
