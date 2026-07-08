# Plan: Prepared Value Architecture Follow-Up Umbrella

Status: Active
Source Idea: ideas/open/595_prepared_value_architecture_followup_umbrella.md

## Purpose

Classify the remaining prepared-value architecture work after the
587/588/589/590 freshness chain and the 592/593/594 branch-stack queue, then
produce a durable handoff and ordered follow-up ideas for work that is not
already covered.

Goal: produce the `docs/prepared_value_architecture_followup_umbrella/`
handoff and any necessary new `ideas/open/` follow-up ideas without changing
implementation, tests, runtime behavior, expectations, unsupported markers, or
allowlists.

Core Rule: this is an umbrella triage route, not a repair route; classify
evidence by first owning layer before creating follow-up work.

## Read First

- `ideas/open/595_prepared_value_architecture_followup_umbrella.md`
- `ideas/open/591_prepared_mir_view_contract_research.md`
- Closure evidence named by the source idea:
  - `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
  - `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
  - `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
  - `ideas/closed/590_branch_stack_load_freshness_contract.md`
  - the closed or latest lifecycle records for ideas 592, 593, and 594

Read `ideas/closed/` only as historical evidence for the paths named by the
source idea. Only `ideas/open/` is the candidate-work inventory.

## Current Targets And Scope

- Create or refresh:
  - `docs/prepared_value_architecture_followup_umbrella/index.md`
  - `docs/prepared_value_architecture_followup_umbrella/01_six_point_reassessment.md`
  - `docs/prepared_value_architecture_followup_umbrella/02_current_open_queue_mapping.md`
  - `docs/prepared_value_architecture_followup_umbrella/03_followup_idea_backlog.md`
  - `docs/prepared_value_architecture_followup_umbrella/04_dependency_and_priority_order.md`
- Generate new `ideas/open/*.md` follow-up ideas only for remaining work that
  is ready for durable source intent.
- Preserve `ideas/open/591_prepared_mir_view_contract_research.md` unless the
  evidence shows a specific source-intent repair is required.

## Non-Goals

- Do not implement source, backend, test, diagnostic, harness, or runtime
  changes.
- Do not rewrite expectations, unsupported markers, allowlists, or capability
  contracts.
- Do not duplicate existing open work from idea 591.
- Do not merge producer-publication repair, target consumer migration,
  Prepared MIR view design, pointer/address semantic modeling, and diagnostic
  policy into one broad follow-up idea.
- Do not claim capability progress through documentation-only classification.

## Working Model

The umbrella must re-evaluate the six prepared-value architecture directions
against fresh lifecycle evidence:

1. prepared publication model completeness
2. move-bundle authority design
3. value-home, preservation, and rematerialization priority
4. pointer/address arithmetic and local-memory boundaries
5. call-boundary and post-call value publication
6. diagnostic narrowing versus real capability closure

Each remaining problem needs a first owning layer:

- shared-prealloc producer publication
- shared-prealloc consumer authority
- target RV64 consumption
- target-independent MIR view contract
- pointer/address semantic model
- call-boundary freshness
- diagnostics or reviewer policy

## Execution Rules

- Keep all durable classification in the handoff docs.
- Create new follow-up ideas only when the handoff docs identify a clear owner,
  prerequisite set, acceptance criteria, and reviewer reject signals.
- Any generated idea under `ideas/open/` must include concrete reviewer reject
  signals for testcase-shaped shortcuts, expectation downgrades, broad mixed
  ownership, and retaining the same failure mode behind a new abstraction name.
- If evidence is insufficient for a follow-up idea, mark the item deferred in
  the handoff instead of inventing a vague implementation route.
- If idea 591 needs amendment, prefer documenting the exact amendment need in
  the handoff and route the source-idea edit through plan-owner lifecycle work.
- Update `todo.md` with packet progress and proof; do not rewrite this runbook
  for routine packet completion.

## Steps

### Step 1: Confirm Evidence Inputs And Handoff Scope

Goal: establish the evidence set that supersedes stale queue references.

Actions:

- Inspect the source idea and current `ideas/open/` inventory.
- Read the named closure evidence for ideas 587, 588, 589, 590, 592, 593, and
  594.
- Confirm that ideas 592, 593, and 594 are closed or otherwise no longer the
  active execution queue.
- Record the exact evidence list that will be cited by the handoff docs.

Completion Check:

- The execution notes in `todo.md` identify the evidence paths used and any
  missing or stale references that the handoff must handle.

### Step 2: Build The Handoff Document Set

Goal: create the required documentation structure without making architecture
claims before the evidence is classified.

Actions:

- Create `docs/prepared_value_architecture_followup_umbrella/` if needed.
- Create or refresh the required `index.md` and four numbered handoff files.
- Keep each file focused on its assigned purpose from the source idea.

Completion Check:

- The handoff directory contains exactly the required umbrella docs unless the
  packet explicitly documents an additional supporting file in `todo.md`.

### Step 3: Reassess The Six Improvement Directions

Goal: classify each original direction against fresh closure and open-queue
evidence.

Actions:

- Write `01_six_point_reassessment.md`.
- For each direction, state current evidence, current status, first owning
  layer, and recommended disposition.
- Distinguish already addressed work, active or closed branch-freshness queue
  work, 591-owned Prepared MIR view research, new implementation candidates,
  new research candidates, and intentional deferrals.

Completion Check:

- All six directions have explicit status, owner, and disposition entries.

### Step 4: Map Existing Open Coverage

Goal: prevent duplicate follow-up ideas.

Actions:

- Write `02_current_open_queue_mapping.md`.
- Map what idea 591 covers and deliberately excludes.
- Account for ideas 592, 593, and 594 using their current closed or latest
  lifecycle evidence rather than stale open-queue assumptions.
- State whether 591 should remain unchanged, wait, or receive a specific
  amendment request.

Completion Check:

- Existing open coverage and closed queue residue are separated from genuinely
  new follow-up work.

### Step 5: Create The Follow-Up Backlog And Source Ideas

Goal: turn only ready remaining work into ordered durable source ideas.

Actions:

- Write `03_followup_idea_backlog.md`.
- For each required follow-up family, generate a new `ideas/open/*.md` file or
  explicitly decline/defer it with evidence.
- Ensure each generated idea has an owning layer, prerequisites, in-scope and
  out-of-scope boundaries, acceptance criteria, and reviewer reject signals.
- Do not create a broad mixed-owner umbrella as a substitute for narrow source
  ideas.

Completion Check:

- Every required follow-up family is either represented by an existing idea,
  a new generated idea, or an explicit defer/decline entry with a reason.

### Step 6: Order Dependencies And Recommend The Next Activation

Goal: make the backlog executable without route drift.

Actions:

- Write `04_dependency_and_priority_order.md`.
- Order follow-up ideas by first owning layer and dependency pressure.
- Name prerequisites that must close before each follow-up can activate.
- State which follow-up should run immediately after this umbrella closes.

Completion Check:

- The priority document gives a concrete next activation recommendation and
  records which items must wait for 591 or other prerequisites.

### Step 7: Final Consistency Review

Goal: prepare the umbrella for lifecycle closure review.

Actions:

- Update `index.md` so it links all four numbered docs and summarizes the
  final classifications and generated ideas.
- Check that no implementation, test expectation, unsupported-marker,
  allowlist, runtime, or default harness files changed.
- Check that generated ideas do not duplicate 591 and include reviewer reject
  signals.
- Record proof commands and changed files in `todo.md`.

Completion Check:

- The handoff docs and generated ideas satisfy the source acceptance criteria
  and are ready for a plan-owner close decision.
