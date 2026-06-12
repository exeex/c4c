# Phase E3 Prepared Compare-Join Selected-Value-Chain Metadata Follow-Up

Status: Active
Source Idea: ideas/open/236_phase_e3_prepared_compare_join_selected_value_chain_metadata_follow_up.md

## Purpose

Repair prepared compare-join selected-value-chain metadata so pointer-backed
same-module global return-context ownership keeps publishing the true
global-root selected-value chain.

## Goal

Make the pointer-backed selected-value-chain handoff-boundary assertion
advance through semantic prepared compare-join metadata while preserving the
idea 234 stack-home and idea 235 Route 6 repairs as regression guards.

## Core Rule

Repair selected-value-chain metadata publication. Do not claim progress through
testcase-shaped matching, assertion weakening, baseline refreshes, helper
renames, or unrelated Route 6/x86 stack-home changes.

## Read First

- `ideas/open/236_phase_e3_prepared_compare_join_selected_value_chain_metadata_follow_up.md`
- Parked predecessor context:
  - `ab5b854db isolate x86 compare-join stack-home handoff repair`
  - `ea2b3a133 publish Route 6 scalar i32 call argument sources`
- Prepared compare-join metadata surfaces around
  `src/backend/prealloc/control_flow.hpp`, selected-value-chain ownership,
  return-context publication, helper-oracle expectations, and
  `backend_x86_handoff_boundary`.

## Current Targets

- Pointer-backed same-module global return-context ownership selected-value
  chain publication.
- The shared helper path that should preserve or publish the true global-root
  selected-value chain through compare-join return context ownership.
- Focused helper and handoff-boundary proof for the failing assertion and
  nearby same-feature metadata cases.
- Regression protection for idea 234 x86 stack-home behavior and idea 235
  Route 6 consumed scalar i32 call-argument source behavior.

## Non-Goals

- Do not change x86 compare-join stack-home handoff behavior from idea 234.
- Do not change Route 6 consumed scalar i32 call-argument source behavior from
  idea 235.
- Do not start broad pointer lowering rewrites, ABI/call wrapper migration,
  prepared diagnostic/oracle retirement, E4, E5, or draft 155 work.
- Do not rewrite route-debug/helper expected strings, baselines, wrappers, or
  prepared diagnostic output unless the supervisor explicitly approves that
  scope after a review.
- Do not add testcase-name, label-text, block-shape, or assertion-string
  shortcuts for the pointer-backed global fixture.

## Working Model

- Idea 234 repaired the x86 compare-join stack-home handoff and exposed a
  Route 6 consumed scalar i32 call-argument source failure.
- Idea 235 repaired Route 6 publication in
  `src/backend/prealloc/call_plans.cpp`; the delegated proof now advances to
  the selected-value-chain failure.
- The active failure is:

```text
scalar-control-flow compare-against-zero prepared compare-join pointer-backed same-module global selected-value chain return context ownership:
shared helper stopped publishing the true global-root selected-value chain
```

- The likely owner is prepared compare-join selected-value-chain metadata,
  especially the shared helper path around `src/backend/prealloc/control_flow.hpp`.

## Execution Rules

- Start by reproducing or inspecting the selected-value-chain failure before
  implementation edits.
- Identify the semantic metadata owner and the exact selected-value-chain facts
  that should survive through compare-join return-context ownership.
- Preserve fail-closed behavior when the root chain cannot be proven.
- Treat any need to alter Route 6 consumed-plan behavior or x86 stack-home
  compare-join behavior as a stop signal for this idea.
- Every code-changing step needs fresh build or compile proof plus the
  supervisor-delegated test subset.
- Ask the supervisor to choose broader validation before acceptance if the diff
  touches shared prepared control-flow, helper-oracle, MIR query, direct-call,
  wrapper, or selected-value-chain surfaces beyond the focused owner.

## Steps

### Step 1: Locate The Selected-Value-Chain Metadata Gap

Goal: reproduce the pointer-backed selected-value-chain failure and identify
the minimal prepared compare-join metadata owner before implementation edits.

Primary targets:

- `backend_x86_handoff_boundary`
- `src/backend/prealloc/control_flow.hpp`
- selected-value-chain root publication paths
- compare-join return-context ownership helpers
- helper-oracle or route-debug guard surfaces selected by the supervisor

Actions:

- Reproduce or inspect the failing `backend_x86_handoff_boundary` assertion:
  `shared helper stopped publishing the true global-root selected-value chain`.
- Trace how the true global-root selected-value chain should be represented in
  prepared compare-join metadata.
- Identify the smallest owner that drops, replaces, or fails to publish that
  chain through return-context ownership.
- Inventory fail-closed cases and nearby same-feature metadata cases that must
  remain unchanged.
- Record the selected owner, proposed Step 2 implementation surface, proof
  command, and regression guard surfaces in `todo.md`.

Completion check:

- `todo.md` names the failing case, metadata owner, expected selected-value
  chain facts, fail-closed cases, nearby guards, and recommended proof command.
- No implementation, expected-string, baseline, wrapper, helper-oracle,
  Route 6, or x86 stack-home behavior change is made in this step unless the
  executor is explicitly delegated to begin implementation.

### Step 2: Repair Global-Root Chain Publication

Goal: preserve or publish the true global-root selected-value chain through
prepared compare-join return-context ownership.

Primary targets:

- The implementation owner identified in Step 1
- `src/backend/prealloc/control_flow.hpp`
- selected-value-chain metadata construction or merge helpers
- return-context ownership publication paths

Actions:

- Repair the selected-value-chain metadata path so pointer-backed same-module
  global return contexts keep the true global-root chain.
- Derive the repaired facts from semantic prepared compare-join metadata, not
  from fixture names, labels, block shapes, or assertion strings.
- Preserve fail-closed behavior when selected-value-chain roots are missing,
  contradictory, or outside the supported pointer-backed same-module global
  path.
- Preserve idea 234 stack-home behavior and idea 235 Route 6 behavior.
- Build the touched target and run the supervisor-delegated narrow proof.

Completion check:

- The selected-value-chain assertion advances without weakening or removing it.
- The implementation repairs metadata publication rather than matching the
  failing fixture.
- Route 6 and x86 stack-home guard subsets remain unchanged under the
  supervisor-selected proof.

### Step 3: Prove Nearby Metadata Stability

Goal: prove the repair does not broaden selected-value-chain publication into
best-effort guesses or unrelated ownership paths.

Primary targets:

- Focused helper coverage for selected-value-chain root ownership
- `backend_x86_handoff_boundary`
- Route 6 and stack-home guard tests selected by the supervisor

Actions:

- Add or tighten focused coverage for missing roots, contradictory roots,
  non-global pointer-backed paths, and nearby same-feature metadata cases where
  existing coverage is insufficient.
- Confirm helper-oracle, route-debug, wrapper, baseline, and prepared
  diagnostic output contracts remain unchanged unless explicitly approved.
- Do not absorb Route 6, x86 stack-home, ABI, wrapper, or broad pointer
  lowering work into this validation step.

Completion check:

- Focused positive and fail-closed coverage exists for the repaired
  selected-value-chain behavior.
- The selected-value-chain handoff-boundary path and selected nearby guards
  pass without expected-string or baseline rewrites.
- No unrelated Route 6, stack-home, wrapper, baseline, or helper-oracle change
  is part of the diff.

### Step 4: Validate And Prepare Acceptance Notes

Goal: prove the completed selected-value-chain metadata repair and leave clear
handoff state for supervisor review.

Primary targets:

- Build or compile target chosen by the supervisor
- Delegated selected-value-chain, Route 6, and stack-home guard subset
- Any broader validation requested by the supervisor
- `todo.md`

Actions:

- Run the exact supervisor-delegated proof command and record the command and
  result in `todo.md`.
- Summarize the semantic owner changed, selected-value-chain facts preserved,
  fail-closed cases retained, and unchanged Route 6/stack-home/debug/helper
  surfaces.
- If shared prepared control-flow, helper-oracle, MIR query, direct-call,
  wrapper, or selected-value-chain surfaces were touched, ask the supervisor to
  choose broader validation before acceptance.

Completion check:

- Fresh build or compile proof exists.
- Delegated narrow tests pass, and any supervisor-requested broader validation
  passes.
- `todo.md` records the implementation summary, proof commands, preserved
  fail-closed behavior, unchanged guard surfaces, and residual risks.
