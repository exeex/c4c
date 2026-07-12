# Cursor-Complete Prepared Call Plan Production Runbook

Status: Active
Source Idea: ideas/open/716_prepared_call_plan_cursor_complete_production.md
Activated after parking: ideas/open/708_x86_named_handoff_materializer_cleanup.md

## Purpose

Repair common prepared-call production so target consumers receive complete,
cursor-exact authority for every supported semantic callsite.

## Goal

Publish exactly one correctly keyed `PreparedCallPlan` per supported semantic
call and prove that incomplete or inconsistent plan identity still fails
closed.

## Core Rule

Common preparation owns call enumeration and identity. Consumers must never
recover a missing plan by source order, nearest cursor, callee-name special
case, or route-derived fallback.

## Read First

- `ideas/open/716_prepared_call_plan_cursor_complete_production.md`
- `ideas/open/708_x86_named_handoff_materializer_cleanup.md`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`

## Current Scope

- Common semantic-call enumeration and `PreparedCallPlan` production.
- Block/instruction cursor, callee, argument, result, ABI, move, and
  preservation identity.
- Producer-level mixed-call proof and affected x86 boundary proof.

## Non-Goals

- Do not change target materializers or restore Route 6 authority.
- Do not change ABI classification or supported call behavior.
- Do not absorb memory, publication, joined-control, or route-vocabulary work.
- Do not weaken exact lookup or fail-closed behavior.

## Execution Rules

- Establish the first incorrect producer fact before changing code.
- Repair semantic enumeration/cursor accounting generally across supported
  call shapes; reject fixture-shaped or callee-shaped shortcuts.
- Keep exact identity validation intact and add negative proof alongside the
  positive producer repair.
- Use the supervisor-delegated build and focused tests for each code step, then
  broader regression proof before closure.

## Ordered Steps

### Step 1: Trace the missing-call producer path

Goal: identify where the first semantic call is filtered, overwritten, or
assigned the later call's cursor.

Actions:

- Trace the supported direct-extern fixture from semantic instructions through
  common call enumeration into `PreparedCallPlansFunction::calls`.
- Compare the two semantic calls' block, cursor, callee, arguments, result,
  ABI, move-bundle, and preservation inputs.
- Identify the earliest incorrect producer fact and nearby supported call
  shapes that exercise the same rule.

Completion check:

- The exact common producer defect is localized, and the proposed repair is a
  semantic rule that covers more than the named fixture.

### Step 2: Produce one cursor-exact plan per supported call

Goal: repair common production without weakening consumer identity checks.

Actions:

- Correct call enumeration, filtering, or cursor accounting at the owning
  common layer.
- Preserve complete callee, argument, result, ABI, move, and preservation
  identity for every emitted plan.
- Ensure zero-argument fixed-arity, variadic, result-producing, and adjacent
  supported calls follow the same production rule.

Completion check:

- The mixed direct-extern function publishes distinct correct plans at cursors
  0 and 1, and nearby supported shapes retain correct plan identity.

### Step 3: Prove positive completeness and negative rejection

Goal: lock the producer contract without introducing fallback authority.

Actions:

- Add producer-focused assertions for plan count and exact block/cursor/callee/
  argument identity across adjacent mixed-shape calls.
- Prove missing, duplicate, stale-cursor, and callee/cursor mismatch states are
  unavailable or rejected by exact lookup/consumption.
- Run the affected x86 direct-extern boundary test without changing its
  supported expectation.

Completion check:

- Producer-focused positive and negative tests plus the x86 boundary proof are
  green, with no expectation downgrade or target-local synthesis.

### Step 4: Run acceptance proof and hand back to idea 708

Goal: establish closure-quality evidence and unblock the parked consumer work.

Actions:

- Audit the diff for fixture, callee-name, source-order, nearest-plan, or route
  shortcuts.
- Run the supervisor-selected broader backend before/after regression guard.
- Record the proven producer contract needed when idea 708 resumes Step 2.

Completion check:

- Focused and broader proof are green, reviewer reject signals are absent, and
  idea 708 can resume with cursor-complete common prepared-call authority.
