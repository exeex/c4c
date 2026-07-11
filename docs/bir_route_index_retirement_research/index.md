# BIR Route Index Retirement Research Package

This package collects the Step 1 through Step 8 research answers for retiring
the current `bir_route_index` surface. The index is a navigation and synthesis
page only; the numbered answer files remain the source of the detailed
inventory, boundary classifications, sequencing, test policy, follow-up
recommendations, and stack handoff.

## Package Contents

The verified package shape is exactly one index plus eight numbered answer
files:

1. [01_current_route_inventory.md](01_current_route_inventory.md) inventories
   the current Route 1 through Route 8 files, the narrow Route 4/Route 7
   facade, and the consumers that still depend on route-numbered records.
2. [02_required_bir_to_prealloc_inputs.md](02_required_bir_to_prealloc_inputs.md)
   separates required prepared codegen inputs from route-index proof and debug
   residue.
3. [03_named_view_replacement_shape.md](03_named_view_replacement_shape.md)
   proposes named BIR views for producer, memory, publication, control, call,
   and return facts while keeping prepared authority in prepared records.
4. [04_publication_and_authority_boundaries.md](04_publication_and_authority_boundaries.md)
   classifies Route 4, Route 5, and Route 7 as BIR semantic or diagnostic
   surfaces rather than executable publication, freshness, move, or destination
   authority.
5. [05_retirement_sequence.md](05_retirement_sequence.md) defines a phased
   route-retirement strategy with rollback points and focused proof commands.
6. [06_test_and_dump_policy_after_route_retirement.md](06_test_and_dump_policy_after_route_retirement.md)
   sets the post-retirement test and dump policy, promoting prepared, MIR,
   object, and runtime proof above route dump comparison.
7. [07_followup_idea_recommendations.md](07_followup_idea_recommendations.md)
   recommends dependency-ordered follow-up ideas without editing or replacing
   any active idea file.
8. [08_stack_view_and_destination_authority_handoff.md](08_stack_view_and_destination_authority_handoff.md)
   defines the prepared stack authority handoff and the MIR fail-closed
   consumption rule for stack destination fan-in.

## Recommended Route-Retirement Strategy

The recommended route-retirement strategy is behavior-preserving and staged:
add named compatibility views first, move low-risk facade consumers next, then
privatize or delete `bir_route_index` only after Route 4 and Route 7 consumers
no longer require public route-index status. Route 5 publication proof should
move behind named publication agreement before stored `route5_*` diagnostics or
dump vocabulary are removed.

The route-numbered builders can remain private compatibility implementation
details while public consumers migrate to names that describe ownership:
producer, memory, publication, control, call, and return views. This avoids
treating `RouteIndexReferenceFacade` as a general route registry and keeps
Route 4, Route 5, and Route 7 status fields from becoming hidden authority.

Prepared data remains the executable boundary. `PreparedFunctionLookups`,
prepared edge publications, prepared move bundles, prepared value homes,
freshness records, aggregate stack authority, and `PreparedMirCoreView` own the
facts target lowering may rely on. Named BIR views may feed or validate those
prepared producers, but route-numbered status is proof or compatibility data
unless a later plan creates an explicit named prepared fact.

## Authority And Test Policy Summary

The package recommends keeping route-view and route-dump proof as the weakest
acceptance layer. Future implementation slices should prove behavior through
prepared contracts, prepared MIR views, object emission, object-runtime, or
runtime checks whenever the change affects executable lowering. Route-view
tests should exist only for named BIR semantic views or temporary compatibility
adapters with a planned rewrite or deletion point.

The prepared stack authority handoff is intentionally separate from facade
cleanup. MIR must consume prepared stack, frame, value-home, move-bundle,
freshness, aggregate stack, and destination authority records. It should fail
closed on missing, ambiguous, invalid, or route-only evidence, including
Route 4, Route 5, Route 7, facade status, or dump rows. Ideas 647 and 655 need
positive prepared producer evidence before they resume; route agreement or
baseline-only evidence is insufficient.

## Verification Notes

At assembly time, the directory is expected to contain exactly these Markdown
files:

- `index.md`
- `01_current_route_inventory.md`
- `02_required_bir_to_prealloc_inputs.md`
- `03_named_view_replacement_shape.md`
- `04_publication_and_authority_boundaries.md`
- `05_retirement_sequence.md`
- `06_test_and_dump_policy_after_route_retirement.md`
- `07_followup_idea_recommendations.md`
- `08_stack_view_and_destination_authority_handoff.md`

The package is ready when the file-count proof confirms exactly nine Markdown
files and the index contains the required references to each numbered answer,
route-retirement strategy, prepared stack authority, and fail closed behavior.
