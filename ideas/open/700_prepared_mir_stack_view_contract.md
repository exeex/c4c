# Prepared MIR Stack View Contract

Status: Open
Type: Implementation
Parent: `ideas/open/694_bir_route_index_retirement_umbrella.md`
Handoff:
- `docs/bir_route_index_retirement/research_digest.md`
- `docs/bir_route_index_retirement/ownership_dependencies.md`
- `docs/bir_route_index_retirement/ordered_followup_plan.md`
Queue Order: 6

## Goal

Define prepared-owned stack, frame, value-home, move, freshness, aggregate
stack-source, branch stack-load, and destination-authority views that MIR can
consume without rediscovering authority from route records.

## Why This Exists

The handoff states that BIR semantic views may feed prepared producers, but
MIR must consume explicit prepared records and fail closed on missing,
ambiguous, invalid, unsupported, or route-only authority. This idea creates the
prepared/MIR contract needed before parked stack destination ideas 647 or 655
can resume.

## Owned Files

- `src/backend/prealloc/` prepared producer records for stack/frame/value-home,
  move bundles, freshness, aggregate stack-source, branch stack-load, and
  explicit destination authority.
- Prepared MIR view files that expose these records to MIR.
- Focused prepared contract, prepared MIR, object, object-runtime, or runtime
  tests for the first producer family.

## First Owning Layer

Prepared/prealloc producer records and MIR prepared views.

## First Producer Migration

Publish one explicit prepared stack authority view over existing contacts such
as `PreparedStackLayout`, `PreparedFramePlanFunction`, `PreparedValueHome`,
`PreparedMoveBundle`, `PreparedMoveResolution`,
`PreparedValueFreshnessAuthority`,
`PreparedAggregateStackSourceAuthority`,
`PreparedBranchStackLoadAuthority`, or prepared MIR direct-edge source views.

## Proof Surface

Prepared contract, prepared MIR, object, object-runtime, or runtime proof for
the first prepared authority family. MIR must fail closed when required rows
are missing, ambiguous, invalid, unsupported, stale, or route-only.

## Numbered Route APIs Kept Private Compatibility

- Route 4 publication rows
- Route 5 CFG-edge and join-source agreement rows
- Route 7 comparison validation rows
- `RouteIndexReferenceFacade`
- Any route dump row used only as historical agreement evidence

These may feed prepared producers only through named BIR views. They are not
direct MIR-side destination, stack, value-home, freshness, frame-layout,
move-bundle, branch stack-load, or publication authority.

## In Scope

- Define one explicit prepared authority record family and MIR prepared view.
- Preserve fail-closed statuses for missing or route-only authority.
- Prove one positive producer shape and one negative rejection shape where
  feasible.
- Record whether the produced seam is sufficient to revisit ideas 647 or 655.

## Out Of Scope

- Reactivating ideas 647 or 655 without positive prepared producer evidence.
- Inferring MIR destinations from route agreement, dump rows, expectations, or
  allowlists.
- BIR facade contraction or dump vocabulary cleanup.
- Target materialization before prepared authority exists.

## Acceptance Criteria

- A named prepared authority family owns the first stack/MIR contract surface.
- MIR consumes only available prepared rows and fails closed otherwise.
- Route-numbered evidence remains historical or compatibility input, not
  executable authority.
- Proof exists above route dumps for any executable behavior change.

## Reviewer Reject Signals

- Reject MIR rediscovery of stack destination authority from Route 4, Route 5,
  Route 7, facade status, route dumps, expectations, or allowlists.
- Reject reactivating ideas 647 or 655 before positive prepared producer
  evidence exists.
- Reject target materialization that chooses stack destinations before prepared
  authority is published.
- Reject broad stack rewrites that mix producer records, MIR lowering, and test
  policy cleanup without one proof surface.
- Reject expectation rewrites, unsupported downgrades, timeout changes, or
  route-dump-only proof for executable behavior changes.
