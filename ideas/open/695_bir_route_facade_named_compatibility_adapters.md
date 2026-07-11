# BIR Route Facade Named Compatibility Adapters

Status: Open
Type: Implementation
Parent: `ideas/open/694_bir_route_index_retirement_umbrella.md`
Handoff:
- `docs/bir_route_index_retirement/research_digest.md`
- `docs/bir_route_index_retirement/ownership_dependencies.md`
- `docs/bir_route_index_retirement/ordered_followup_plan.md`
Queue Order: 1

## Goal

Introduce ownership-named BIR compatibility adapters for the narrow
`bir_route_index` Route 4 and Route 7 facade, then move the first facade users
without changing behavior or prepared authority.

## Why This Exists

The handoff shows that `bir_route_index` is not a general Route 1 through
Route 8 public API. It is a compatibility facade over Route 4 publication
validation and Route 7 comparison validation. This idea creates named adapter
entry points so later packets can stop treating route-index status as durable
public architecture.

## Owned Files

- `src/backend/bir/` route-index facade, route proof adapter, and named BIR
  proof-view headers and implementations.
- Focused backend tests only when they prove the named adapter contract without
  changing expectations or default harness policy.

## First Owning Layer

BIR compatibility and proof-adapter layer.

## First Consumer Migration

Move the first low-risk Route 4 block-entry attribution or Route 7 comparison
diagnostic caller from direct `RouteIndexReferenceFacade` access to a named
publication or comparison proof adapter.

## Proof Surface

Build plus focused backend proof that the moved Route 4 block-entry
attribution or Route 7 comparison diagnostic still publishes the same prepared
or target-facing proof rows through the named adapter. Runtime or object proof
is required if executable behavior changes, but this first idea should be
behavior-preserving.

## Numbered Route APIs Kept Private Compatibility

- `RouteIndexReferenceFacade`
- `RouteIndexRoute`
- `RouteIndexRecordReference`
- `Route4IndexReferenceValidation`
- `Route7IndexReferenceValidation`
- `Route4PublicationAvailabilityIndex`
- `Route7ComparisonConditionIndex`

These may remain as private adapter inputs during migration. They must not be
introduced as new prepared publication, freshness, value-home, stack
destination, frame-layout, branch stack-load, move-bundle, or MIR authority.

## In Scope

- Add or tighten named BIR publication and comparison proof adapter entry
  points over the existing route builders.
- Move one first consumer through the named adapter.
- Keep route-numbered records available as rollback compatibility.
- Record negative or missing adapter states explicitly if the current facade
  cannot provide a named proof row.

## Out Of Scope

- Rebuilding Route 1 through Route 8 semantics.
- Prealloc executable authority changes.
- Route 5 publication cleanup.
- Stack destination authority repair.
- Dump vocabulary retirement or expectation rewrites.

## Acceptance Criteria

- The first facade consumer no longer treats `RouteIndexReferenceFacade` as a
  public architecture boundary.
- Named publication or comparison proof vocabulary owns the migrated call site.
- Route-numbered APIs remain private compatibility and are not exposed as new
  public architecture.
- Focused proof shows no behavior change for the migrated surface.

## Reviewer Reject Signals

- Reject public expansion of `RouteIndexReferenceFacade` or `RouteIndexRoute`
  under a new name.
- Reject treating Route 4 or Route 7 validation status as prepared authority,
  MIR destination authority, source freshness, value-home, or move-bundle
  authority.
- Reject broad Route 1 through Route 8 rewrites inside this facade packet.
- Reject expectation rewrites, unsupported-marker edits, allowlist edits,
  timeout/accounting changes, or weaker runtime checks as progress.
- Reject helper renames or classification-only changes claimed as facade
  contraction if a real consumer still depends on the old public facade.
