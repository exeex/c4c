# BIR MIR Contract Abstraction Umbrella Runbook

Status: Active
Source Idea: ideas/open/703_bir_mir_contract_abstraction_umbrella.md

## Purpose

Classify the remaining route-numbered BIR-to-MIR contract leaks and turn the
evidence into named handoff contracts plus ordered, single-owner follow-up
ideas.

## Goal

Create a durable contract-abstraction handoff under
`docs/bir_mir_contract_abstraction/` and generate the evidence-backed follow-up
idea queue needed to remove route-numbered APIs from cross-layer contracts.

## Core Rule

This umbrella performs evidence classification and planning only. Do not
implement backend changes or disguise the existing route records behind new
names.

## Read First

- `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
- `ideas/closed/683_prepared_mir_view_contract_research.md`
- `ideas/closed/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md`
- `ideas/closed/693_bir_route_index_retirement_research.md`
- `ideas/closed/694_bir_route_index_retirement_umbrella.md`
- `docs/prepared_mir_view_contract_research/`
- `docs/bir_route_index_retirement_research/`
- `docs/bir_route_index_retirement/`

## Current Scope

- Route-numbered dependencies in BIR, prealloc, MIR, target MIR backends, and
  backend tests.
- Named BIR semantic views, prepared facts, and MIR-facing handoff contracts.
- Stack, frame, value-home, move-bundle, branch stack-load, and destination
  authority ownership.
- Durable handoff documents and ordered follow-up ideas under `ideas/open/`.

## Non-Goals

- Do not change implementation or tests inside this umbrella.
- Do not reactivate ideas 647 or 655 without positive prepared/prealloc stack
  authority producer evidence.
- Do not combine BIR view production, prepared fact production, MIR migration,
  target materialization, and test cleanup in one follow-up.
- Do not use route dumps, freshness, source order, final assembly, or testcase
  identity as semantic authority.
- Do not weaken expectations, unsupported markers, allowlists, runtime
  behavior, harness contracts, or baseline policy.

## Working Model

- BIR owns named semantic view production and may retain route-numbered
  builders privately during migration.
- Prealloc owns conversion of semantic views into explicit prepared facts.
- MIR consumes named handoff or prepared objects and must not rediscover BIR
  route analysis.
- Target materializers consume common MIR-facing contracts after those
  contracts exist.
- Debug and test vocabulary follows semantic consumer migration; it does not
  establish semantic authority.

## Execution Rules

- Base the inventory on a fresh source scan and record the exact scan command.
- Classify every dependency by first owning layer and distinguish semantic
  inputs from debug, proof, and compatibility artifacts.
- Keep every generated follow-up idea single-owner, ordered by dependency, and
  explicit about its first consumer and proof surface.
- Give every generated idea concrete reviewer reject signals and a
  route-vocabulary retirement guard.
- Write umbrella progress to `todo.md`; change this runbook only if the route
  contract or ordering materially changes.

## Ordered Steps

### Step 1: Refresh the dependency inventory

Goal: establish the current route-numbered dependency baseline across all
layers named by the source idea.

Primary targets:

- `src/backend/bir/`
- `src/backend/prealloc/`
- `src/backend/mir/` and target MIR backends
- backend tests and route-related debug/proof surfaces

Actions:

- Run a fresh route-vocabulary scan covering `route[1-8]_`, `Route[1-8]`,
  `bir_route[1-8]`, `RouteIndex`, and `route_index`.
- Reconcile current hits with the prepared-MIR-view and route-retirement docs.
- Record each dependency, consumer, current producer, and whether it is
  semantic, debug/proof, or compatibility-only.
- Create the handoff directory and write the current dependency inventory.

Completion check:

- `docs/bir_mir_contract_abstraction/` contains a reproducible, current
  inventory covering every required layer, with stale evidence identified.

### Step 2: Define ownership and named handoff contracts

Goal: specify where each semantic fact is produced and what narrow form may
cross into prealloc or MIR.

Actions:

- Classify dependencies by BIR view producer, prepared fact producer, MIR
  consumer, target materializer, debug/proof artifact, or compatibility bridge.
- Propose named views for producer, memory, publication, call, comparison,
  return, control-flow, stack, frame, value-home, move-bundle, branch
  stack-load, and destination-authority facts where evidence requires them.
- Define public/private header boundaries and prevent wrappers from exposing
  full route records under renamed types.
- State the explicit prepared/prealloc evidence required before ideas 647 and
  655 can resume.

Completion check:

- The handoff docs contain an ownership classification and named contract
  proposal that distinguish semantic authority from debug and compatibility.

### Step 3: Generate the ordered follow-up idea queue

Goal: convert the classified contracts into small, dependency-ordered source
ideas with one first owning layer each.

Actions:

- Generate the required BIR semantic handoff, MIR query migration, prepared
  fact boundary, target MIR cleanup, stack-authority gate, route quarantine,
  and test-vocabulary cleanup idea families unless evidence justifies a better
  split.
- Give each idea an owning layer, first migrated consumer, proof surface,
  dependencies, retirement guard, acceptance criteria, and concrete reviewer
  reject signals.
- Order ideas by contract ownership rather than route number and record the
  sequence in the handoff docs.

Completion check:

- Ordered follow-up ideas exist under `ideas/open/`, agree with the handoff
  documents, and do not mix producer, consumer, target, or test ownership.

### Step 4: Audit the handoff and prepare lifecycle closure

Goal: verify the umbrella has a complete, traceable handoff without performing
implementation work.

Actions:

- Re-run the route-vocabulary guard and document how each dependency is
  expected to shrink through the ordered ideas.
- Verify docs agree on evidence, ownership, contract names, ordering, and the
  stack-authority resume gate.
- Record which route APIs may remain private compatibility and which follow-up
  owns every deferred dependency.
- Populate the source idea's required closure-note evidence in `todo.md` for
  plan-owner review; do not close solely because the runbook steps are done.

Completion check:

- The source acceptance criteria are evidenced, every remaining dependency has
  an owner or explicit deferral, and the supervisor can request formal close
  review and regression-guard handling.
