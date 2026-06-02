# AArch64 Memory Owner Subresponsibility Audit Runbook

Status: Active
Source Idea: ideas/open/86_aarch64_memory_owner_subresponsibility_audit.md

## Purpose

Audit the post-contraction AArch64 memory owner and decide which memory
subresponsibilities are justified target-local lowering versus candidates for
narrow follow-up implementation ideas.

## Goal

Produce a function-level responsibility inventory for
`src/backend/mir/aarch64/codegen/memory.cpp`, then create follow-up source
ideas only where the owner boundary and proof route are concrete.

## Core Rule

This plan is audit-only. Do not edit implementation files while executing this
runbook; create or update lifecycle idea files only when the audit proves a
separate narrow initiative is needed.

## Read First

- `ideas/open/86_aarch64_memory_owner_subresponsibility_audit.md`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `ideas/closed/70_aarch64_memory_prepared_address_authority_cleanup.md`
- `ideas/closed/80_aarch64_dispatch_publication_owner_relocation.md`
- `ideas/closed/81_aarch64_dispatch_edge_copy_owner_contraction.md`
- `ideas/closed/83_aarch64_local_helper_duplication_tail_cleanup.md`
- `ideas/closed/84_aarch64_prepared_consumer_wrapper_contraction.md`

## Current Targets

- Primary audit target:
  `src/backend/mir/aarch64/codegen/memory.cpp`
- Header context:
  `src/backend/mir/aarch64/codegen/memory.hpp`
- Historical comparison only:
  closed ideas 70, 80, 81, 83, and 84

## Non-Goals

- Do not make direct implementation edits in `memory.cpp` or `memory.hpp`.
- Do not move AArch64 addressing legality, scratch selection, register
  spelling, or machine-record construction into shared BIR/prealloc authority.
- Do not reopen dispatch publication or edge-copy relocation decisions from
  ideas 80 and 81.
- Do not treat line-count reduction as success unless the resulting owner
  boundary preserves memory lowering ownership.
- Do not create a monolithic "shrink memory.cpp" implementation route.

## Working Model

Classify each memory owner function or helper cluster into one or more of:

- `target-local-memory-emission`
- `frame-slot-addressing`
- `stack-source-publication`
- `store-retargeting`
- `identity-validation`
- `prepared-record-construction`
- `diagnostics-and-error-spelling`
- `candidate-local-subowner`
- `needs-shared-authority-evidence`

Large clusters may remain in `memory.cpp` when they are target-local memory
lowering, ABI-sensitive, or coupled to AArch64 addressing legality.

## Execution Rules

- Keep audit notes in `todo.md` until they are durable enough to become source
  idea content or a new follow-up idea.
- Prefer function-level evidence over line-count arguments.
- When a cluster looks movable, name the owner boundary, the files it would
  own, and the proof set before creating a follow-up idea.
- Reject any route that says "move to BIR" without identifying the missing
  target-neutral fact.
- If implementation files are accidentally touched, stop and return the dirty
  state to the supervisor; audit-only proof no longer applies.

## Steps

### Step 1: Build The Memory Function Inventory

Goal: Produce a complete function-level map of `memory.cpp` and the public
surface from `memory.hpp`.

Primary target:
`src/backend/mir/aarch64/codegen/memory.cpp`

Actions:

- List top-level functions, local helpers, exported functions, and important
  local structs or enums in `memory.cpp`.
- Cross-check declarations and public entry points in `memory.hpp`.
- Record each item with a one-line responsibility summary.
- Mark obvious references to dispatch publication, edge-copy, prepared
  address, local-helper, and prepared-wrapper history for later comparison.

Completion check:

- `todo.md` contains an inventory covering every relevant function/helper in
  `memory.cpp` and each public memory surface in `memory.hpp`.

### Step 2: Classify Responsibility Clusters

Goal: Group the inventory into the source idea's responsibility categories.

Actions:

- Assign each function/helper to the working-model categories.
- Identify clusters that must stay in `memory.cpp` because they own
  target-local memory emission, AArch64 addressing legality, scratch choice,
  register spelling, diagnostics, or machine-record construction.
- Identify clusters that appear separable only when they have a stable local
  owner boundary.
- Mark unclear clusters as `needs-shared-authority-evidence` instead of
  turning uncertainty into implementation scope.

Completion check:

- `todo.md` names the clusters, their category assignments, and the evidence
  for stay-in-memory versus candidate-local-subowner treatment.

### Step 3: Compare Against Closed-Idea Boundaries

Goal: Ensure the audit does not reopen recently closed owner relocations or
wrapper contraction decisions.

Actions:

- Compare candidate clusters against closure intent from ideas 70, 80, 81, 83,
  and 84.
- Reject candidates that merely undo dispatch publication relocation,
  edge-copy owner contraction, or prepared-wrapper contraction.
- Preserve any durable rationale that explains why growth in `memory.cpp` is
  justified owner-local memory lowering.

Completion check:

- `todo.md` records which candidates were rejected by prior closed-idea
  boundaries and which remain viable after historical comparison.

### Step 4: Create Narrow Follow-Up Ideas Where Justified

Goal: Convert only concrete, bounded audit findings into new source ideas.

Actions:

- For each viable candidate, create a focused `ideas/open/*.md` implementation
  idea only if it has a stable owner boundary and proof route.
- Each new idea must include goal, why, scope, non-goals, acceptance or proof
  expectations, and concrete reviewer reject signals.
- Do not create a follow-up idea for clusters that should stay intentionally
  target-local or that only have vague shared-authority speculation.

Completion check:

- New follow-up ideas exist only for concrete implementation candidates, and
  `todo.md` names every created idea.

### Step 5: Prepare Audit Close Summary

Goal: Make the audit outcome clear enough for the supervisor to close or route
the lifecycle state.

Actions:

- Summarize which memory clusters remain intentionally target-local.
- Summarize which follow-up ideas were created and why.
- Summarize rejected candidates and the evidence that blocked them.
- State that no backend tests were required if no implementation files changed.

Completion check:

- `todo.md` contains a close-ready audit summary that satisfies the source
  idea's proof expectations.
