# BIR Route Index Retirement Ownership And Dependencies

This handoff classifies route-retirement follow-up work by first owning layer
and dependency order. It is a planning boundary, not an implementation plan:
the route-numbered builders may remain private compatibility while public
consumers move to named views and prepared authority records.

## Ownership Buckets

| Bucket | First owning layer | Scope | Depends on | Must not own |
| --- | --- | --- | --- | --- |
| BIR named view extraction | `src/backend/bir/` semantic view layer | Introduce ownership-named wrappers and later durable views for producer, memory, publication, control, call, and return facts. The first extraction should be behavior-preserving adapters over current route builders. | Research digest and current route inventory. | Prealloc authority, MIR lowering decisions, dump policy, stack destination repair, or route-numbered public expansion. |
| route facade contraction | BIR compatibility/proof adapter layer | Move Route 4 and Route 7 consumers off `RouteIndexReferenceFacade`, then privatize or delete `RouteIndexRoute`, `RouteIndexRecordReference`, `Route4IndexReferenceValidation`, and `Route7IndexReferenceValidation` after consumers have named proof adapters. | Named compatibility wrappers for publication and comparison proof. | Route 1 through Route 8 semantic rebuilds, Route 5 publication authority cleanup, or stack authority. |
| BIR publication boundary cleanup | BIR publication view plus prealloc publication proof boundary | Split Route 4 and Route 5 BIR publication facts from prepared publication authority. Route 4 block-entry attribution and Route 5 join-source agreement become named proof or diagnostics, not durable prepared authority. | Named BIR publication proof adapters and initial facade consumer migration. | Source freshness, move-bundle authority, destination homes, stack slots, or MIR destination selection. |
| prealloc consumer migration | `src/backend/prealloc/` consumers of BIR facts | Move prealloc proof readers from numbered route APIs to named BIR proof views while preserving prepared facts. Early consumers are Route 4 block-entry attribution, Route 4 printer agreement, and Route 5 current-block join-source agreement. | Named BIR compatibility views; route facade contraction sequencing. | Changing executable prepared edge publication behavior, deleting compatibility fields before printers/tests move, or proving correctness only by route dumps. |
| prepared/MIR stack view contract | prepared/prealloc producer records and MIR prepared views | Define status-rich prepared views for frame layout, value homes, move bundles, freshness, aggregate stack sources, branch stack-load authority, and explicit stack destination authority. MIR consumes only `Available` prepared rows and fails closed otherwise. | Publication boundary cleanup and the stack handoff evidence from file 08. | Direct MIR inference from Route 4, Route 5, Route 7, facade status, or dump rows. |
| test/dump contract cleanup | backend test and diagnostic policy | Rewrite transitional Route 4, Route 5, and Route 7 dump vocabulary to named block-entry publication agreement, current-block join-source or edge-publication agreement, and comparison agreement after named proof surfaces exist. | Consumer migration and facade/publication cleanup for the rows being renamed. | Expectation downgrades, unsupported-marker edits, allowlist filtering, baseline-only acceptance, or dump rewrites that lead semantic migration. |
| residual stack authority prerequisites | prepared stack authority producer evidence | Revisit residual stack destination work only after positive prepared producer seams exist above route dumps. This includes selected value home, move bundle or move resolution, freshness, aggregate stack source, branch stack-load, and MIR fail-closed proof. | prepared/MIR stack view contract with positive producer evidence. | Reactivating ideas 647 or 655 from route agreement, facade, dump, expectation, or allowlist evidence alone. |

## Dependency Model

The route-retirement dependency order is:

1. BIR named view extraction starts with thin compatibility wrappers. These
   wrappers give later packets ownership-named entry points while retaining the
   current route builders, record lifetimes, statuses, and behavior.
2. route facade contraction moves the narrow `bir_route_index` users next,
   because the facade is only a Route 4 and Route 7 compatibility surface. The
   first low-risk moves are Route 4 block-entry attribution and Route 7
   comparison validation through named proof adapters.
3. prealloc consumer migration follows the named wrappers for the exact
   consumer being moved. It may keep old route-numbered fields as rollback and
   printer compatibility while call sites stop treating the facade as public
   architecture.
4. BIR publication boundary cleanup follows the first Route 4 and Route 7
   moves. Route 5 is later because it mixes real CFG-edge publication facts
   with trailing `route5_*` proof residue and prepared edge publication
   contacts.
5. test/dump contract cleanup follows semantic and consumer migration. Dump
   spelling should not lead the implementation; it should reflect named proof
   surfaces that already exist.
6. prepared/MIR stack view contract work is separate from facade cleanup. It
   may consume named BIR facts as inputs, but authority must be prepared-owned
   before MIR lowers stack or destination behavior.
7. residual stack authority prerequisites are last. Ideas 647 and 655 remain
   parked until the prepared stack authority work exposes positive producer
   seams and proof above route dumps.

This order prevents mixed-owner packets: BIR wrappers do not alter prepared
authority, prealloc migrations do not rewrite test policy, dump cleanup does
not create semantic facts, and stack authority does not borrow route-numbered
agreement as executable evidence.

## Route-Numbered Private Compatibility

These APIs may remain during migration only as private compatibility builders,
proof adapters, or diagnostic bridges:

- `RouteIndexReferenceFacade`, `RouteIndexRoute`,
  `RouteIndexRecordReference`, `Route4IndexReferenceValidation`, and
  `Route7IndexReferenceValidation` may remain until Route 4 and Route 7
  consumers no longer require public route-index status.
- `Route4PublicationAvailabilityIndex` may back
  `BirPublicationView::BlockEntry`, `BirPublicationView::CurrentBlock`, and a
  named block-entry publication agreement proof.
- `Route5EdgeJoinSourceIndex`, Route 5 CFG-edge records, and Route 5
  current-block join-source records may back a named publication proof adapter,
  while `route5_join_source`, `route5_join_source_status`, and
  `route5_join_source_agrees` stay diagnostic compatibility only.
- `Route7ComparisonConditionIndex` and Route 7 validation records may back a
  named comparison/control-value proof adapter for AArch64 comparison
  migration.
- `Route1ProducerIndex`, `Route2SelectChainValueIndex`,
  `Route3MemoryAccessIndex`, `Route6CallUseSourceIndex`, and
  `Route8ReturnChainIndex` may remain private route-numbered internals until
  their named producer, control, memory, call, and return view plans own the
  migration.

None of these compatibility APIs may become prepared publication, source
freshness, move-bundle, value-home, stack destination, frame layout, branch
stack-load, or MIR authority. If prepared or MIR needs a fact, a named prepared
record or named MIR prepared view must own it.

## Residual Stack Authority Prerequisites

The prepared/MIR stack view contract must establish positive producer evidence
before residual stack destination work resumes. The minimum dependency is a
producer or existing prepared record family that names:

- destination value identity, destination home, and destination storage kind;
- source value/home plus selected move bundle or move resolution;
- selected freshness with use kind, source kind, proof kind, rank, and
  prepared reference;
- frame slot, offset, size, alignment, and stack object or aggregate stack
  authority evidence for stack sources;
- branch stack-load evidence with branch cursor, role, value home, frame slot,
  stack object, source freshness status, and selected freshness authority when
  branch loads are involved;
- MIR fail-closed statuses for missing, ambiguous, invalid, or route-only
  authority; and
- proof at prepared contract, prepared MIR, object, object-runtime, or runtime
  level when executable behavior changes.

Ideas 647 and 655 remain parked because current route-retirement evidence does
not itself expose that producer seam. Route 4, Route 5, Route 7,
`RouteIndexReferenceFacade`, dump rows, expectations, and allowlists can show
historical agreement or compatibility only. They are insufficient to choose a
stack destination, repair fan-in, authorize a move bundle, select freshness, or
make MIR lower a stack path.

## Classification Outcome

The follow-up queue should therefore be split by first owner:

- BIR first: add named compatibility wrappers and later extract producer,
  memory, publication, control, call, and return semantic views.
- BIR compatibility first: contract the Route 4/Route 7 facade after named
  proof adapters exist.
- BIR/prealloc boundary first: move publication proof and route agreement
  consumers without changing executable prepared authority.
- prealloc/prepared first: define stack/frame/value-home/move/freshness and
  destination-authority records before MIR consumes stack destinations.
- test policy first only after named proof surfaces exist: rewrite route dump
  vocabulary and selected expectations without weakening contracts.
- residual stack authority last: revisit 647 and 655 only when positive
  prepared producer evidence exists above route dumps.
