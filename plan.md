# BIR Cutover And Legacy Cleanup Runbook

Status: Active
Source Idea: ideas/open/06_bir_cutover_and_legacy_cleanup.md

## Purpose

Complete the backend migration by making BIR the default path, deleting
legacy-only backend plumbing, and leaving one clear backend test story.

## Goal

Ship a BIR-default backend flow with only a short-lived escape hatch if needed,
while materially reducing legacy routing, compatibility code, and legacy-only
tests.

## Core Rule

Do not widen this effort into unrelated backend cleanup. Keep each slice narrow,
test-backed, and directly tied to the cutover from the legacy backend path to
the BIR-first path.

## Read First

- [ideas/open/06_bir_cutover_and_legacy_cleanup.md](/workspaces/c4c/ideas/open/06_bir_cutover_and_legacy_cleanup.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)

## Current Scope

- flip the default backend routing to BIR
- retain only a short-lived fallback switch if required for safety
- remove backend-side legacy lowering and compatibility plumbing that only
  exists for the old path
- clean up transitional `backend_ir` naming where practical
- make backend tests primarily validate the BIR-oriented layered structure

## Non-Goals

- inventing another backend IR abstraction after BIR
- keeping two equal-status backend paths
- broad stylistic cleanup outside the cutover surfaces
- deleting useful safety coverage before BIR-default behavior is proven

## Working Model

Treat this as a consolidation pass, not a greenfield rewrite:

1. prove BIR can be the default path
2. keep an emergency fallback only if active failures justify it
3. remove dead or near-dead legacy code once the default path is covered
4. rename surviving concepts so BIR is the authoritative backend IR term
5. leave tests aligned with the surviving architecture

## Execution Rules

- Start from the smallest backend-routing slice that can be validated with
  backend-focused tests.
- Prefer deleting compatibility layers only after the BIR-default path is
  covered by targeted tests.
- If a legacy helper is still required by an active fallback, mark it as
  temporary and keep the remaining surface minimal.
- When renaming APIs or files, update tests and docs in the same slice only when
  they directly describe the changed backend path.
- If execution uncovers adjacent but separate work, record it under
  `ideas/open/` instead of silently expanding this runbook.

## Step 1. Establish the Cutover Surface

Goal: identify the exact backend routing entrypoints, flags, and tests that
control BIR-vs-legacy behavior today.

Primary targets:

- backend lowering entrypoints and route selection code
- fallback or feature-flag plumbing controlling old/new path choice
- backend-focused tests that currently exercise both paths

Actions:

- inspect the backend entrypoints and route-selection call graph
- identify the current default path and any escape-hatch knobs
- map the narrowest test set that proves default-path behavior
- record temporary keep/remove decisions in `plan_todo.md`

Completion check:

- one concrete cutover slice is identified
- the current fallback mechanism is understood well enough to preserve or remove
  it intentionally
- the validating backend test subset for the first slice is named

## Step 2. Flip the Default to BIR

Goal: make BIR the authoritative backend route by default.

Primary targets:

- default backend routing logic
- user-facing or developer-facing flags that select backend paths
- tests that validate the default route

Actions:

- add or update narrow tests that assert BIR-default behavior
- change routing so the default backend flow uses BIR
- retain only the minimum fallback knob needed for short-term safety
- remove developer ambiguity about which backend path is authoritative

Completion check:

- default execution goes through BIR
- targeted backend tests cover the new default
- any retained fallback is explicitly temporary and minimal

## Step 3. Remove Legacy Backend Plumbing

Goal: reduce or delete backend-side code that only served the old route.

Primary targets:

- legacy adapter-first lowering entrypoints
- backend parsing helpers used only by the old path
- compatibility wrappers between LIR and old backend IR layers
- transitional debug and print utilities tied only to legacy flow

Actions:

- delete dead helpers after proving they are not on the BIR-default path
- collapse compatibility shims that no longer serve the surviving design
- keep changes in small slices so regressions are attributable

Completion check:

- legacy-only backend plumbing is materially reduced
- any code kept for fallback is clearly limited and justified

## Step 4. Clean Up Surviving Names

Goal: make the surviving backend pipeline read as HIR -> LIR -> BIR -> target
emission.

Primary targets:

- transitional `backend_ir` names
- public or semi-public lowering entrypoints whose names still describe the old
  model
- comments or docs that describe the old backend IR as authoritative

Actions:

- rename surviving APIs and types to BIR-oriented names where practical
- retire or reduce legacy-named compatibility headers
- update directly affected comments and tests to match the surviving vocabulary

Completion check:

- the surviving backend path is described in BIR terms
- legacy transitional naming is removed or confined to explicit compatibility
  shims

## Step 5. Finish the Backend Test Migration

Goal: make the BIR-oriented layered test structure the primary backend
validation path for cutover.

Primary targets:

- `tests/c/internal/backend_*`
- `tests/backend/*`
- backend-regex filtered regression runs used during cutover

Actions:

- keep or add tests that validate BIR-default routing, lowering, validation,
  and target emission boundaries
- remove obsolete tests that only protect deleted transitional plumbing
- preserve only legacy-path tests that still justify a short fallback window
- run targeted backend tests and then full-suite regression validation

Completion check:

- backend tests primarily validate the BIR-based path
- obsolete transitional-path coverage is reduced
- full-suite validation is monotonic after the cutover slices land
