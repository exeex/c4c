# Cursor-Complete Prepared Call Plan Production Runbook

Status: Active
Source Idea: ideas/open/716_prepared_call_plan_cursor_complete_production.md
Activated after parking: ideas/open/708_x86_named_handoff_materializer_cleanup.md

## Purpose

Repair common prepared-call argument production so target consumers receive
complete, cursor-exact authority for every supported semantic callsite.

## Goal

Publish exactly one correctly keyed `PreparedCallPlan` per supported semantic
call by using semantic operands as base argument identity, permitting only
unique compatible relationship refinement, and keeping inconsistent evidence
fail closed.

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

- Use Step 1's proven first incorrect producer fact: cursor assignment is
  already exact; absent optional argument-source metadata incorrectly erases
  an otherwise valid argument-bearing call.
- Repair semantic operand/refinement handling generally across supported call
  shapes; reject fixture-shaped or callee-shaped shortcuts.
- Keep exact identity validation intact and add negative proof alongside the
  positive producer repair.
- Use the supervisor-delegated build and focused tests for each code step, then
  broader regression proof before closure.

## Ordered Steps

### Step 1: Trace the missing-call producer path (complete)

Goal: identify why one of the two semantic calls is omitted and verify whether
stored cursor identity is correct.

Actions:

- Trace the supported direct-extern fixture from semantic instructions through
  common call enumeration into `PreparedCallPlansFunction::calls`.
- Compare the two semantic calls' block, cursor, callee, arguments, result,
  ABI, move-bundle, and preservation inputs.
- Identify the earliest incorrect producer fact and nearby supported call
  shapes that exercise the same rule. Confirmed: cursor 0 is published exactly;
  cursor 1 is dropped because absent optional argument-source metadata makes
  `find_call_argument` incomplete.

Completion check:

- Complete at commit `e5a52dd29`: the exact common producer defect is
  localized, cursor accounting is not the defect, and the proposed repair is
  a semantic operand/refinement rule covering more than the named fixture.

### Step 2: Produce one cursor-exact plan per supported call

Goal: repair common argument production without weakening consumer identity
checks or relationship ambiguity rejection.

Actions:

- Derive each base argument identity from `CallInst::args[arg_index]` at the
  owning common layer; absence of optional relationship metadata must not drop
  the call.
- Apply a unique compatible `CallArgumentSourceRelationship` only as a
  refinement of source/base ID, selection, aggregate-lane, and producer
  identity.
- Reject duplicate/ambiguous, out-of-range, stale, or semantic-operand-
  contradicting relationships; do not select the first row.
- Preserve complete callee, argument, result, ABI, move, and preservation
  identity for every emitted plan.
- Preserve the enclosing semantic instruction cursor as-is; do not repair or
  renumber already-correct cursor accounting.
- Ensure fixed-extern, variadic-extern, indirect, result-producing, and
  adjacent supported calls follow the same production rule.

Completion check:

- The mixed direct-extern function publishes distinct correct plans for
  `actual_function` at cursor 0 and argument-bearing `printf` at cursor 1;
  nearby argument-bearing direct-BIR shapes without relationships also publish
  exact plans, while contradictory or ambiguous evidence remains unavailable.

### Step 3: Prove positive completeness and negative rejection

Goal: lock the producer contract without introducing fallback authority.

Actions:

- Add producer-focused assertions for plan count and exact block/cursor/callee/
  argument identity across adjacent mixed-shape calls.
- Cover the no-relationship semantic-operand path and unique compatible
  refinement path across nearby fixed, variadic, and indirect calls.
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
