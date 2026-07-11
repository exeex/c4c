# BIR Semantic Handoff Views

Status: Open
Type: BIR contract implementation
Parent: `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
After: umbrella Step 3 handoff

## First Owner And Scope

First owning layer: `src/backend/bir/` semantic-view production.  Define narrow
`BirProducerView`, `BirMemoryAccessView`, `BirPublicationView`,
`BirCallBoundaryView`, `BirComparisonView`, `BirReturnView`, and
`BirControlFlowView` contracts and the public/private header boundary.  Existing
route builders may implement them privately during migration.

First migrated consumer: the producer/source-semantic entry points used by
common `src/backend/mir/query.cpp`; prepared consumers migrate in idea 705.

## Dependencies And Proof

- Depends on the contracts in
  `docs/bir_mir_contract_abstraction/02_ownership_and_named_handoff_contracts.md`.
- Proof surface: BIR contract/unit tests for available, unavailable, incomplete,
  and ambiguous results; compile guard proving public named headers expose no
  route type or builder index.

## Retirement Guard

Run the umbrella route-vocabulary guard.  This idea need not delete private
`bir_route*.cpp`, but no new named public header or result may mention or return
`RouteN`, `RouteIndex`, `route_index`, or a full route record.

## Acceptance Criteria

- Stable identities and explicit availability are exposed without frame,
  home, move, freshness, ABI, or destination authority.
- Each named result has focused positive and negative producer proof.
- Direct public route declarations shrink; private adapters do not acquire new
  downstream callers.

## Reviewer Reject Signals

- A renamed wrapper, alias, inheritance layer, or accessor exposes a complete
  route record.
- BIR starts selecting prepared homes, moves, freshness, or target placement.
- Proof is only a renamed dump/fixture or one named testcase.
