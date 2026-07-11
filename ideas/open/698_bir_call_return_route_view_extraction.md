# BIR Call And Return Route View Extraction

Status: Open
Type: Implementation
Parent: `ideas/open/694_bir_route_index_retirement_umbrella.md`
Handoff:
- `docs/bir_route_index_retirement/research_digest.md`
- `docs/bir_route_index_retirement/ownership_dependencies.md`
- `docs/bir_route_index_retirement/ordered_followup_plan.md`
Queue Order: 4

## Goal

Extract named BIR call-boundary and return-chain views for Route 6 and Route 8
facts without folding target ABI lowering or return materialization policy into
the BIR view layer.

## Why This Exists

The handoff identifies Route 6 call argument/result source reconstruction and
Route 8 return-chain value identity as independent route-numbered APIs, not
parts of the narrow `bir_route_index` facade. They need their own named view
contract before prealloc or target consumers can stop depending on route names.

## Owned Files

- `src/backend/bir/` call-boundary, return-chain, and named view/proof adapter
  code.
- Focused backend tests for the named call-boundary or return-chain BIR view
  contract.

## First Owning Layer

BIR call-boundary and return-chain semantic view layer.

## First Consumer Migration

Move one diagnostic or proof reader for Route 6 call argument/result source
facts to a `BirCallBoundaryView` equivalent. Route 8 return-chain migration
should follow only if the return-chain facts remain target-independent enough
for a public named view.

## Proof Surface

Focused backend proof that the named call-boundary view preserves call
argument/result source reconstruction. Target MIR, object, object-runtime, or
runtime proof is required if a packet changes call or return executable
behavior.

## Numbered Route APIs Kept Private Compatibility

- `Route6CallUseSourceIndex`
- `Route8ReturnChainIndex`

These may remain private implementation details behind named call-boundary or
return-chain views. They must not become target ABI policy, prepared
destination authority, frame layout authority, value-home authority, freshness
authority, or MIR lowering authority.

## In Scope

- Add a named Route 6 call-boundary view or proof adapter.
- Decide whether Route 8 return-chain facts are public BIR semantics or
  target-specific compatibility.
- Migrate one first proof reader through the named call-boundary surface.
- Preserve old route data as rollback compatibility while consumers move.

## Out Of Scope

- ABI lowering changes.
- Stack destination fan-in repair.
- Publication, memory, producer, or dump-policy cleanup.
- Rewriting call or return expectations as proof of migration.

## Acceptance Criteria

- The first call-boundary proof reader uses named BIR vocabulary instead of
  route-numbered public access.
- Route 6 and Route 8 remain private compatibility unless a named view owns
  the semantic contract.
- Any executable call or return behavior change is proven above route dumps.
- Missing call or return evidence stays explicit and fail-closed.

## Reviewer Reject Signals

- Reject mixing call/return view extraction with target ABI lowering or stack
  destination repair.
- Reject exposing `Route6CallUseSourceIndex` or `Route8ReturnChainIndex` as
  durable public architecture under renamed wrappers.
- Reject using route facts as value-home, frame-layout, move-bundle,
  freshness, or MIR destination authority.
- Reject named-case-only fixes for one call or return testcase.
- Reject expectation rewrites, unsupported downgrades, allowlist edits, or
  route-dump-only proof for executable behavior changes.
