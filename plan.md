# AArch64 Calls Emission Consolidation Runbook

Status: Active
Source Idea: ideas/open/02_aarch64_calls_emission_consolidation.md

## Purpose

Consolidate the AArch64 `calls*.cpp` family after shared prepared call-plan
authority is available, leaving target-local calls code responsible for
emission rather than planning.

## Goal

Reduce and clarify the AArch64 call emission surface by deleting duplicate
local planning logic only when equivalent prepared call-plan facts already
exist or have been introduced by the shared call-plan authority work.

## Core Rule

The shared prepared call-argument source-selection prerequisite is satisfied
for the covered selected `BeforeCall` source cases. Continue consolidation only
through helper boundaries whose decisions are now represented by prepared
call-plan facts, or through a fresh mapping that proves another duplicate
helper route is already prepared.

## Read First

- `ideas/open/02_aarch64_calls_emission_consolidation.md`
- AArch64 call emission owners, currently expected around `calls.cpp`,
  `calls.hpp`, and remaining `calls_*` helpers
- Shared prepared call-plan facts and any prior `01_shared_call_plan_authority`
  result that describes the owned planning boundary
- Build metadata that lists AArch64 calls translation units

## Current Targets

- Remaining AArch64-local call move, argument-source, and preservation helpers
- Build metadata entries for retired `calls_*` files
- Call printing or effect spelling that belongs in machine-printer or shared
  MIR layers instead of AArch64 emission

## Non-Goals

- Do not invent a new call-plan API inside this plan.
- Do not move AArch64 emission details into the shared planner.
- Do not perform broad dispatch, ALU, memory, comparison, variadic, or prologue
  cleanup.
- Do not change behavior solely to reduce line count.
- Do not weaken or delete tests to make consolidation appear complete.

## Working Model

- Shared prepared call-plan facts own call-boundary planning decisions.
- AArch64-local calls code converts prepared facts into AArch64 machine nodes.
- Consolidation is valid only when deleted local logic is duplicated by shared
  facts or moved to the layer that owns the concern.
- File count and line count reduction must come from removed duplication or
  moved authority, not opaque helpers.

## Execution Rules

- Keep each code slice behavior-preserving unless the source idea explicitly
  requires an ownership move with equivalent proof.
- Prefer small consolidation steps that retire one redundant helper family or
  one obsolete translation unit at a time.
- Update build metadata in the same slice that retires a file.
- After each code-changing step, run a fresh build plus focused call tests.
- Escalate to a broader backend or full validation checkpoint after retiring
  translation units or moving ownership across layers.
- Treat testcase-shaped shortcuts, expectation downgrades, and named-case-only
  fixes as route failures.

## Steps

### Step 1: Reactivation Mapping With Prepared Source Selection

Goal: choose the first valid consolidation route now that prepared
call-argument source selection exists.

Primary target: remaining AArch64 call move/source/preservation helpers and
shared prepared call-plan facts, including
`PreparedCallArgumentSourceSelection`.

Actions:

- Inspect the remaining AArch64 `calls_*` helpers and list which ones still
  rederive source, move, preservation, byval, or call-boundary facts.
- Inspect available shared prepared call-plan facts and identify which local
  helper decisions are already represented.
- Prioritize helper routes made actionable by
  `PreparedCallArgumentSourceSelection` for frame-slot values, frame-slot
  addresses, local-frame address materialization, stack-slot prior
  preservation, and complete byval register-lane selections.
- If no valid route exists, stop execution and report the blocker in `todo.md`
  without attempting local cleanup.

Completion check:

- `todo.md` records the exact helper-to-prepared-fact mapping and names the
  first implementation slice, or records that execution is blocked.

### Step 2: Retire One Proven Duplicate Helper Boundary

Goal: remove one AArch64-local planning boundary whose decisions are already
owned by prepared call-plan facts.

Primary target: the helper family proven actionable by Step 1.

Actions:

- Replace local rederivation with reads from the prepared call-plan facts.
- Keep AArch64-local code limited to emission of machine nodes.
- Delete obsolete helper code once callers no longer need it.
- Update includes and build metadata if a translation unit becomes empty or
  obsolete.

Completion check:

- The chosen helper boundary no longer duplicates prepared call-plan decisions.
- A fresh build and focused call tests pass.

### Step 3: Move Non-Emission Spelling To Its Owner

Goal: remove call printing or effect spelling that is not AArch64 emission from
the AArch64 calls surface.

Primary target: machine-printer or shared MIR layers for non-emission spelling.

Actions:

- Identify spelling logic that belongs outside target-local call emission.
- Move the logic to the owning printer or shared MIR layer without changing
  printed behavior.
- Keep AArch64 calls code consuming the shared representation instead of
  formatting or classifying it locally.

Completion check:

- AArch64 calls code no longer owns the moved spelling concern.
- Focused printer or call tests plus a fresh build pass.

### Step 4: Retire Obsolete Calls Translation Units

Goal: shrink the AArch64 calls file family after duplicate decision logic has
been removed.

Primary target: obsolete `calls_*` translation units and corresponding build
metadata.

Actions:

- Merge remaining emission-only code into the small call emission owner when
  the helper file no longer has a clear boundary.
- Delete retired files and remove them from build metadata.
- Keep optional helper files only when they have an emission-only boundary.

Completion check:

- AArch64 call emission files are fewer and responsibilities are clearer.
- Retired files are absent from build metadata and include graphs.
- A fresh build and focused call tests pass.

### Step 5: Consolidation Acceptance Checkpoint

Goal: decide whether the source idea is complete or whether a narrower
follow-up runbook is needed.

Primary target: the final AArch64 calls emission surface and validation logs.

Actions:

- Compare the remaining AArch64 calls surface against the source idea's
  acceptance criteria.
- Confirm target-local calls code no longer rederives call-plan decisions that
  prepared facts already provide.
- Run the supervisor-selected broader validation checkpoint before closure.
- Record any remaining blockers in `todo.md` instead of closing the idea
  prematurely.

Completion check:

- The source idea can be closed only if the acceptance criteria are satisfied
  and close-time regression guard proof is available.
- Otherwise, `todo.md` names the remaining blocker and the plan stays open or
  is retired by lifecycle decision.
