# Follow-Up Idea Recommendations

This document recommends the dependency-ordered follow-up work that should come
after the route-index retirement research package. It does not open, edit, or
replace any actual `ideas/open/` files. The intent is to give the plan owner a
clean queue of implementation or umbrella ideas to write later.

The ordering is dependency-based: first create named compatibility surfaces,
then move the low-risk facade consumers, then clean publication proof storage
and dump vocabulary, and only then revisit stack destination authority work.
Route-numbered agreement must remain proof or compatibility data until a named
view or prepared authority record replaces it.

## Recommended Dependency Order

| Order | Follow-up idea | Depends on | Owns | Does not own |
| --- | --- | --- | --- | --- |
| 1 | Introduce named BIR compatibility views and proof adapters. | This research package. | Thin wrappers such as `BirPublicationView`, `BirControlValueView`, and `BirCompatibilityProofView` that forward to existing Route 4, Route 5, and Route 7 builders. | Consumer rewrites, dump rewrites, or facade deletion. |
| 2 | Move the Route 4 prepared lookup attribution consumer to named publication proof. | Idea 1. | `attribute_route4_block_entry_publication_if_agreeing` and related block-entry publication proof vocabulary. | Removing route fields from prepared records or changing dump output. |
| 3 | Move the Route 7 AArch64 comparison consumer to named comparison proof. | Idea 1. | A named comparison/control-value proof adapter around current Route 7 validation for materialized conditions and compare operands. | Publication, freshness, move, or stack destination authority. |
| 4 | Move the Route 4 prepared-printer edge to named block-entry publication agreement. | Ideas 1 and 2. | Printer-side proof labels and local agreement lookup for block-entry publication rows. | Broad prepared dump policy or expectation churn outside the selected printer rows. |
| 5 | Move Route 5 current-block join-source agreement behind named publication proof. | Ideas 1, 2, and 4 for the publication naming pattern. | Current-block join-source agreement adapters, stored `route5_*` compatibility use, AArch64/RV64 publication proof readers, and printer consumers. | Treating Route 5 agreement as source freshness, move execution, or destination authority. |
| 6 | Privatize or delete the narrow `bir_route_index` facade. | Ideas 2, 3, and any remaining facade consumer migration. | `RouteIndexReferenceFacade`, Route 4/Route 7 validation wrapper visibility, and route-index status vocabulary. | Route 1 through Route 8 semantic rebuilds. |
| 7 | Rewrite transitional dump and test vocabulary to named proof surfaces. | Ideas 2 through 6, plus the test policy in file 06. | Route 4, Route 5, and Route 7 dump spelling such as block-entry agreement, current-block join-source agreement, edge-publication agreement, and comparison agreement. | Expectation downgrades, unsupported markers, allowlist changes, or route-view tests that only prove a named failing case. |
| 8 | Extract broader named BIR semantic views route family by route family. | Stable wrappers from Idea 1 and migrated facade consumers. | Producer, memory, publication, control, call, and return named views that replace public Route 1 through Route 8 vocabulary over time. | Stack destination authority or prepared value-home decisions. |
| 9 | Define and implement prepared stack destination authority follow-up. | File 08 stack handoff plus completed publication boundary cleanup. | Explicit prepared stack, frame, value-home, move-bundle, freshness, and destination authority records consumed fail-closed by MIR. | Inferring stack destination authority from Route 4, Route 5, Route 7, route-index status, or dump agreement. |

## Umbrella Idea 1: Named Facade Compatibility Layer

Create the smallest named compatibility layer before moving consumers. This is
the prerequisite for every later route facade cleanup because it gives code a
non-route-numbered name while preserving behavior.

Recommended scope:

- Add named wrappers for Route 4 publication agreement, Route 5 publication or
  join-source agreement, and Route 7 comparison agreement.
- Keep current builders, record lifetimes, statuses, and pointer behavior
  unchanged.
- Keep `RouteIndexReferenceFacade` public until all Route 4 and Route 7
  consumers have moved.
- Prove with compile plus focused backend/BIR tests; this is the only follow-up
  where a named route-view test may be the main proof.

This idea should not remove fields, rewrite dumps, or change prepared
authority. It is the dependency that makes later cleanup behavior-preserving.

## Umbrella Idea 2: Route Facade Cleanup

Retire the narrow `bir_route_index` facade only after the named proof adapters
exist and consumers have moved. The facade is a Route 4 and Route 7 compatibility
surface, not a registry for all routes.

Recommended dependent slices:

1. Route 4 block-entry attribution reads named publication proof instead of the
   route facade.
2. Route 7 AArch64 comparison validation reads named comparison proof instead of
   the route facade.
3. Route 4 prepared-printer rows read named block-entry agreement.
4. Only then privatize or delete `RouteIndexReferenceFacade`,
   `RouteIndexRoute`, `RouteIndexRecordReference`,
   `Route4IndexReferenceValidation`, and `Route7IndexReferenceValidation`.

Route facade cleanup is complete only when no consumer requires public
route-index status as the entry point. Route 4 and Route 7 may still have
private compatibility builders behind named proof adapters during the
migration.

## Umbrella Idea 3: Publication Boundary Cleanup

Separate Route 4 and Route 5 publication proof from prepared publication
authority. This work should follow the first facade consumer moves because it
touches wider prealloc, prepared-printer, and target diagnostic surfaces.

Recommended scope:

- Route 4: remove persistent prepared route-attribution dependence after
  block-entry publication agreement is locally recomputable or named.
- Route 5: move `route5_join_source`, `route5_join_source_status`, and
  `route5_join_source_agrees` use behind a named publication proof adapter.
- Preserve prepared authority in `PreparedEdgePublicationLookups`,
  `PreparedMoveBundleLookups`, value homes, selected freshness, and
  `PreparedMirCoreView`.
- Reject any implementation that treats Route 4 or Route 5 agreement as source
  freshness, move-bundle authority, stack destination authority, or destination
  storage proof.

This idea should be proven by prepared publication, prepared-printer, MIR
handoff, object, or runtime checks as appropriate. Route-view proof is
insufficient when executable publication behavior is touched.

## Umbrella Idea 4: Dump And Test Policy Rewrite

Rewrite public diagnostic spelling after named proof surfaces exist. Do not use
dump rewrites as the first implementation step; dumps should follow the
semantic move, not lead it.

Recommended scope:

- Replace Route 4 dump snippets with named block-entry publication agreement.
- Replace `route5_status` and `route5_agrees` with named current-block
  join-source or edge-publication agreement, or delete them when prepared and
  downstream proof cover the same behavior.
- Replace Route 7 route-index status checks with named comparison/control-value
  proof or prepared branch/comparison records.
- Keep semantic `--dump-bir` route-free.
- Promote prepared, MIR, object, object-runtime, and runtime tests above
  intermediate route dump comparison.

New route-view tests need an explicit migration purpose, a named view contract,
and a planned deletion or rewrite point. They are not acceptable as baseline
churn for narrow Route 4, Route 5, or Route 7 cases.

## Umbrella Idea 5: Broader Named View Extraction

Extract durable named BIR views after the compatibility and facade cleanup path
is proven. This should be split by view family rather than handled as one large
route retirement patch.

Recommended dependency order:

1. `BirPublicationView` for Route 4 and Route 5 publication facts, after
   publication proof cleanup has a named adapter.
2. `BirControlValueView` for Route 2 select-chain and Route 7 comparison facts,
   after Route 7 comparison proof has moved.
3. `BirProducerView` for Route 1 producer identity, because Route 1 is shared
   by publication, control, call, materialization, and target dispatch paths.
4. `BirMemoryAccessView` for Route 3, after memory-source proof is separated
   from prepared memory authority.
5. `BirCallBoundaryView` for Route 6, after target call lowering consumes
   prepared call plans as authority.
6. `BirReturnChainView` for Route 8 only after return-chain ownership is kept
   separate from value-home or stack destination authority.

Each extraction should preserve the old route builder as a private
implementation detail until all direct consumers have moved and proof is green.

## Stack Destination Authority Follow-Up

Stack destination authority should be a separate follow-up, not part of route
facade cleanup. Publication and comparison routes can provide BIR facts or
compatibility proof, but MIR must consume explicit prepared authority for stack
destinations.

Recommended scope:

- Define prepared records for frame layout, stack slots, value homes, move
  bundles, freshness, aggregate stack sources, and destination authority.
- Make MIR consumption fail closed when a stack destination lacks explicit
  prepared producer evidence.
- Allow named BIR publication, producer, memory, call, or control views to feed
  prepared authority construction only through explicit prepared records.
- Forbid direct MIR inference from Route 4, Route 5, Route 7, route-index
  validation status, or dump rows.

This work should wait for file 08, because the stack view handoff must identify
the exact prepared producer evidence and fail-closed behavior before any
implementation idea is activated.

## Prerequisites For Revisiting Ideas 647 And 655

Ideas 647 and 655 should not resume immediately after this research. Their
prerequisite is positive prepared authority evidence, not another route-index
or dump comparison patch.

Before revisiting idea 647 or idea 655, the repo should have:

- A written stack view and destination authority handoff from file 08.
- Named publication and comparison proof adapters for Route 4, Route 5, and
  Route 7, or an explicit statement that the stack work does not depend on
  those compatibility routes.
- Prepared stack destination authority records that identify the producer of a
  stack destination, the selected value home, the move bundle or frame record,
  and the freshness or aggregate stack-source authority.
- MIR-side fail-closed checks that reject missing or ambiguous stack
  destination authority instead of rediscovering it from raw route records.
- At least one proof path above route dumps: prepared contract, prepared MIR,
  object, object-runtime, or runtime proof for the stack destination behavior
  being changed.

If those prerequisites are absent, ideas 647 and 655 should remain blocked or
be rewritten by the plan owner around the missing prepared authority producer.
They should not be satisfied by Route 4, Route 5, Route 7, facade, dump, or
baseline-only evidence.

## Ordering Summary

The shortest safe path is:

1. Add named compatibility adapters.
2. Move Route 4 and Route 7 facade consumers.
3. Move Route 4 printer proof.
4. Move Route 5 publication proof and stored agreement users.
5. Privatize or delete the route facade.
6. Rewrite dumps and tests to named proof vocabulary.
7. Extract broader named views route family by route family.
8. Revisit stack destination authority, including ideas 647 and 655, only after
   explicit prepared authority prerequisites exist.

This ordering keeps route-numbered compatibility available until each consumer
has a named replacement, while preventing Route 4, Route 5, Route 7, dump text,
or facade status from becoming hidden publication, freshness, or stack
destination authority.
