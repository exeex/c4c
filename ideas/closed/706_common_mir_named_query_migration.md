# Common MIR Named Query Migration

Status: Complete
Type: common MIR consumer migration
Parent: `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
After:
- `ideas/open/704_bir_semantic_handoff_views.md`
- `ideas/open/705_prepared_fact_boundary_from_bir_views.md`

## First Owner And Scope

First owning layer: common `src/backend/mir/query.cpp` and `query.hpp`.  Replace
direct Routes 1-8 and route-index queries with named BIR source-semantic views
or prepared MIR core/function/feature views according to fact ownership.

First migrated consumer: `src/backend/mir/query.cpp`; target callers retain
their current materialization until ideas 708-710.

## Dependencies And Proof

- Depends on ideas 704 and 705.
- Proof surface: common query contract tests and compilation/behavior of x86,
  AArch64, and RV64 handoff tests through the common interface.

## Retirement Guard

The umbrella guard must reach zero in common `mir/query.*`.  Query headers may
not include route headers, return route records, or accept route indexes.

## Acceptance Criteria

- Source-semantic queries use named BIR results; placement queries use prepared
  views and fail closed.
- Common MIR does not rerun BIR analysis or reconstruct prepared authority.
- Existing target behavior remains proved without a route fallback.

## Completion

- Closed after the Step 6 audit confirmed zero Routes 1-8 or route-index
  authority in common `mir/query.*`, ownership-correct named BIR and prepared
  view consumption, explicit fail-closed handling, and no common or
  target-caller route fallback.
- The default build and directly affected common, x86, AArch64, and RV64
  contracts passed. Matching broader `^backend_` regression logs retained the
  identical accepted baseline of 373 passed and 24 failed tests, with no new
  failure.
- Target materializer cleanup remains owned by ideas 708-710; the positive
  stack-destination authority gate remains owned by idea 707.

## Reviewer Reject Signals

- A generic query hides a `RouteN*` return behind a new name.
- Missing prepared authority falls back to route discovery.
- Target-specific policy is moved into the common query layer.
