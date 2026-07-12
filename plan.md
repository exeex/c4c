# Prepared Stack Destination Authority Positive Gate Runbook

Status: Active
Source Idea: ideas/open/707_prepared_stack_destination_authority_positive_gate.md
Activated after completion of: ideas/closed/705_prepared_fact_boundary_from_bir_views.md, ideas/closed/706_common_mir_named_query_migration.md

## Purpose

Establish one real positive prepared stack-destination authority path so the
parked fan-in work can resume from producer-owned facts instead of route
agreement, dumps, source order, or MIR reconstruction.

## Goal

Produce a uniquely bound `PreparedStackDestinationAuthorityView` row with
complete destination/source, home, move, freshness, cursor/edge, publication,
and required stack evidence, then prove that prepared MIR consumes only an
`Available` row and fails closed for every negative state.

## Core Rule

Prealloc/prepared production owns the authority. MIR may consume the completed
view but must not rediscover destination fan-in, manufacture missing evidence,
or select authority through route vocabulary.

## Read First

- `ideas/open/707_prepared_stack_destination_authority_positive_gate.md`
- `ideas/closed/705_prepared_fact_boundary_from_bir_views.md`
- `ideas/closed/706_common_mir_named_query_migration.md`
- `docs/bir_mir_contract_abstraction/02_ownership_and_named_handoff_contracts.md`
- `docs/bir_mir_contract_abstraction/04_handoff_audit_and_closure_evidence.md`

## Current Scope

- Prealloc/prepared stack-destination authority production.
- One cursor-bound prepared MIR feature view consuming that authority.
- Complete positive proof plus missing, invalid, ambiguous, incomplete, and
  route-only rejection proof.
- Aggregate and branch cases only where required to establish complete
  slot/object/layout/load evidence.

## Non-Goals

- Do not resume or implement ideas 647 or 655 inside this runbook.
- Do not migrate x86, AArch64, or RV64 materializers owned by ideas 708-710.
- Do not infer authority in common or target MIR.
- Do not accept dumps, source order, final assembly, or route agreement as the
  positive producer proof.
- Do not weaken supported expectations or add testcase-shaped selection.

## Working Model

- A positive row binds one destination/source relationship to complete homes,
  one selected move, one selected freshness result, cursor/edge identity,
  publication evidence, and all applicable stack evidence.
- Aggregate cases require complete slot, object, and layout identity.
- Branch stack-load cases require complete load and publication identity.
- Any absent, invalid, ambiguous, incomplete, mismatched, or route-only input
  yields an explicit non-available result.
- The MIR feature view accepts only `Available` and forwards producer-owned
  identity without recomputation.

## Execution Rules

- Work in bounded producer-first packets; do not begin MIR consumption until
  the positive producer contract is complete and focused proof is green.
- Preserve stable function, block, edge, cursor, value, home, move,
  publication, slot, object, layout, and load identities as applicable.
- Add positive and fail-closed proof with each authority family.
- Run the supervisor-delegated build and focused tests for every code step.
- Require a broader backend regression guard before closure because the view
  composes shared prepared frame, home, move, freshness, and publication data.

## Ordered Steps

### Step 1: Inventory the authority schema and select the first positive producer

Goal: map the existing prepared facts and negative gate to every required row
field, then select one real producer path that can become positively available
without route-derived evidence.

Actions:

- Locate the `PreparedStackDestinationAuthorityView` declaration, producer,
  current consumers, and registered contracts.
- Inventory the exact source of destination/source values, complete homes,
  selected move, selected freshness, cursor/edge, publication, and applicable
  slot/object/layout/load evidence.
- Classify every missing field or negative status by its owning prepared
  producer; reject any proposed route, dump, source-order, or MIR-derived
  substitute.
- Select the smallest real aggregate or branch path whose complete upstream
  facts can produce the first `Available` row, and identify nearby negative
  cases that prove the rule is semantic rather than fixture-shaped.
- Record the exact focused build and test targets for the producer and feature
  view before implementation begins.

Completion check:

- Every required field has one prepared owner, the first positive producer is
  bounded to a real semantic path with nearby negative coverage, and no field
  depends on route vocabulary or MIR reconstruction.

### Step 2.1: Establish one real stack-backed publication/move producer

Goal: establish or select a prepared-owned relationship that has an actual
edge publication, a bound move resolution, complete homes, and a stack-backed
destination before authority-row composition begins.

Primary target:

- The smallest semantic out-of-SSA parallel-copy/select producer family that
  can carry a real predecessor/successor edge, `PreparedEdgePublication`, and
  bound `PreparedMoveResolution` into a stack-backed destination.

Actions:

- Start from a real publication/move producer, not the rejected collected
  `Lhs` branch-stack-load fixture, which has no publication or move binding.
- Establish one relationship whose destination/source values, destination and
  source homes, selected move, execution cursor, exact predecessor/successor
  edge, and publication identity all come from their prepared owners.
- Require the destination to be genuinely stack-backed by its prepared home
  and frame evidence; do not turn a register-backed select fixture into proof
  through constants, equality, or load-policy classification.
- Bind nearby branch stack-load and aggregate stack-source evidence to the
  same stable value/home/slot/object/layout identities. Preserve explicit
  applicability when one evidence family does not apply to a given record;
  never manufacture either family inside the authority composer.
- Add focused upstream contracts that distinguish the positive producer from
  a nearby relationship lacking a stack destination, publication, move, or
  matching branch/aggregate evidence.

Completion check:

- A focused green contract proves one real stack-backed publication/move
  relationship with exact edge identity and independently owned applicable
  branch/aggregate evidence. No `PreparedStackDestinationAuthorityView` row is
  composed yet.

### Step 2.2: Define the complete composer input and reachable negative states

Goal: expose a bounded prepared-side composition seam whose independently
owned inputs can represent both the complete producer and every advertised
fail-closed family.

Actions:

- Define the composer input/query around the Step 2.1 publication, bound move,
  homes, freshness, cursor/edge, and stable publication identity.
- Carry and cross-check applicable frame slot, stack object, layout geometry,
  aggregate stack-source, and branch stack-load identities; arithmetic
  plausibility alone is not identity proof.
- Represent missing, invalid, ambiguous, incomplete, mismatched, and
  route-only evidence as independently constructible semantic inputs so each
  status is reachable. Remove nominal statuses that cannot occur at this
  layer instead of testing only their spelling.
- Preserve upstream failure reasons rather than collapsing every non-available
  producer into one generic branch-authority status.
- Add focused composer-input contracts for representative negative states and
  a nearby same-feature non-match that rejects fixture-shaped selection.

Completion check:

- Every retained status is reachable through a real composer input, stable
  identities are explicit across owners, and the focused input/negative
  contract is green without relying on a passing prefix of a red test.

### Step 2.3: Compose and prove one complete positive authority row

Goal: compose the established prepared facts into one uniquely identity-bound
`Available` authority row.

Actions:

- Populate destination/source identities and complete homes only from the
  Step 2.1 producer relationship; equality is allowed only when those owners
  explicitly prove it.
- Bind exactly one real selected move and freshness result to the same cursor,
  exact edge, and stable publication identity.
- Require and copy all applicable slot/object/layout, aggregate stack-source,
  and branch stack-load evidence from the Step 2.2 input seam.
- Return the explicit Step 2.2 negative status for non-authoritative evidence;
  do not derive availability from route agreement, source order, use-kind,
  both possible branch labels, or MIR reconstruction.
- Add focused positive-row proof alongside the reachable negative contracts,
  using a genuinely green executable or isolated subset.

Completion check:

- A real producer returns one complete, uniquely bound `Available` row; all
  retained non-authoritative states fail closed, and the focused producer
  contract is green. Only then may execution advance to Step 3.

### Step 3: Consume the row through the prepared MIR feature view

Goal: make the cursor-bound MIR view accept only the producer-owned positive
authority without rediscovering fan-in.

Actions:

- Adapt the bounded prepared MIR feature view to copy the complete row only
  when status is `Available`.
- Preserve every producer-owned identity and reject all negative statuses,
  identity mismatches, and incomplete applicable stack evidence.
- Remove any route agreement, source-order, dump, or local reconstruction used
  as executable authority in this bounded path.
- Add focused positive consumption and fail-closed negative contracts.

Completion check:

- MIR consumes only the complete `Available` row, performs no authority
  reconstruction, and focused positive and negative contracts are green.

### Step 4: Prove the positive gate and audit resume readiness

Goal: establish closure-quality evidence for idea 707 without absorbing the
parked fan-in initiatives.

Actions:

- Audit the positive row and MIR consumer for route vocabulary, hidden route
  selection, source-order inference, and testcase-shaped shortcuts.
- Prove the positive producer plus missing, invalid, ambiguous, incomplete,
  mismatched, and route-only rejection behavior.
- Prove applicable aggregate slot/object/layout and branch stack-load evidence
  is complete.
- Run the focused prepared contracts, affected runtime/object proof, and the
  supervisor-selected broader backend before/after regression guard.
- Record whether ideas 647 and 655 now have every positive row field and
  fail-closed prerequisite required to resume; do not activate them here.

Completion check:

- At least one real positive row and its MIR consumption are green, all
  negative states fail closed, route vocabulary does not select authority,
  broader proof has no regression, and resume readiness for ideas 647/655 is
  explicit.
