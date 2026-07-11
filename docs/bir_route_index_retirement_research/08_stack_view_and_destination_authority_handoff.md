# Stack View And Destination Authority Handoff

This document defines where stack, frame, value-home, move-bundle, and
destination-authority analysis should live after the numbered BIR route surface
is retired. The split is:

- BIR semantic views describe source-program facts and route publication facts.
- prealloc and prepared producers turn those facts into executable frame,
  value-home, move, freshness, aggregate stack, and destination authority
  records.
- MIR consumes only explicit prepared authority and must fail closed when that
  authority is missing, ambiguous, or not shaped for the requested target use.

Route 4, Route 5, and Route 7 may help build or validate prepared stack
authority during migration. They must not become direct MIR-side inference
inputs for stack destination fan-in, frame slot ownership, freshness, or move
execution.

## Responsibility Split

| Layer | Responsibility | Current code contacts | Non-responsibility |
| --- | --- | --- | --- |
| BIR semantic view | Expose named source facts: producer identity, memory access identity, publication availability, comparison/control facts, call boundary facts, and return-chain facts. Route publication facts may be inputs to a prepared producer. | Proposed names in `03_named_view_replacement_shape.md`; current sources include `Route4PublicationAvailabilityIndex`, `Route5EdgeJoinSourceIndex`, and `Route7ComparisonConditionIndex` in `src/backend/bir/bir.hpp` and route builders under `src/backend/bir/`. | It does not authorize target stack slots, physical registers, scratch ownership, move bundles, or MIR destination selection. |
| prealloc / prepared producer | Own frame layout, value home, move bundle, freshness, edge publication, aggregate stack source, branch stack-load, and explicit destination authority records. | `PreparedFunctionLookups` in `src/backend/prealloc/prepared_lookups.hpp`; `PreparedStackLayout` and `PreparedFramePlanFunction` in `src/backend/prealloc/frame.hpp`; `PreparedValueHome`, `PreparedMoveBundle`, and `PreparedValueFreshnessAuthority` in `src/backend/prealloc/value_locations.hpp`; `PreparedAggregateStackSourceAuthority`, `PreparedBranchStackLoadAuthority`, and `PreparedFrameSlotSourceFact` in `src/backend/prealloc/publication_plans.hpp`. | It should not preserve route-numbered status as semantic authority. Route agreement fields are proof or compatibility data unless converted into a named prepared fact. |
| MIR consumer | Read prepared views and lower only when the requested stack or destination fact has explicit prepared authority. | `PreparedMirCoreView`, `PreparedMirFunctionView`, and `PreparedMirDirectEdgePublicationSourceView` in `src/backend/mir/prepared_view.hpp`; fail-closed checks in `PreparedMirFunctionView::current_block_direct_edge_publication_sources` in `src/backend/mir/prepared_view.cpp`; RV64 stack/freshness checks in `src/backend/mir/riscv/codegen/object_emission.cpp`. | It must not reconstruct stack destination authority from raw Route 4, Route 5, Route 7, facade status, dump rows, or route-index validation records. |

## Current Prepared Contacts

The repo already has several prepared records that should be treated as
contacts for the final stack handoff:

- Frame layout: `PreparedStackObject`, `PreparedFrameSlot`, `PreparedStackLayout`,
  and `PreparedFramePlanFunction` describe stack objects, frame slots, frame
  size, alignment, saved-register slot placement, dynamic stack state, and
  frame-pointer policy in `src/backend/prealloc/frame.hpp`.
- Value home: `PreparedValueHome` records whether a value lives in a register,
  stack slot, rematerializable immediate, or pointer-base-plus-offset home. It
  carries `slot_id`, `offset_bytes`, `size_bytes`, `align_bytes`,
  register identity, and value identity in `src/backend/prealloc/value_locations.hpp`.
- Move bundle: `PreparedMoveBundle` and `PreparedMoveResolution` record phase,
  execution position, move authority kind, destination kind, destination storage
  kind, cycle-temp use, parallel-copy edge labels, and destination placement in
  `src/backend/prealloc/value_locations.hpp` and
  `src/backend/prealloc/regalloc.hpp`.
- Freshness: `PreparedValueFreshnessAuthority` carries use kind, source kind,
  proof kind, rank, and references to homes, publications, edge publications,
  move bundles, moves, and cursor positions in
  `src/backend/prealloc/value_locations.hpp`.
- Aggregate stack source: `PreparedAggregateStackSourceAuthority` records stack
  source slot evidence, copy width, destination storage kind, destination
  register placement, lane mapping flags, ABI-layout reference, scratch
  ownership, and status in `src/backend/prealloc/publication_plans.hpp`.
- Branch stack-load authority: `PreparedBranchStackLoadAuthority` records the
  stack slot, stack object, branch cursor, freshness status, and selected
  source freshness authority required for stack-backed branch loads in
  `src/backend/prealloc/publication_plans.hpp`.
- MIR prepared publication view: `PreparedMirFunctionView::current_block_direct_edge_publication_sources`
  maps prepared publication source facts into MIR-facing views and rejects
  unsupported destination homes or invalid direct-edge freshness in
  `src/backend/mir/prepared_view.cpp`.

These contacts are not yet a single stack destination authority contract. Step
8 therefore recommends a named prepared/MIR view shape that can unify them
without pushing route inference into MIR.

## First-Cut View Shapes

The first implementation idea after this research should define prepared stack
views in terms of existing prepared records. The names below are documentation
contracts, not code added by this packet.

| View shape | Producer | MIR-facing payload | Required status model |
| --- | --- | --- | --- |
| `PreparedFrameLayoutView` | prealloc stack layout and frame planning. | Function name, frame size/alignment, frame slot order, stack object identity, slot id, object id, offset, size, alignment, fixed-location bit, dynamic-stack flags, saved-register slots, and frame-pointer policy. | `Available`, `MissingFunction`, `MissingFramePlan`, `MissingSlot`, `IncompleteSlot`, `DynamicStackUnsupported`, `AmbiguousSlot`. |
| `PreparedValueHomeView` | prepared value-home lookups. | Value id/name, home kind, register name/target register, frame slot id, offset, size, alignment, immediate or pointer-base details, and value type when available. | `Available`, `MissingValue`, `MissingHome`, `UnsupportedHome`, `IncompleteStackHome`, `HomeValueMismatch`. |
| `PreparedMoveBundleView` | prepared move-bundle lookups and regalloc move records. | Move phase, block/instruction cursor, authority kind, source/destination value ids, destination kind/storage kind, destination register or stack offset, parallel-copy predecessor/successor labels, cycle-temp use, and move resolution pointer identity. | `Available`, `MissingBundle`, `MissingMove`, `UnsupportedAuthority`, `UnsupportedDestination`, `IncompleteDestination`, `AmbiguousMove`. |
| `PreparedStackDestinationAuthorityView` | prepared destination-authority producer over frame layout, value homes, move bundles, freshness, and edge publication. | Destination value id/name, destination home, destination frame slot or register placement, source value/home, selected move, selected freshness, aggregate stack source authority, edge publication when relevant, and the BIR semantic view facts used to produce the record. | `Available`, `MissingDestinationHome`, `MissingSourceHome`, `MissingMoveBundle`, `MissingSelectedFreshness`, `InvalidFreshness`, `AmbiguousFreshness`, `MissingAggregateStackAuthority`, `UnsupportedRouteOnlyEvidence`, `AmbiguousFanIn`. |

MIR may consume these views only through prepared status fields. A row with
`UnsupportedRouteOnlyEvidence`, `MissingSelectedFreshness`, or `AmbiguousFanIn`
is a negative result, even if a Route 4, Route 5, or Route 7 record appears to
agree with the same value names.

## Route Publication Feed Rules

Route publication facts may feed stack authority production in one direction:

1. A named BIR publication view exposes current-block, block-entry, or CFG-edge
   publication semantics from the current Route 4 and Route 5 builders.
2. The prepared producer compares those BIR semantic facts with prepared value
   homes, edge publications, move bundles, frame slots, and freshness
   candidates.
3. If all required prepared facts are present and unique, the producer emits a
   named prepared authority record with a positive status.
4. MIR consumes that prepared record and ignores the route identity that helped
   produce or validate it.

Route 7 comparison/control facts can be used the same way for branch stack-load
authority: they may identify the BIR control-value use being lowered, but the
executable stack-load permission must come from `PreparedBranchStackLoadAuthority`
and selected `PreparedValueFreshnessAuthority`.

Rejected feed paths:

- MIR reading Route 4 block-entry status to infer a stack destination.
- MIR reading Route 5 `route5_join_source_agrees` as freshness or move-bundle
  authority.
- MIR reading Route 7 comparison validation as stack-load or destination
  authority.
- A dump row or route facade status becoming sufficient proof of frame layout,
  value home, move bundle, aggregate stack source, or destination authority.

## Positive Producer Evidence For 647 And 655

Ideas 647 and 655 should remain blocked until the implementation route can show
positive producer evidence above route dumps. Minimum evidence:

1. A named prepared stack destination authority producer exists, or the active
   idea explicitly names the existing prepared record family that owns the
   destination decision.
2. The producer records destination value id/name, destination home, and
   destination storage kind from `PreparedValueHome` or an equivalent prepared
   value home view.
3. The producer records source value/home, selected move bundle or move
   resolution, and move authority kind from `PreparedMoveBundle` /
   `PreparedMoveResolution`.
4. The producer records selected freshness with the expected use kind, source
   kind, proof kind, rank, and reference, not merely a route agreement status.
5. Stack sources include frame slot id, offset, size, alignment, and stack
   object or aggregate stack authority evidence. Aggregate copies must name
   `PreparedAggregateStackSourceAuthority` or a successor record with the same
   concrete fields.
6. Stack branch loads include `PreparedBranchStackLoadAuthority`-style evidence:
   branch cursor, role, value home, frame slot, stack object, source freshness
   status, and selected source freshness authority.
7. At least one proof path exercises prepared, prepared MIR, object,
   object-runtime, or runtime behavior. Route 4, Route 5, Route 7, facade, dump,
   expectation, or allowlist evidence alone is insufficient.

If any of those items are missing, ideas 647 and 655 should be rewritten around
the missing producer before implementation resumes.

## Fail-Closed MIR Behavior

MIR stack destination fan-in must fail closed. The consuming rule should be:

- If destination home is missing, non-stack/non-register where unsupported, or
  lacks slot/register details, return an unsupported or missing-authority status.
- If a fan-in has multiple candidate prepared authorities and no unique selected
  authority, return ambiguous authority.
- If freshness is required and no selected `PreparedValueFreshnessAuthority`
  matches the use/source/proof/rank expected by the view, return missing,
  invalid, or ambiguous freshness.
- If aggregate stack source handling is required and
  `PreparedAggregateStackSourceAuthority` is unavailable, incomplete, or missing
  aggregate-copy authority, do not synthesize a stack copy from route agreement.
- If the only evidence is Route 4, Route 5, Route 7, route-index facade status,
  or dump text, return unsupported route-only evidence.

This matches the current direction in `PreparedMirFunctionView::current_block_direct_edge_publication_sources`,
which rejects direct-edge publication sources when the destination home is not a
supported register home or the selected freshness does not have the
`DirectEdgePublicationSource` / `DirectEdgePublication` /
`DirectEdgePublicationMove` / `DirectEdgePublication` shape. Stack destination
authority should use the same pattern: expose status-rich prepared rows and
lower only `Available` rows.

## Handoff Summary

The stack view handoff should be prepared-owned. BIR semantic view records are
inputs and diagnostics; prealloc/prepared records are the authority; MIR is a
fail-closed consumer. The first follow-up should define a prepared stack
destination authority view over frame layout, value home, move bundle,
freshness, aggregate stack source, and branch stack-load evidence. It should
permit Route 4, Route 5, and Route 7 facts to feed the producer only through
named BIR views and should reject any route-only path as insufficient positive
producer evidence for ideas 647 or 655.
