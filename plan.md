# Cursor-Complete Prepared Call Plan Production Acceptance Runbook

Status: Active
Source Idea: ideas/open/716_prepared_call_plan_cursor_complete_production.md
Resumed after completing: ideas/closed/717_prepared_mir_join_source_identity_completion.md
Also restored prerequisite: ideas/closed/718_block_entry_publication_identity_completion.md

## Purpose

Resume acceptance of cursor-complete common prepared-call production now that
its two parked common identity prerequisites are closed.

## Goal

Prove that every supported semantic callsite publishes exactly one
cursor-exact `PreparedCallPlan`, that malformed or contradictory relationship
evidence remains fail closed, and that the repaired common authority is ready
for the parked idea-708 consumer work.

## Core Rule

`CallInst` operands provide base argument identity and only one compatible
source relationship may refine it. Never reconstruct missing call authority by
plan-vector position, nearest cursor, callee name, route, or target behavior.

## Read First

- `ideas/open/716_prepared_call_plan_cursor_complete_production.md`
- `ideas/closed/717_prepared_mir_join_source_identity_completion.md`
- `ideas/closed/718_block_entry_publication_identity_completion.md`
- `ideas/open/708_x86_named_handoff_materializer_cleanup.md`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`

## Current Scope

- Revalidate the completed semantic-operand/refinement producer repair.
- Prove exact block, instruction cursor, callee, argument, result, ABI, move,
  and preservation identity across adjacent supported call shapes.
- Prove exact lookup rejects missing, duplicate, stale, mismatched, and
  contradictory evidence.
- Obtain focused review and a broader backend comparison before closure.

## Non-Goals

- Do not change x86, AArch64, or RV64 materializers.
- Do not absorb idea-708 joined-branch or handoff materialization work.
- Do not change ABI classification or supported call semantics.
- Do not restore route-derived call authority or weaken supported
  expectations.

## Working Model

- Source-idea Steps 1 and 2 are complete: the first bad fact was optional
  relationship absence erasing a valid semantic operand, and common production
  now treats the relationship as an optional unique refinement.
- The source-idea Step 3 producer and exact-lookup assertions were completed at
  commit `021c869e9` before acceptance parked behind ideas 717 and 718.
- Ideas 717 and 718 are now closed. Resume by revalidating the owned call-plan
  assertions and classifying any later failure before editing.

## Execution Rules

- Establish the first failing assertion and its owner before changing code.
- Repair only general semantic operand/refinement or exact lookup rules owned
  by idea 716; reject fixture-, callee-, cursor-, and target-shaped shortcuts.
- Do not weaken an expectation to route around remaining idea-708 work.
- Use supervisor-delegated build and proof commands and record packet state in
  `todo.md`.

## Ordered Steps

### Step 1: Revalidate cursor-complete producer and lookup authority

Goal: prove the completed idea-716 assertions with both common prerequisites
restored.

Actions:

- Run the supervisor-delegated focused prepared-call producer and lookup proof.
- Confirm cursor-0 fixed-arity and cursor-1 argument-bearing variadic calls
  retain distinct exact plans, and cover nearby argument-bearing shapes with
  no relationship or one compatible refinement.
- Confirm missing, duplicate, stale-cursor, owner/callee mismatch, out-of-range,
  ambiguous, and operand-contradicting evidence remains unavailable.
- Classify any assertion or later executable failure before editing; do not
  absorb idea-708 target/joined-control work.

Completion check:

- All idea-716-owned producer and exact-lookup assertions are green without
  expectation changes, and any remaining failure is either repaired by a
  general owned rule or documented with its independent owner.

### Step 2: Audit focused acceptance and route quality

Goal: independently verify the common producer repair satisfies the source
idea without retaining fallback authority.

Actions:

- Review idea-716 implementation from its activation checkpoint through
  `HEAD` against the source idea and this resumed runbook.
- Reject callee-name, source-order, nearest-plan, route, target, or named-fixture
  authority and any weakening of ambiguity rejection.
- Run the supervisor-selected affected focused boundary proof required for
  acceptance, keeping independent idea-708 failures out of this scope.

Completion check:

- Independent review reports no blocking alignment, overfit, or proof finding,
  and the idea-716-owned focused contract is green without expectation changes.

### Step 3: Run broader acceptance and hand back to idea 708

Goal: close idea 716 with regression evidence and resume its blocked consumer.

Actions:

- Run the supervisor-selected matching broader backend before/after guard.
- Confirm no new failures and no loss of covered passes.
- Request lifecycle closure only after focused review and regression acceptance.

Completion check:

- Broader acceptance is green, reviewer reject signals are absent, and idea
  716 can close before resuming idea 708.
