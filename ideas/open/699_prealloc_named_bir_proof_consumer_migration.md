# Prealloc Named BIR Proof Consumer Migration

Status: Open
Type: Implementation
Parent: `ideas/open/694_bir_route_index_retirement_umbrella.md`
Handoff:
- `docs/bir_route_index_retirement/research_digest.md`
- `docs/bir_route_index_retirement/ownership_dependencies.md`
- `docs/bir_route_index_retirement/ordered_followup_plan.md`
Queue Order: 5
Depends On:
- `ideas/open/695_bir_route_facade_named_compatibility_adapters.md`
- `ideas/open/697_bir_memory_publication_view_extraction.md`

## Goal

Move prealloc proof consumers from numbered BIR route APIs to named BIR proof
views while preserving executable prepared facts.

## Why This Exists

The handoff separates BIR view extraction from prealloc consumer migration.
Prealloc should consume named BIR proof rows for diagnostics and agreement,
but prepared function lookups, value homes, edge publications, move bundles,
freshness, stack-source authority, and prepared MIR views remain the executable
authority.

## Owned Files

- `src/backend/prealloc/` consumers that currently read BIR route proof rows.
- BIR-to-prealloc boundary adapters needed to consume named BIR proof views.
- Focused prepared lookup, prepared-printer, or backend tests for the migrated
  consumer.

## First Owning Layer

BIR-to-prealloc consumer boundary.

## First Consumer Migration

Move Route 4 block-entry attribution in prepared lookup proof code to the
named publication proof view. Follow with Route 4 prepared-printer agreement or
Route 7 AArch64 comparison proof only after the first migration is proven.

## Proof Surface

Prepared lookup or prepared-printer proof for the first migrated Route 4
consumer, then focused AArch64 comparison proof for the first Route 7 consumer.
Executable behavior changes require prepared MIR, object, object-runtime, or
runtime proof.

## Numbered Route APIs Kept Private Compatibility

- `RouteIndexReferenceFacade`
- `Route4IndexReferenceValidation`
- `Route7IndexReferenceValidation`
- `Route4PublicationAvailabilityIndex`
- `Route7ComparisonConditionIndex`
- Route 5 publication agreement rows only after named publication proof exists

These may remain private compatibility and rollback inputs. They must not be
copied into prepared authority records as stable route-numbered state.

## In Scope

- Migrate one prealloc proof consumer at a time to a named BIR proof view.
- Retain route-numbered fields and printer compatibility during rollback.
- Keep executable prepared records unchanged unless the packet proves a real
  prepared behavior change.
- Add focused proof for each migrated consumer.

## Out Of Scope

- Adding new BIR view semantics.
- Changing prepared edge publication behavior.
- Stack/frame/value-home authority.
- Dump vocabulary cleanup before named consumers exist.

## Acceptance Criteria

- The first prealloc consumer no longer reads a route-numbered API as public
  architecture.
- Prepared executable authority remains named prepared state, not route-index
  status.
- Rollback to the old route consumer remains possible for the migrated
  surface.
- Proof covers the actual migrated consumer, not only a route dump row.

## Reviewer Reject Signals

- Reject copying route-index validation status into `PreparedFunctionLookups`,
  `PreparedMirCoreView`, value-home, freshness, move-bundle, or destination
  authority records.
- Reject migrating multiple unrelated consumers in one packet without a single
  proof surface.
- Reject proving consumer migration only by updating expected route dump text.
- Reject weakening unsupported markers, allowlists, default harness contracts,
  timeouts, or runtime checks.
- Reject helper renames that leave prealloc callers dependent on public
  route-numbered APIs.
