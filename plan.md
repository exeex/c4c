# LIR Remaining Authority Owner Triage Runbook

Status: Active
Source Idea: ideas/open/866_lir_remaining_authority_owner_triage.md
Switched From: ideas/closed/865_lir_next_non_body_parameter_authority_handoff.md
after no-change conclusion; parent 734 remains paused after receiver commit
`750b6b3ba`

## Purpose

Classify the remaining post-Step-7.51 current-LIR authority space before any
new producer or Raw-BIR receiver implementation route is selected.

## Goal

Produce a current evidence bundle, owner classification, and ordered follow-up
ideas for the next executable first-owner routes that can eventually return to
734.

## Core Rule

Do not implement fixes, edit tests, or reopen accepted receiver rows. This
umbrella only classifies first ownership and creates ordered follow-up ideas.

## Read First

- `ideas/open/866_lir_remaining_authority_owner_triage.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/865_lir_next_non_body_parameter_authority_handoff.md`
- Accepted receiver commits `750b6b3ba` and `0c44e810ad`
- Relevant stale open ideas only when their scope overlaps the current
  post-Step-7.51 734 blocker

## Current Targets And Scope

- Create or refresh docs under `docs/lir_remaining_authority_owner_triage/`.
- Record why 865 produced no handoff and why `LirAbsOp`
  selected-global/i32 is not selectable.
- Classify remaining families by first owning layer and dependency order.
- Generate or repair ordered follow-up ideas under `ideas/open/`.
- Keep 734 paused until a later follow-up produces an exact accepted handoff.

## Non-Goals

- Do not edit implementation files, tests, expectations, unsupported markers,
  allowlists, runtime behavior, Raw-BIR receiver code, importer code, or
  verifier behavior.
- Do not publish LIR producer/schema/verifier authority inside this umbrella.
- Do not select a one-row implementation packet directly from this runbook.
- Do not treat stale open idea return records as current 734 authority without
  reconciliation.
- Do not reopen `LirAbsOp` selected-global/i32, direct-call argument 0/1,
  body-parameter, local-object, VLA, accepted call-result, or other accepted
  receiver rows.

## Execution Rules

- Preserve evidence at the documentation layer first.
- Keep every follow-up idea single-owner and single-layer unless the source
  explicitly requires a bounded handoff/return pairing.
- Every generated follow-up must include concrete reject signals and an exact
  734 return condition when applicable.
- Use `git diff --check` as the minimum proof for lifecycle/docs-only slices.

## Steps

### Step 1 - Build the current evidence bundle

Goal: create the triage documentation directory and record the authoritative
post-Step-7.51 evidence.

Actions:

- Create `docs/lir_remaining_authority_owner_triage/`.
- Write a current-evidence document naming accepted commits `750b6b3ba` and
  `0c44e810ad`, the rejected 865 row, and the accepted/stale rows that must
  not be reopened.
- List the broad remaining families from the 865 blocker without assigning
  implementation yet.

Completion check:

- The evidence document states that 865 produced no handoff.
- The evidence document states that `LirAbsOp` selected-global/i32 is already
  received by `0c44e810ad`.
- No implementation or test file is edited.

### Step 2 - Classify remaining families by first owner

Goal: decide which first owning layer must move before each remaining family
can return to 734.

Actions:

- Classify CFG/PHI residuals, memory/VA, aggregate/vector,
  module/type/global/metadata, residual instruction/terminator, inline-asm,
  and stale overlapping open ideas.
- For each family, identify whether the first owner is LIR producer/schema,
  LIR verifier, Raw-BIR receiver, importer, documentation, or broader policy.
- Mark families that are not currently executable and explain the blocker.

Completion check:

- The classification document separates first owners and does not mix producer
  repair with Raw-BIR receipt.
- Stale open ideas are either reconciled or rejected as current successors.

### Step 3 - Generate ordered follow-up ideas and close the umbrella

Goal: create the next executable source ideas and record their order.

Actions:

- Create or repair follow-up ideas under `ideas/open/` for the selected ordered
  first-owner routes.
- Ensure each follow-up has goal, scope, non-goals, acceptance criteria, exact
  734 return condition where relevant, and reviewer reject signals.
- Update the umbrella source with a closure note naming docs, generated ideas,
  ordering, and deferred work.
- Send the completed route to plan-owner for close.

Completion check:

- Follow-up ideas exist or are explicitly rejected with evidence.
- The umbrella closure note names the current evidence and ordered successors.
- `git diff --check` passes.
