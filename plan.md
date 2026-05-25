# Shared Prepared Call-Argument Source Selection Runbook

Status: Active
Source Idea: ideas/open/01_shared_prepared_call_argument_source_selection.md
Supersedes: parked active runbook from ideas/open/02_aarch64_calls_emission_consolidation.md

## Purpose

Add shared call-plan authority for selected `BeforeCall` argument source
choices so target-local call emission can consume prepared facts instead of
rederiving caller-side planning decisions.

## Goal

Create and prove a shared prepared call-argument source-selection fact covering
prior preservation, local-frame address materialization, frame-slot/address
sources, and byval register-lane materialization for selected call-argument
moves.

## Core Rule

Keep source selection in the shared prepared call-plan layer and machine-node
emission in the target-local layer. Do not use this prerequisite runbook to
consolidate AArch64 call files or move target-specific emission details into
shared planning.

## Read First

- `ideas/open/01_shared_prepared_call_argument_source_selection.md`
- `ideas/open/02_aarch64_calls_emission_consolidation.md`
- Current shared prepared call-plan data structures and lookup helpers
- AArch64 caller-side `BeforeCall` lowering around `lower_before_call_move`
- Existing prepared preservation, address-materialization, frame-slot, and
  byval-related facts

## Current Targets

- Shared prepared call-plan representation for argument source choices
- Population logic for selected `BeforeCall` argument moves
- Lookup or attachment API for target emitters
- AArch64 caller-side emission consumption of the new fact

## Non-Goals

- Do not retire or merge AArch64 calls translation units in this runbook.
- Do not create target-specific source-selection rules in the shared planner.
- Do not broaden into unrelated ABI, dispatch, ALU, memory, comparison,
  variadic, or prologue cleanup.
- Do not weaken or delete tests to make the new fact appear complete.

## Working Model

- Shared prepared call planning owns the decision that a selected call argument
  should be sourced from prior preservation, prepared address materialization,
  frame-slot/address materialization, or byval register lanes.
- Target-local calls code owns lowering the already-selected source into
  target machine nodes.
- The proof of readiness for the parked AArch64 consolidation idea is that
  AArch64 consumes the shared selection for its formerly local source-choice
  cases.

## Execution Rules

- Start with a mapping step that names the existing local choices and the
  shared facts needed to populate them.
- Keep representation changes narrow and target-independent.
- After each code-changing step, run a fresh build plus focused call tests.
- Escalate validation if the shared prepared call-plan API or broad call
  lowering behavior changes across targets.
- Treat testcase-shaped shortcuts, expectation downgrades, and named-case-only
  fixes as route failures.

## Steps

### Step 1: Source-Choice Contract Mapping

Goal: define the exact shared source-selection contract before editing code.

Primary target: existing prepared call-plan facts and AArch64
`lower_before_call_move` source-choice sites.

Actions:

- List each selected `BeforeCall` argument source choice still made by AArch64.
- Map each choice to the existing prepared facts needed to decide it.
- Name the new prepared source-selection representation and its minimum fields.
- Identify the lookup key or attachment point target emitters will use.

Completion check:

- `todo.md` records the source-choice contract, required fields, and proof
  target for the first implementation slice.

### Step 2: Add Shared Prepared Source-Selection Fact

Goal: introduce the target-independent representation and populate it during
shared call planning.

Primary target: shared prepared call-plan data structures and construction
logic.

Actions:

- Add the prepared source-selection type or field for selected `BeforeCall`
  argument moves.
- Populate prior-preservation, address-materialization, frame-slot/address,
  and byval register-lane source choices from existing shared inputs.
- Keep target-specific machine-node details out of the shared representation.
- Add focused internal assertions or tests where the repo already has an
  appropriate prepared-plan test surface.

Completion check:

- The shared call plan exposes selected source choices without requiring
  target-local BIR rescans or local source-choice reconstruction.
- A fresh build and focused call-plan tests pass.

### Step 3: Consume The Shared Selection In AArch64

Goal: prove the shared fact replaces AArch64-local caller-side source
selection.

Primary target: AArch64 caller-side `BeforeCall` argument lowering.

Actions:

- Replace the local source-choice decision path with reads from the prepared
  source-selection fact.
- Keep AArch64 code responsible only for materializing the selected source into
  AArch64 machine nodes.
- Remove now-obsolete local source-choice helper branches only when equivalent
  shared facts drive the behavior.

Completion check:

- AArch64 no longer owns the selected source choice for the covered
  `BeforeCall` argument move cases.
- A fresh build and focused AArch64 call tests pass.

### Step 4: Prerequisite Acceptance Checkpoint

Goal: decide whether the parked AArch64 consolidation idea can be reactivated.

Primary target: shared prepared source-selection proof and AArch64 consumption.

Actions:

- Compare the implemented fact against the blocker recorded in
  `ideas/open/02_aarch64_calls_emission_consolidation.md`.
- Confirm prior preservation, local-frame address materialization,
  frame-slot/address sources, and byval register-lane materialization are
  either represented or explicitly documented as unsupported by existing
  semantics.
- Run the supervisor-selected broader validation checkpoint before closure or
  switching back to AArch64 consolidation.
- Record remaining blockers in `todo.md` instead of closing prematurely.

Completion check:

- The source idea can close only if the shared fact exists, AArch64 consumes it
  for the required source-choice cases, and close-time regression proof is
  available.
- Otherwise, `todo.md` names the remaining blocker and the plan stays active or
  is retired by lifecycle decision.
