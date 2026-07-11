# BIR Semantic Handoff Views Runbook

Status: Active
Source Idea: ideas/open/704_bir_semantic_handoff_views.md

## Purpose

Replace public route-numbered BIR handoff APIs with narrow, ownership-named
semantic views while allowing existing route builders to remain private during
migration.

## Goal

Expose stable BIR identities and explicit availability through seven named
semantic views, migrate the first common-MIR producer/source-semantic entry
points, and prove that the public boundary contains no route record or builder
index.

## Core Rule

BIR may report source-semantic relationships only. It must not select frame or
value homes, moves, freshness, ABI placement, destination authority, or any
other prepared decision.

## Read First

- `ideas/open/704_bir_semantic_handoff_views.md`
- `docs/bir_mir_contract_abstraction/02_ownership_and_named_handoff_contracts.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/mir/query.cpp`

## Current Scope

- `BirProducerView`, `BirMemoryAccessView`, `BirPublicationView`,
  `BirCallBoundaryView`, `BirComparisonView`, `BirReturnView`, and
  `BirControlFlowView` contracts.
- BIR-owned production and a public/private header boundary under
  `src/backend/bir/`.
- Producer/source-semantic entry points used by common
  `src/backend/mir/query.cpp`.
- Focused BIR contract/unit proof for positive and negative result states.
- A compile guard against public route vocabulary and route-shaped payloads.

## Non-Goals

- Do not migrate the prepared consumers owned by idea 705.
- Do not produce frame, home, move, freshness, ABI, stack-destination, or
  target-placement authority in BIR.
- Do not delete private `bir_route*.cpp` builders merely to satisfy this idea.
- Do not add downstream callers to private compatibility adapters.
- Do not rename, alias, inherit from, or wrap a complete route record and claim
  it as a named semantic view.
- Do not weaken tests, unsupported expectations, or contracts.

## Working Model

- Each public query is keyed by stable function, block, instruction, value,
  edge, call, or relationship identity as appropriate.
- Each result carries only the requested semantic fact plus an explicit status
  that distinguishes available, unavailable, incomplete, and ambiguous input.
- Route builders and indexes may implement a view behind private BIR seams,
  but neither their types nor their vocabulary cross the public named header.
- Common MIR may temporarily consume these named BIR views for source-semantic
  questions; prepared placement authority remains outside this plan.

## Execution Rules

- Implement one coherent contract family at a time and keep its positive and
  negative proof in the same step.
- Prefer shared status/result vocabulary only when it remains narrow and does
  not recreate a generic route record.
- Keep route-to-view adapters private and behavior-preserving.
- Before accepting each code-changing step, run the delegated build and narrow
  BIR proof selected by the supervisor.
- Run the umbrella route-vocabulary guard after boundary changes; hits in
  private compatibility implementation are allowed, but named public headers
  and named results must contain none.
- Escalate to broader backend validation at the final integration step.

## Ordered Steps

### Step 1: Establish the named public/private contract boundary

Goal: define the narrow public result/status vocabulary and move route-shaped
implementation details behind a private BIR seam.

Primary targets:

- public and private headers under `src/backend/bir/`
- existing `BirProducerView` declarations in `src/backend/bir/bir.hpp`
- focused BIR contract or compile-boundary tests

Actions:

- Inventory the current public producer-view surface and the route types or
  indexes it exposes, directly or through private members and constructors.
- Introduce the ownership-named public header boundary needed by the seven
  views, using stable identities and explicit available, unavailable,
  incomplete, and ambiguous outcomes.
- Refactor `BirProducerView` so its public declaration and result payload do
  not mention, return, inherit, alias, or reproduce a route record or builder
  index.
- Keep any route-backed construction adapter private to BIR implementation.
- Add focused producer-view positive and negative proof plus a compile guard
  that rejects route vocabulary in the named public boundary.

Completion check:

- `BirProducerView` has focused available/unavailable/incomplete/ambiguous
  proof, and the public named header compiles without `RouteN`, `RouteIndex`,
  `route_index`, or a complete route-shaped payload.

### Step 2: Add memory and publication semantic views

Goal: expose narrow memory-access and source-publication relationships without
granting prepared storage or movement authority.

Actions:

- Implement `BirMemoryAccessView` for access identity/kind, address base,
  result or stored value, and permitted source relationships.
- Implement `BirPublicationView` for current-block, block-entry, or CFG-edge
  identity and the published source value relationship.
- Keep frame slots, homes, freshness, moves, and destinations out of both
  contracts.
- Add focused positive proof and unavailable, incomplete, and ambiguous proof
  for each view.

Completion check:

- Both views return narrow status-rich facts, their public declarations pass
  the route-vocabulary guard, and focused BIR tests cover every required result
  state.

### Step 3: Add call and comparison semantic views

Goal: expose call-boundary and comparison relationships without selecting ABI
resources, operand placement, or executable branch transfers.

Actions:

- Implement `BirCallBoundaryView` for call/callee identity, argument source
  relationships, direct-global dependency, and result identity/lane.
- Implement `BirComparisonView` for comparison identity, operands, predicate
  or condition, materialized condition identity, and branch use.
- Keep call-plan/ABI placement and prepared comparison/control authority out of
  these BIR results.
- Add focused positive and all required negative-state proof for both views.

Completion check:

- Call and comparison queries expose only source semantics, pass the public
  route-vocabulary guard, and have focused available/unavailable/incomplete/
  ambiguous proof.

### Step 4: Add return and control-flow semantic views

Goal: finish the named BIR contract family for return provenance and
target-independent select, branch, edge, and join relationships.

Actions:

- Implement `BirReturnView` for return identity, returned value, provenance
  link, and chain completeness where a target-independent consumer exists;
  otherwise retain target-only traversal as private compatibility.
- Implement `BirControlFlowView` for select dependencies, branch condition,
  successor/predecessor edge, join source, and completeness.
- Do not authorize publication placement, branch stack loads, target
  transfers, or prepared destinations through either view.
- Add focused positive and all required negative-state proof for both views.

Completion check:

- The seven-view public family is complete, return/control queries are
  target-independent and narrow, and focused tests cover the required status
  states without route-shaped fixtures being the sole proof.

### Step 5: Migrate the first common-MIR semantic entry points

Goal: make the producer/source-semantic entry points used by common
`src/backend/mir/query.cpp` consume the named BIR boundary.

Primary target:

- `src/backend/mir/query.cpp`

Actions:

- Identify only the common-MIR queries whose answer is a BIR-owned producer or
  source-semantic relationship.
- Replace their direct public route declarations and queries with the named
  BIR views.
- Fail closed on unavailable, incomplete, and ambiguous results.
- Do not migrate prepared consumers assigned to idea 705 or add direct callers
  to private route adapters.
- Add or update focused common-MIR query proof without expectation-only
  renames.

Completion check:

- The first common-MIR semantic entry points use named views, negative results
  fail closed, and no migrated consumer includes or returns a public route
  record or index.

### Step 6: Audit the boundary and run integration proof

Goal: prove the implementation satisfies the source idea without expanding
route compatibility or prepared authority.

Actions:

- Run the umbrella route-vocabulary guard and classify remaining hits as
  private compatibility, deferred consumers, debug/proof, or violations.
- Verify no named public header or result mentions or returns `RouteN`,
  `RouteIndex`, `route_index`, or a full route record.
- Verify private adapters gained no new downstream callers.
- Run the full focused BIR contract set, common-MIR query tests, and the
  supervisor-selected broader backend regression check.
- Record remaining consumer migration as idea 705 scope rather than absorbing
  it into this runbook.

Completion check:

- All seven contracts have focused positive and negative producer proof, the
  public compile guard is green, direct public route declarations have shrunk,
  broader validation is green, and no prepared or target authority moved into
  BIR.
