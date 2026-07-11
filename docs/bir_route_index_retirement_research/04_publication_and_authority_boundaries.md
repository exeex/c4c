# Publication And Authority Boundary Classification

This document classifies the publication boundary around Route 4, Route 5,
and Route 7 so the route-index retirement work can separate executable
prepared authority from numbered-route proof records. The core rule is simple:
prepared data owns codegen authority; route-numbered records may describe BIR
semantic facts or validate agreement, but they do not decide where target
codegen reads, writes, moves, or refreshes a value unless a named prepared
record explicitly carries that authority.

The current authority boundary is anchored by `PreparedEdgePublicationLookups`,
`PreparedMoveBundleLookups`, prepared value homes, freshness records, and
`PreparedMirCoreView`. Route 4, Route 5, and Route 7 records can still be
useful local proof surfaces while the migration is in progress.

## Classification Matrix

| Surface | Current role | May influence codegen? | Authority boundary |
| --- | --- | --- | --- |
| Route 4 current-block publication records | BIR publication availability around a block-local use. | Only as local compatibility evidence when a target path still checks old BIR publication availability. | Observational unless converted into a named prepared fact. It must not override prepared value homes, move bundles, or `PreparedMirCoreView` bindings. |
| Route 4 block-entry publication records | BIR block-entry publication agreement for a destination value. | No direct codegen authority in prepared data. The executable block-entry publication is `PreparedBlockEntryPublication`; Route 4 only annotates agreement. | Diagnostic/proof. `PreparedCurrentBlockEntryPublication` stores `route4_block_entry_publication_*` fields, but those fields are attribution, not destination authority. |
| Route 5 CFG edge publication records | BIR edge/join publication identity and source producer shape. | Yes only when the raw BIR publication fact is used to construct or validate a named publication view during migration. Existing executable edge publication authority is prepared-owned. | Semantic BIR publication fact plus diagnostic agreement. The codegen owner is `PreparedEdgePublicationLookups` with `PreparedMoveBundleLookups`, value homes, and selected freshness. |
| Route 5 current-block join-source records | Historical BIR agreement for prepared current-block join source facts. | No. The prepared fact already carries source/destination homes, publication, move, and freshness. | Observational proof. The `route5_join_source`, `route5_join_source_status`, and `route5_join_source_agrees` fields must not be treated as source freshness or destination authority. |
| Route 7 comparison instruction, operand, and branch records | BIR control-value facts for comparison operands and materialized branch conditions. | It may influence AArch64 comparison lowering only as target-local validation or fallback while the prepared comparison path is incomplete. | Diagnostic/proof for route-index validation. Route 7 must not become prepared publication, freshness, or stack destination authority. |
| `RouteIndexReferenceFacade` and validation records | Narrow Route 4/Route 7 facade and status vocabulary. | No independent codegen authority. | Compatibility-only. Its statuses are rejectable as authority claims unless backed by prepared data or a named BIR semantic view with explicit ownership. |

## Prepared Publication Authority

`PreparedEdgePublicationLookups` in
`src/backend/prealloc/publication_plans.hpp` owns executable edge publication
records. A `PreparedEdgePublication` carries predecessor/successor labels,
destination value identity, source value identity, source producer shape,
source memory access facts, aggregate stack-source evidence, source and
destination homes, destination storage kind, move phase, parallel-copy
metadata, and pointers to the move bundle and move resolution. The builder in
`src/backend/prealloc/prepared_lookups.cpp` populates those records and indexes
them by edge plus destination.

That means Route 5 may agree with a publication, but it is not the edge move
authority. The move authority comes from `PreparedMoveBundleLookups` and the
`PreparedMoveResolution` referenced by the prepared publication. Destination
authority comes from the prepared destination home and destination storage kind.
Freshness authority comes from `PreparedValueFreshnessAuthority`, not from a
Route 5 status.

Concrete authority surfaces:

- `PreparedFunctionLookups` in `src/backend/prealloc/prepared_lookups.hpp`
  aggregates `PreparedMoveBundleLookups`, `PreparedValueHomeLookups`,
  `PreparedEdgePublicationLookups`, and
  `PreparedEdgePublicationSourceProducerLookups`.
- `PreparedEdgePublication` and
  `PreparedCurrentBlockJoinParallelCopySourceFact` in
  `src/backend/prealloc/publication_plans.hpp` carry prepared publication,
  source, destination, and freshness fields.
- `publish_direct_edge_publication_source_freshness_authority` and
  `prepared_direct_edge_publication_source_freshness_status` in
  `src/backend/prealloc/publication_plans.cpp` select and validate direct edge
  publication freshness.
- `PreparedMirFunctionView::current_block_direct_edge_publication_sources` in
  `src/backend/mir/prepared_view.cpp` rechecks destination-home and freshness
  shape before exposing target-facing direct-edge publication sources.

## Freshness View Boundary

Freshness is a prepared view, not a route-numbered view. For direct edge
publication, the selected authority must have:

- use kind `DirectEdgePublicationSource`
- source kind `DirectEdgePublication`
- proof kind `DirectEdgePublicationMove`
- rank `DirectEdgePublication`
- a reference to the prepared edge publication and prepared move

`PreparedMirCoreView` preserves that rule by rejecting a direct-edge source as
`InvalidSourceFreshness` when the selected freshness authority does not match
the direct-edge publication shape. This is the target-facing freshness view.
Route 4 and Route 5 agreement fields may be printed beside it, but they do not
select the freshness candidate and do not repair a missing or ambiguous
freshness authority.

## Destination Authority Boundary

Destination authority is prepared-owned. For register destinations,
`PreparedMirFunctionView::current_block_direct_edge_publication_sources`
requires a prepared destination home with a register name before exposing an
available direct-edge source. RV64 prepared edge publication emission reads the
prepared `destination_home` from the `PreparedEdgePublication` to choose
register or stack destinations. AArch64 publication dispatch also reads the
prepared publication destination home and value identity.

Residual stack destination authority should remain under prepared stack and
move records, not under Route 4, Route 5, or Route 7. Existing stack-related
authority records such as `PreparedAggregateStackSourceAuthority`, branch
stack-load authority records, and move-bundle authority kinds are prepared
facts. A route-numbered agreement record may prove that an old BIR path saw the
same value, but it cannot authorize a stack slot, register destination, scratch
ownership, or parallel-copy execution site.

## Route-Specific Boundaries

### Route 4

Route 4 can publish BIR facts about current-block and block-entry publication
availability. The current prepared contact is
`PreparedCurrentBlockEntryPublication` in
`src/backend/prealloc/value_locations.hpp`, which stores both the prepared
publication and route-attribution fields:
`route4_block_entry_publication_attributed`,
`route4_block_entry_publication_status`,
`route4_block_entry_publication_route_status`, and
`route4_block_entry_publication_instruction_index`.

`attribute_route4_block_entry_publication_if_agreeing` in
`src/backend/prealloc/prepared_lookups.cpp` builds a one-function Route 4 index
and compares the route block-entry record against the prepared publication
bundle instruction. This function is proof attribution. It does not construct
the destination home, does not select the move, and does not decide codegen
storage.

Classification:

- Codegen-relevant: the prepared `PreparedBlockEntryPublication`, destination
  value id/name, prepared destination home, move bundle, and move resolution.
- Observational/diagnostic: Route 4 facade validation status, route status, and
  attributed instruction index.
- Reject: any claim that Route 4 validation status alone proves a prepared
  destination register, stack slot, or freshness authority.

### Route 5

Route 5 is split. The raw BIR CFG edge publication record is a real BIR
publication semantic fact, so a named `BirPublicationView::CfgEdge` may expose
it during migration. The prepared executable authority is still
`PreparedEdgePublicationLookups` plus move bundles, value homes, and freshness.

The current agreement annotation lives on
`PreparedCurrentBlockJoinParallelCopySourceFact`:
`route5_join_source`, `route5_join_source_status`, and
`route5_join_source_agrees`. `attach_route5_current_block_join_source_if_agrees`
in `src/backend/prealloc/publication_plans.cpp` attaches those fields only
after the prepared fact is already available and after the Route 5 record
matches predecessor/successor labels, destination value, source value, and
source producer shape. The route record is therefore trailing proof.

Classification:

- Codegen-relevant: `PreparedEdgePublication`, `PreparedEdgeCopySourceFacts`,
  selected source freshness authority, destination/source homes, move bundle,
  move resolution, and aggregate stack-source authority.
- BIR-semantic but not prepared authority: Route 5 CFG edge publication records
  and current-block join-source records.
- Observational/diagnostic: stored `route5_*` status/agreement fields and dump
  output such as `route5_status` and `route5_agrees` in
  `src/backend/prealloc/prepared_printer/select_chains.cpp`.
- Reject: any claim that `route5_join_source_agrees` substitutes for selected
  source freshness, destination-home validity, move-bundle authority, or stack
  destination authority.

### Route 7

Route 7 is control-value and comparison proof, not publication authority.
AArch64 comparison lowering currently builds a local Route 7 comparison index
and uses `route_index_validate_materialized_condition_reference` and
`route_index_validate_comparison_operand_reference` in
`src/backend/mir/aarch64/codegen/comparison.cpp`. Those checks compare a
materialized condition and compare operands against BIR route records before
continuing with prepared comparison producer facts.

Route 7 can remain a named BIR comparison/control-value view during migration.
It must not cross into publication, freshness, or stack destination authority.

Classification:

- Codegen-relevant only as target-local comparison validation/fallback until
  prepared comparison facts fully cover the path.
- Observational/diagnostic: `Route7IndexReferenceValidation`,
  `Route7ComparisonStatus`, and `RouteIndexRelationshipKind` records.
- Reject: any Route 7-based argument for prepared edge publication source,
  destination home, source freshness, stack copy authority, or move execution
  authority.

## Codegen Versus Observational Records

Records that may influence codegen:

- `PreparedEdgePublication`
- `PreparedEdgePublicationLookups`
- `PreparedEdgeCopySourceFacts`
- `PreparedCurrentBlockJoinParallelCopySourceFact`, excluding `route5_*`
  fields
- `PreparedMoveBundleLookups`
- `PreparedValueHomeLookups`
- `PreparedValueFreshnessAuthority`
- `PreparedAggregateStackSourceAuthority`
- `PreparedMirCoreView` and `PreparedMirFunctionView` direct-edge source views
- Named BIR publication/control views only when they are explicitly consumed as
  semantic input or target-local validation

Records that must remain observational or diagnostic unless a later plan gives
them named ownership:

- `RouteIndexReferenceFacade`
- `Route4IndexReferenceValidation`
- `Route7IndexReferenceValidation`
- Route 4 route-index validation status fields on
  `PreparedCurrentBlockEntryPublication`
- Route 5 `route5_join_source`, `route5_join_source_status`, and
  `route5_join_source_agrees`
- Route-numbered status names printed in prepared dumps
- Route 7 operand/materialized-condition validation status records

## Reviewer Reject Rules

A reviewer should reject publication or authority claims when:

1. The claim uses only `RouteIndexReferenceFacade`, Route 4 status, Route 5
   status/agreement, or Route 7 validation status as the source of executable
   authority.
2. A Route 4 or Route 5 agreement record is used to justify codegen without
   matching prepared destination home, move bundle, edge publication, and
   freshness records.
3. A Route 5 `route5_join_source_agrees` field is treated as proof of source
   freshness instead of requiring selected `PreparedValueFreshnessAuthority`.
4. A Route 4 block-entry attribution field is treated as destination authority
   instead of a proof row for `PreparedBlockEntryPublication`.
5. A Route 7 comparison validation result is used outside comparison/control
   lowering to authorize publication, freshness, stack destination, or move
   execution.
6. A route-numbered debug record is added to `PreparedFunctionLookups` or
   `PreparedMirCoreView` as stable public state instead of being recomputed at
   a diagnostic surface.
7. A dump-only or proof-only field is promoted into codegen decisions without a
   named prepared owner and a migration rule that removes dependence on route
   numbering.

Acceptable claims must name the authority explicitly: prepared edge
publication, prepared move bundle, prepared value home, prepared source
freshness, prepared stack/move authority, or a named BIR semantic view whose
scope is limited to BIR facts.

## Retirement Guidance

Route 4, Route 5, and Route 7 should retire in different ways:

- Route 4: remove persistent prepared route-attribution fields after the proof
  rows can recompute or print named publication agreement locally.
- Route 5: keep raw BIR edge publication semantics available through a named
  publication view, but move stored `route5_*` agreement out of prepared
  source facts once diagnostics can recompute it locally.
- Route 7: replace route-index validation status with a named comparison proof
  adapter or prepared comparison facts; do not route it through publication or
  freshness state.

This keeps publication semantics visible while preventing route-numbered
debug/proof records from becoming hidden codegen authority.
