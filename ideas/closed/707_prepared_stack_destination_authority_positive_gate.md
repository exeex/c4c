# Prepared Stack Destination Authority Positive Gate

Status: Closed
Type: prepared producer capability gate
Parent: `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
Blocks Resume Of:
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`
After:
- `ideas/open/705_prepared_fact_boundary_from_bir_views.md`
- `ideas/open/706_common_mir_named_query_migration.md`

## First Owner And Scope

First owning layer: prealloc/prepared stack-destination fact production.  Emit
a unique positive `PreparedStackDestinationAuthorityView` row by composing
frame, home, move, freshness, publication, aggregate stack-source, and branch
stack-load facts; do not infer it in MIR.

First migrated consumer: a cursor-bound prepared MIR feature view that accepts
`Available` and rejects all negative states.

## Dependencies And Proof

- Depends on the named prepared boundary and fail-closed MIR view seams from
  ideas 705 and 706.
- Proof surface: prepared-contract plus object/object-runtime or runtime proof
  for one positive producer and at least one missing, ambiguous, or route-only
  rejection.

## Retirement Guard

No positive row may contain or be selected by route vocabulary.  The umbrella
guard must not grow, and ideas 647/655 remain parked until every positive row
field and fail-closed proof in the Step 2 gate is present.

## Acceptance Criteria

- The row names destination/source values and complete homes, selected move,
  selected freshness, cursor/edge, publication, and required stack evidence.
- Aggregate and branch cases carry complete slot/object/layout/load evidence.
- MIR consumes only `Available`; missing, invalid, ambiguous, incomplete, and
  route-only states fail closed.

## Reviewer Reject Signals

- Route agreement, dumps, source order, or final assembly substitutes for a
  positive prepared producer.
- MIR rediscovers destination fan-in or manufactures missing facts.
- Only a negative gate test exists, or the positive proof is testcase-shaped.

## Closure

Idea 707 is complete. One real out-of-SSA select-materialized edge-copy
producer now emits a complete, uniquely bound prepared stack-destination
authority row, and the bounded MIR feature view consumes only its `Available`
state. Focused coverage proves the positive path and fail-closed missing,
invalid, ambiguous, incomplete, mismatched, route-only, and upstream states.
The close-time backend regression guard matched the accepted baseline exactly
at 374 passed and 24 failed, with no new failures.

Ideas 647 and 655 remain parked. Idea 707's completed row does not provide the
positive non-637 ordered-final-state, mutual-exclusion, or explicit-merge
producer evidence required at their failing consumer points; they must not be
reactivated until that prepared/prealloc evidence exists.
