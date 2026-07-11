# Prepared MIR Stack View Contract Runbook

Status: Active
Source Idea: ideas/open/700_prepared_mir_stack_view_contract.md

## Purpose

Create the first prepared-owned stack authority contract that MIR can consume
without rediscovering authority from route-numbered proof records.

## Goal

Publish one explicit prepared authority record family and a matching MIR
prepared view, with positive prepared proof where feasible and fail-closed
negative proof for missing, ambiguous, invalid, unsupported, stale, or
route-only authority.

## Core Rule

MIR may consume only prepared records or named prepared MIR views as authority.
Route 4, Route 5, Route 7, `RouteIndexReferenceFacade`, route dumps,
expectations, and allowlists are compatibility or historical proof only.

## Read First

- `ideas/open/700_prepared_mir_stack_view_contract.md`
- `docs/bir_route_index_retirement/research_digest.md`
- `docs/bir_route_index_retirement/ownership_dependencies.md`
- `docs/bir_route_index_retirement/ordered_followup_plan.md`

## Current Targets

- `src/backend/prealloc/` prepared producer records for the selected first
  family.
- Prepared MIR view surfaces that expose the selected prepared records to MIR.
- Focused prepared contract, prepared MIR, object, object-runtime, or runtime
  tests for one positive shape and one negative rejection shape.

## Non-Goals

- Do not reactivate ideas 647 or 655 before positive prepared producer evidence
  exists.
- Do not infer MIR destinations, value homes, freshness, move bundles, frame
  layout, stack objects, or branch stack-load authority from route agreement,
  facade status, dump rows, expectations, or allowlists.
- Do not perform route facade contraction, BIR dump vocabulary cleanup, or
  broad test policy cleanup in this runbook.
- Do not target-materialize a stack path before the selected prepared authority
  record is published and visible through the MIR prepared view.

## Working Model

- BIR semantic views may feed prepared producers through named view boundaries.
- Prepared/prealloc owns executable authority records for frame layout, value
  homes, move bundles, move resolution, freshness, aggregate stack sources,
  branch stack-loads, and explicit destination authority.
- MIR consumes prepared rows and must fail closed when a required row is
  missing, ambiguous, invalid, unsupported, stale, or route-only.
- Any executable behavior change needs proof above route dumps: prepared
  contract, prepared MIR, object, object-runtime, or runtime proof.

## Execution Rules

- Select exactly one first prepared authority family before implementing
  producer or MIR consumer changes.
- Preserve route-numbered APIs only as private compatibility or diagnostic
  inputs through named BIR/prepared views.
- Keep positive proof and negative fail-closed proof together for the selected
  family.
- Leave parked residual stack ideas parked unless this runbook produces
  positive prepared evidence that satisfies their prerequisite threshold.
- Treat expectation rewrites, unsupported-marker changes, allowlist changes,
  timeout changes, diagnostic wording, and route-dump-only proof as non-progress
  for executable authority.

## Ordered Steps

### Step 1: Select The First Prepared Authority Family

Goal: choose one concrete prepared stack authority family for the first packet.

Primary target: `src/backend/prealloc/` authority records and existing prepared
contacts such as `PreparedStackLayout`, `PreparedFramePlanFunction`,
`PreparedValueHome`, `PreparedMoveBundle`, `PreparedMoveResolution`,
`PreparedValueFreshnessAuthority`, `PreparedAggregateStackSourceAuthority`,
`PreparedBranchStackLoadAuthority`, or prepared MIR direct-edge source views.

Actions:

- Inspect current prepared/prealloc records and MIR consumption sites for stack,
  frame, value-home, move, freshness, aggregate stack-source, branch
  stack-load, and explicit destination authority.
- Identify which existing prepared record family has the smallest positive
  producer seam and a precise missing-authority rejection seam.
- Record in `todo.md` which family was selected, which files own it, which
  route-numbered evidence is compatibility-only, and which proof command will
  be used by the executor.

Completion check:

- One first family is selected with named positive and negative proof surfaces,
  or the packet reports a precise blocker showing no legal first family is
  currently available.

### Step 2: Publish Or Tighten The Prepared Authority Record

Goal: make the selected authority family explicit in prepared/prealloc state.

Primary target: selected files under `src/backend/prealloc/`.

Actions:

- Add or tighten status-rich prepared records for the selected family.
- Preserve `Available` versus fail-closed status distinctions for missing,
  ambiguous, invalid, unsupported, stale, and route-only authority.
- Keep route-numbered builders private and pass their facts through named
  prepared or BIR view boundaries only when they are inputs, not authority.

Completion check:

- The selected family has explicit prepared records and negative statuses
  without changing unrelated route facade, dump, or test policy behavior.

### Step 3: Expose The MIR Prepared View

Goal: give MIR a prepared-owned view for the selected authority family.

Primary target: prepared MIR view files and the narrow MIR consumer surface for
the selected family.

Actions:

- Add or tighten a prepared MIR view that reads only the selected prepared
  records.
- Make MIR fail closed when the selected authority row is missing, ambiguous,
  invalid, unsupported, stale, or route-only.
- Avoid direct MIR inference from Route 4, Route 5, Route 7, facade status,
  dump rows, expected output, or final assembly.

Completion check:

- MIR consumption for the selected family is routed through the prepared view
  and preserves fail-closed behavior for unavailable authority.

### Step 4: Prove Positive And Negative Behavior

Goal: prove the selected prepared authority family at the strongest practical
surface.

Primary target: focused prepared contract, prepared MIR, object,
object-runtime, or runtime tests.

Actions:

- Add or update one focused positive proof for the selected family when a legal
  positive shape exists.
- Add or update one focused negative rejection proof for missing, ambiguous,
  invalid, unsupported, stale, or route-only authority.
- Run build proof first, then the narrow test subset chosen by the supervisor.
- Escalate to broader backend or full validation when the selected family
  affects shared prepared/MIR behavior.

Completion check:

- Fresh proof demonstrates the selected family above route dumps, and the
  negative path remains fail-closed.

### Step 5: Record Residual Stack Revisit Eligibility

Goal: state whether this prepared/MIR contract is sufficient prerequisite
evidence for ideas 647 or 655.

Primary target: `todo.md`, and only then `plan.md` or the linked source idea if
the supervisor requests a durable lifecycle decision.

Actions:

- Compare the produced prepared authority evidence with the prerequisite
  threshold for destination value identity, destination home, storage kind,
  source value/home, move bundle or move resolution, freshness, aggregate stack
  source, branch stack-load, and MIR fail-closed proof as applicable.
- Record whether ideas 647 or 655 should remain parked or can be revisited by a
  later lifecycle packet.
- Do not reactivate parked residual stack ideas from route-only evidence.

Completion check:

- The runbook has a clear lifecycle handoff: either positive prepared evidence
  exists for a residual-stack revisit, or the residual stack ideas remain
  parked with precise missing prerequisite evidence.
