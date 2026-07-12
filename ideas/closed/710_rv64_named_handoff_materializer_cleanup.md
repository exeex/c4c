# RV64 Named Handoff Materializer Cleanup

Status: Complete
Type: RV64 MIR consumer migration
Parent: `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
After:
- `ideas/open/705_prepared_fact_boundary_from_bir_views.md`
- `ideas/open/706_common_mir_named_query_migration.md`

## First Owner And Scope

First owning layer: RV64 MIR materialization.  Make prepared edge publication
and object emission consume prepared authority without branching on Route 3/5
agreement or retaining route-labelled executable intent.

First migrated consumer:
`src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`.

## Dependencies And Proof

- Depends on prepared publication from idea 705 and common queries from 706.
- Proof surface: RV64 prepared edge-publication tests and object-emission
  behavior/dump proof with semantic decisions independent of agreement rows.

## Retirement Guard

The umbrella guard must reach zero in semantic RV64 emission.  Route-labelled
debug text may be removed with the consumer or deferred explicitly to 712,
but it may never select emission.

## Acceptance Criteria

- Prepared publication is sole executable source and move authority.
- Missing or inconsistent prepared input fails closed.
- Object intent uses ownership-named facts and preserves behavior proof.

## Reviewer Reject Signals

- Route agreement remains a branch condition for emission.
- Dump vocabulary is renamed while executable dependency remains.
- Final assembly alone is treated as authority proof.

## Closure Evidence

- RV64 prepared edge publication and object intent now consume prepared,
  ownership-named authority without executable Route 3/5 agreement.
- Missing or inconsistent prepared inputs retain fail-closed focused proof;
  behavior and structured object-emission proof remain green.
- The semantic RV64 Route 3/5 retirement guard is zero. Remaining generic
  `fallback` vocabulary was classified as unrelated to retired route
  authority, with no scoped debug-only route text left to defer.
- The matching full-suite regression guard passed with identical before/after
  results: 3,373 passed, 56 baseline failures, 3,429 total, zero new failures,
  and zero new timeout violations.
