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

### Step 2: Produce one complete positive authority row

Goal: compose the selected prepared facts into one uniquely identity-bound
`Available` authority row.

Actions:

- Populate destination/source identity and complete homes from prepared-owned
  facts.
- Bind exactly one selected move and freshness result to the same cursor/edge
  and publication identity.
- For an aggregate path, require complete slot, object, and layout evidence;
  for a branch path, require complete stack-load and publication evidence.
- Return explicit negative status for missing, invalid, ambiguous, incomplete,
  mismatched, or route-only evidence.
- Add focused producer proof for the positive row and representative negative
  states, including a nearby same-feature case that rejects overfit.

Completion check:

- A real producer returns one complete, uniquely bound `Available` row; all
  non-authoritative inputs fail closed, and focused producer proof is green.

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
