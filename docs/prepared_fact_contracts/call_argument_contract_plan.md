# Prepared Call Argument Contract Plan

Status: Published for idea 414 Step 4
Source Idea: `ideas/open/414_typed_prepared_call_argument_contracts.md`

This document records the typed prepared call-argument route contract as it is
migrated route by route. Step 2 starts with the `FrameSlotAddress` source route
and keeps `PreparedCallArgumentSourceSelection` as a compatibility boundary
while typed accessors become the consumer-facing API.

## Consumed Taxonomy Rows

- `TAX-FAM-CALL-ARG-TYPED-ROUTES-PLACEHOLDER-001`: owns the typed
  call-argument source route boundary. Missing route facts are
  `producer_missing`; contradictory route payloads are `producer_incoherent`.
- `TAX-FAM-CALL-BOUNDARY-ARG-RESULT-001`: owns the call argument plan, ABI
  binding, source/destination shape, and move-bundle context that decides
  whether a call argument route is required.
- Call-route portions of `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001`: own
  publication rows consumed when the selected source route needs target-visible
  frame-slot publication.
- Storage/home portions of `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`: own the
  prepared source home, frame-slot identity, byte offset, extent, and alignment
  that the call route references.

## Consumed Audit Rows

- `418-AUD-RV64-FRAME-SLOT-CALL-RECOVERY-001`: RV64 frame-slot address
  argument materialization must consume the selected prepared route and stack
  offset instead of reconstructing the route from nearby materializations.
- `418-AUD-A64-FRAME-SLOT-PUBLICATION-REVIEW-001`: the AArch64 missing
  frame-slot publication bridge must distinguish optional non-applicable
  arguments from required-but-missing or incoherent `FrameSlotAddress` facts.
- Reference rows `418-AUD-RV64-CALL-COHERENT-001` and
  `418-AUD-A64-CALL-COHERENT-001` remain examples for complete call-plan
  consumption. `418-AUD-RV64-BYVAL-COHERENT-001` is a later-route reference for
  aggregate/byval transport and is not part of the Step 2 payload.

## Step 2 Route Scope

The first migrated payload is the `FrameSlotAddress` source route. Its typed
view is `PreparedCallArgumentFrameSlotAddressRoute`, exposed through
`as_frame_slot_address_source_route`.

Required route facts:

- selected source frame-slot id
- selected source stack byte offset
- selected source byte extent
- selected source alignment

Optional route facts:

- source value id, value name, and stack-slot home kind when the producer has a
  value-home record for the selected source
- address materialization block label, instruction index, materialized
  frame-slot id, and byte offset when the route came from an explicit prepared
  frame-slot address materialization

Rejected compatibility-bag combinations:

- missing required slot, byte offset, extent, or alignment
- partial address materialization payloads
- materialization frame-slot id that contradicts the selected source slot id
- preservation, byval-lane, source-base, or pointer-delta payload mixed into
  the `FrameSlotAddress` route
- non-stack source-home kind on a frame-slot address route

The compatibility bridge keeps old producers writing
`PreparedCallArgumentSourceSelection`, but typed consumers must query the route
through `as_frame_slot_address_source_route`. Invalid old optional-field
combinations are therefore absent from the typed API and from shared
frame-slot publication classification.

## Step 3 Verification

Step 3 adds `verify_prepared_frame_slot_address_source_route_contract` as the
producer-side report surface for the same route boundary. The verifier reports
`PreparedContractFactFamily::CallArgumentTypedRoute`.

Producer-missing statuses:

- missing selected `FrameSlotAddress` route
- missing source frame-slot id
- missing source stack byte offset
- missing source extent
- missing source alignment

Producer-incoherent statuses:

- non-stack source-home kind
- partial address materialization payload
- materialization frame-slot id contradicts the selected source slot id
- preservation, byval-lane, source-base, or pointer-delta payload mixed into
  the frame-slot address route

These diagnostics are intentionally producer-owned. RV64 and AArch64 target
work should consume the verifier report at required-route fail-closed sites
instead of treating an absent typed view as permission to reconstruct the route.

## Step 4 Consumer Migration

Step 4 migrates the first RV64/AArch64 `FrameSlotAddress` consumers to the
typed route boundary.

- RV64 `emit_riscv_frame_slot_address_argument` no longer falls back to scanning
  indexed prepared address materializations when no source selection is present.
  Its selected frame-slot address helper verifies
  `PreparedCallArgumentFrameSlotAddressRoute` before reading route facts.
- AArch64 `StackFrameSlotCallOperandOwner::selected_frame_slot_source` verifies
  and consumes `PreparedCallArgumentFrameSlotAddressRoute` for
  `FrameSlotAddress`; the old optional-bag path remains only for unmigrated
  `FrameSlotValue` route handling.

The selected consumers now treat an absent or incoherent typed route as a
fail-closed prepared-producer issue, not as permission to recover stack offsets
or materialization identity locally.

## Next Contract Step

Step 5 should broaden validation and decide whether the next route should be
`FrameSlotValue`, `LocalFrameAddressMaterialization`, `PriorPreservation`, or
`ByvalRegisterLane`.
