# Phase E3 Route 6 Consumed Scalar I32 Call-Argument Source Follow-Up

Status: Active
Source Idea: ideas/open/235_phase_e3_route6_consumed_scalar_i32_call_argument_source_follow_up.md

## Purpose

Repair Route 6 consumed-plan lookup behavior so named scalar i32 call-argument
sources are available through `ConsumedPlans` when prepared call-plan authority
already records the source relationship.

## Goal

Make the Route 6 scalar i32 call-use boundary pass through semantic prepared
call-plan facts while preserving fail-closed behavior and existing route-debug
output contracts.

## Core Rule

Derive Route 6 named scalar i32 call-argument source facts from prepared
call-plan authority. Do not claim progress through testcase-shaped matching,
expected-string rewrites, baseline refreshes, helper renames, or broad
ABI/call-wrapper migration.

## Read First

- `ideas/open/235_phase_e3_route6_consumed_scalar_i32_call_argument_source_follow_up.md`
- The committed idea 234 isolation context:
  `ab5b854db isolate x86 compare-join stack-home handoff repair`
- Route 6 and prepared lookup surfaces around:
  `find_consumed_scalar_i32_call_argument_source(...)`,
  consumed call-argument plan lookup, prepared call-plan authority,
  `backend_prepared_lookup_helper`, `backend_x86_route_debug`, and
  `backend_x86_handoff_boundary`

## Current Targets

- Route 6 scalar i32 call-argument source facts exposed through
  `ConsumedPlans`.
- Prepared call-plan authority for named scalar i32 arguments when it supplies
  source encoding, source value id, and source name without guessing.
- Fail-closed behavior for absent Route 6 facts, mismatched prepared ids,
  missing source names, and non-Route 6 or non-i32 cases.
- Preservation of x86 compare-join stack-home behavior from idea 234 and the
  existing route-debug/helper output contracts.

## Non-Goals

- Do not change x86 compare-join stack-home handoff behavior from idea 234.
- Do not change prepared compare-join selected-value-chain metadata from idea
  236.
- Do not rewrite Route 6 route-debug expected strings, baselines, helper
  fallback output, wrappers, or prepared diagnostic/oracle contracts.
- Do not start broad ABI/call wrapper migration, prepared diagnostic/oracle
  retirement, E4, E5, or draft 155 work.
- Do not add direct extern call testcase-name, label-text, block-shape, or
  assertion-string shortcuts.

## Working Model

- Idea 234 advanced `backend_x86_handoff_boundary` past the compare-join
  stack-home assertion and exposed the Route 6 consumed scalar i32
  call-argument source failure.
- The dirty prior attempt suggested the missing relationship may be available
  from prepared call-plan authority, but that implementation must be rederived
  in this idea and kept fail-closed.
- `backend_prepared_lookup_helper` and `backend_x86_route_debug` are
  regression guards for Route 6 lookup/debug behavior, not permission to
  rewrite expected output.

## Execution Rules

- Start by reproducing or inspecting the Route 6 consumed scalar i32 failure
  before implementation edits.
- Identify the narrow consumed-plan lookup owner and the exact prepared
  call-plan facts that are safe to reuse.
- Preserve fail-closed behavior whenever Route 6 facts are absent, prepared
  ids disagree, source names are missing, or the argument is outside the named
  scalar i32 path.
- Treat any need to change x86 compare-join stack-home behavior or
  pointer-backed selected-value-chain metadata as a stop signal for this idea.
- Every code-changing step needs fresh build or compile proof plus the
  supervisor-delegated test subset.
- Ask the supervisor to choose broader validation before acceptance if the diff
  touches shared `ConsumedPlans`, prepared call/debug, direct-call, MIR query,
  helper-oracle, or wrapper surfaces.

## Steps

### Step 1: Locate The Route 6 Consumed-Plan Gap

Goal: reproduce the Route 6 scalar i32 call-use boundary failure and identify
the minimal consumed-plan lookup owner before implementation edits.

Primary targets:

- `backend_x86_handoff_boundary`
- `find_consumed_scalar_i32_call_argument_source(...)`
- consumed call-argument plan lookup paths
- prepared call-plan authority for named scalar i32 arguments
- `backend_prepared_lookup_helper` and `backend_x86_route_debug`

Actions:

- Reproduce or inspect the failing `backend_x86_handoff_boundary` assertion:
  `x86 Route 6 call-use boundary: scalar call argument source did not thread through ConsumedPlans`.
- Trace how Route 6 named scalar i32 call-argument source facts should reach
  `ConsumedPlans`.
- Identify which prepared call-plan facts are authoritative enough to backfill
  source encoding, source value id, and source name without guessing.
- Inventory fail-closed cases that must remain rejected.
- Record the selected owner, proposed Step 2 implementation surface, proof
  command, and regression guard surfaces in `todo.md`.

Completion check:

- `todo.md` names the failing case, consumed-plan lookup owner, safe prepared
  call-plan facts, required fail-closed cases, and recommended proof command.
- No implementation, expected-string, baseline, wrapper, helper-oracle,
  prepared call/debug, x86 stack-home, or selected-value-chain behavior change
  is made in this step unless the executor is explicitly delegated to begin
  implementation.

### Step 2: Repair Named Scalar I32 Source Lookup

Goal: expose Route 6 named scalar i32 call-argument source facts through
`ConsumedPlans` from semantic prepared call-plan authority.

Primary targets:

- The implementation owner identified in Step 1
- `find_consumed_scalar_i32_call_argument_source(...)`
- related consumed call-argument plan lookup paths
- prepared call-plan authority for named scalar i32 arguments

Actions:

- Repair the lookup path so named scalar i32 call arguments can recover source
  encoding, source value id, and source name from prepared call-plan authority
  when Route 6 facts are present and consistent.
- Keep the repair independent of direct extern call testcase names, labels,
  assertion text, and expected output strings.
- Preserve fail-closed behavior for absent Route 6 facts, prepared-id
  mismatches, missing source names, unsupported encodings, non-i32 arguments,
  and non-Route 6 paths.
- Preserve x86 compare-join stack-home behavior from idea 234 and do not touch
  selected-value-chain metadata from idea 236.
- Build the touched target and run the supervisor-delegated narrow proof.

Completion check:

- The Route 6 consumed scalar i32 call-use boundary advances without weakening
  assertions or rewriting expected output.
- The implementation derives facts from semantic prepared call-plan authority
  rather than fixture-specific matching.
- `backend_prepared_lookup_helper` and `backend_x86_route_debug` remain green.

### Step 3: Prove Fail-Closed And Nearby Stability

Goal: prove the repair does not turn missing or mismatched Route 6 facts into
best-effort guesses.

Primary targets:

- `backend_prepared_lookup_helper`
- Route 6 consumed-plan lookup coverage
- `backend_x86_route_debug`
- `backend_x86_handoff_boundary`

Actions:

- Add or tighten focused fail-closed coverage for absent facts, prepared-id
  mismatches, missing source names, unsupported encodings, non-i32 arguments,
  and non-Route 6 paths where existing coverage is insufficient.
- Prove the positive named scalar i32 path and the fail-closed paths selected
  by Step 1 or Step 2.
- Confirm route-debug row spelling, helper fallback behavior, expected
  strings, wrappers, and prepared call/debug output remain unchanged.
- Do not absorb x86 compare-join stack-home or selected-value-chain metadata
  work into this validation step.

Completion check:

- Focused positive and fail-closed coverage exists for the repaired lookup
  behavior.
- Route-debug and prepared lookup guard subsets remain green without expected
  rewrites.
- No unrelated x86 stack-home, selected-value-chain, wrapper, baseline, or
  helper-oracle change is part of the diff.

### Step 4: Validate And Prepare Acceptance Notes

Goal: prove the completed Route 6 consumed scalar i32 call-argument source
repair and leave clear handoff state for supervisor review.

Primary targets:

- Build or compile target chosen by the supervisor
- Delegated Route 6, prepared lookup, and handoff-boundary test subset
- Any broader validation the supervisor requests because of touched surfaces
- `todo.md`

Actions:

- Run the exact supervisor-delegated proof command and record the command and
  result in `todo.md`.
- Summarize the semantic owner changed, prepared call-plan authority used,
  fail-closed cases preserved, route-debug/helper output contracts unchanged,
  and any residual risks.
- If shared `ConsumedPlans`, prepared call/debug, direct-call, MIR query,
  helper-oracle, or wrapper surfaces were touched, ask the supervisor to choose
  broader validation before acceptance.

Completion check:

- Fresh build or compile proof exists.
- Delegated narrow tests pass, and any supervisor-requested broader validation
  passes.
- `todo.md` records the implementation summary, proof commands, preserved
  fail-closed behavior, unchanged debug/output surfaces, and residual risks.
