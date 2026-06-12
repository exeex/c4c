# Phase E3 x86 Compare-Join Stack-Home Handoff Follow-Up

Status: Active
Source Idea: ideas/open/234_phase_e3_x86_compare_join_stack_home_handoff_follow_up.md

## Purpose

Repair the x86 prepared-module consumer path so scalar-control-flow
compare-against-zero compare-join lanes with stack-backed parameter homes keep
following authoritative prepared stack-home state through compare-join entry
and return.

## Goal

Make `backend_x86_handoff_boundary` pass the compare-against-zero
compare-join stack-backed parameter-home case without weakening its assertion
or changing the already-confirmed Route 6 route-debug row behavior.

## Core Rule

Fix compare-join stack-home handoff semantics in the x86 prepared-module
consumer path. Do not absorb this into Route 6 route-debug row spelling,
expected strings, fallback matrices, wrappers, baseline refreshes, or
fixture-specific matching.

## Read First

- `ideas/open/234_phase_e3_x86_compare_join_stack_home_handoff_follow_up.md`
- The Route 6 handoff context from idea 232 and its completed commits when
  needed for regression comparison
- x86 prepared-module consumer and handoff-boundary tests around:
  `backend_x86_handoff_boundary`,
  `backend_x86_route_debug`, and
  `backend_prepared_lookup_helper`
- Implementation surfaces selected by Step 1 around compare-join entry,
  compare-join return, prepared stack-home authority, and x86 prepared-module
  consumption

## Current Targets

- The failing `backend_x86_handoff_boundary` semantic case:
  scalar-control-flow compare-against-zero compare-join lane with
  stack-backed parameter home.
- x86 prepared-module consumer behavior through compare-join entry and return.
- Prepared stack-home authority used by the handoff-boundary proof.
- Preservation of Route 6 route-debug row behavior already confirmed by
  `backend_x86_route_debug` and `backend_prepared_lookup_helper`.
- Focused nearby same-feature coverage for compare-join stack-home handoff.

## Non-Goals

- Do not change Route 6 route-debug row spelling, expected strings, fallback
  behavior, wrappers, prepared call/debug output, or helper-oracle behavior.
- Do not start broad ABI/call wrapper migration, E4 wrapper migration, draft
  155, E5, or prepared diagnostic/oracle retirement.
- Do not downgrade supported assertions, mark the handoff-boundary case
  unsupported, refresh baselines as proof, mask timeouts, or weaken the
  failing assertion.
- Do not add testcase-name, label-text, block-shape, or assertion-string
  shortcuts for the compare-join fixture.

## Working Model

- The broader x86 handoff-boundary proof now builds and fails semantically,
  which separates this work from the completed Route 6 route-debug row.
- The selected failure means the x86 prepared-module consumer stops following
  authoritative prepared stack-home state through compare-join entry and
  return for a scalar compare-against-zero lane.
- The repair should flow through semantic prepared or handoff state that also
  explains nearby same-feature cases, not through fixture-specific matching.
- Route 6 route-debug behavior remains a regression guard, not the target of
  this plan.

## Execution Rules

- Start by reproducing and localizing the `backend_x86_handoff_boundary`
  failure before editing implementation.
- Trace the prepared stack-home authority through compare-join entry, return,
  and the x86 prepared-module consumer path.
- Keep any implementation change scoped to compare-join stack-home handoff
  semantics unless Step 1 proves a smaller or more precise owner.
- Preserve Route 6 route-debug outputs and helper/debug contracts.
- Every code-changing step needs fresh build or compile proof plus the
  supervisor-delegated test subset.
- Ask the supervisor to choose broader validation before acceptance if the
  diff touches shared x86 wrapper, module lowering, prepared dispatch,
  `ConsumedPlans`, MIR query, direct-call, helper-oracle, or prepared
  call/debug surfaces.

## Steps

### Step 1: Locate The Compare-Join Stack-Home Handoff Surface

Goal: reproduce the semantic failure and identify the exact prepared stack-home
handoff owner before implementation edits.

Primary targets:

- `backend_x86_handoff_boundary`
- x86 prepared-module consumer code selected by the failure
- prepared stack-home authority and compare-join entry/return state
- Route 6 regression surfaces:
  `backend_x86_route_debug` and `backend_prepared_lookup_helper`

Actions:

- Reproduce or inspect the failing `backend_x86_handoff_boundary` case and
  record the exact assertion text and observed wrong handoff state.
- Trace the compare-against-zero compare-join lane from prepared stack-home
  authority through compare-join entry and return.
- Identify the minimal implementation target files and nearby same-feature
  tests needed for Step 2.
- Inventory which Route 6 route-debug behaviors must remain unchanged.
- Record the selected owner, suspected semantic gap, proof gaps, and proposed
  Step 2 packet in `todo.md`.

Completion check:

- `todo.md` names the failing case, the precise prepared stack-home handoff
  surface, minimal implementation targets, Route 6 regression guard surfaces,
  and the proof command recommended for Step 2.
- No implementation, test, expected-string, baseline, wrapper, helper-oracle,
  prepared call/debug, or route-debug behavior change is made in this step
  unless the executor is explicitly delegated to begin implementation.

### Step 2: Repair Prepared Stack-Home Handoff Semantics

Goal: make the x86 prepared-module consumer follow authoritative prepared
stack-home state through compare-join entry and return.

Primary targets:

- The implementation owner identified in Step 1
- Compare-join entry and return prepared/handoff state
- The x86 prepared-module consumer path for stack-backed parameter homes

Actions:

- Repair the semantic handoff path so the compare-join lane retains or
  reacquires the authoritative prepared stack home through entry and return.
- Use prepared or handoff state that naturally explains the failing case and
  nearby same-feature cases.
- Keep the fix independent of testcase names, label text, block shape, and
  assertion strings.
- Preserve Route 6 route-debug row spelling, fallback behavior, expected
  strings, wrappers, prepared call/debug output, and helper-oracle behavior.
- Build the touched target and run the supervisor-delegated narrow proof.

Completion check:

- The failing compare-join stack-backed parameter-home case passes without
  weakening the assertion.
- The implementation follows semantic prepared stack-home authority rather
  than fixture-specific matching.
- The delegated narrow proof passes without expectation or baseline rewrites.

### Step 3: Prove Nearby Handoff Stability

Goal: prove the repair is not shaped around one compare-join fixture and does
not disturb Route 6 route-debug behavior.

Primary targets:

- `backend_x86_handoff_boundary`
- `backend_x86_route_debug`
- `backend_prepared_lookup_helper`
- Nearby same-feature compare-join, stack-home, scalar-control-flow, and
  compare-against-zero coverage selected by the supervisor

Actions:

- Add or tighten focused tests only where Step 1 or Step 2 found missing
  coverage for compare-join stack-home handoff semantics.
- Cover the repaired positive path and nearby same-feature paths that should
  retain prepared stack-home authority through compare-join entry and return.
- Prove Route 6 route-debug behavior remains unchanged through the selected
  route-debug and prepared lookup subsets.
- Do not weaken handoff-boundary assertions, rewrite expected strings, refresh
  baselines, or change wrappers/helper-oracle outputs.

Completion check:

- Focused tests or explicit proof cover the fixed stack-home handoff behavior
  and at least one nearby same-feature path.
- `backend_x86_route_debug` and `backend_prepared_lookup_helper` remain green.
- No Route 6 route-debug, wrapper, expected-string, baseline, helper-oracle,
  or prepared call/debug contract change is part of the diff.

### Step 4: Validate And Prepare Acceptance Notes

Goal: prove the completed x86 compare-join stack-home handoff repair and leave
clear handoff state for supervisor review.

Primary targets:

- Build or compile target chosen by the supervisor
- Delegated narrow x86 handoff, route-debug, and prepared lookup test subset
- Any broader validation the supervisor requests because of touched surfaces
- `todo.md`

Actions:

- Run the exact supervisor-delegated proof command and record the command and
  result in `todo.md`.
- Summarize the semantic owner changed, prepared stack-home authority retained,
  Route 6 behavior preserved, same-feature proof, and unchanged surfaces.
- If shared x86 wrapper, module lowering, prepared dispatch, `ConsumedPlans`,
  MIR query, direct-call, helper-oracle, or prepared call/debug surfaces were
  touched, ask the supervisor to choose broader validation before acceptance.

Completion check:

- Fresh build or compile proof exists.
- Delegated narrow tests pass, and any supervisor-requested broader validation
  passes.
- `todo.md` records the implementation summary, proof commands, retained
  prepared stack-home authority, unchanged Route 6/debug/output surfaces, and
  residual risks.
