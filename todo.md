Status: Active
Source Idea Path: ideas/open/67_aarch64_local_slot_address_offset_probe.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Classify The Path Outcome

# Current Packet

## Just Finished

Step 3 completed: classified `local_slot_address_frame_offset` using the Step 1
static caller inventory and Step 2 runtime probe evidence.

Classification: dead/unreachable under current callers.
- Step 1 static evidence found `local_slot_address_frame_offset` only at its
  public declaration in `dispatch_publication.hpp` and null definition in
  `dispatch_publication.cpp`; no production or test caller references it.
- Step 1 also found the adjacent `local_aggregate_address_frame_offset` helper
  is declaration/definition only; live aggregate address behavior is routed
  through prepared `LocalFrameAddressMaterialization` call source selection.
- Step 2 runtime evidence proved current local-slot pointer `Add` lowering can
  publish the adjusted frame address through prepared frame-address
  materialization lookups and `lower_local_slot_address_publication`, printing
  both the base address and adjusted address without using the null helper.
- Step 2 negative evidence removed prepared frame-address materialization facts
  and confirmed the route does not rederive the adjusted address from
  local-slot identity.
- A fresh Step 3 scan again found `local_slot_address_frame_offset` only at its
  declaration/definition; the only new related hits are the Step 2 probe names.

This is a classification result, not capability progress. The supported local
slot pointer Add/Sub path already runs through prepared frame-address
materialization authority; no missing prepared offset coverage was exposed by
the probe.

## Suggested Next

Step 4 - Apply Only The Classification-Sized Follow-Up: delete the unreachable
`local_slot_address_frame_offset` API surface if the supervisor delegates that
cleanup. Suggested scope is only the declaration in `dispatch_publication.hpp`
and the null definition in `dispatch_publication.cpp`, with route scans and
focused AArch64 proof. Do not delete or reshape adjacent prepared
frame-address materialization, local aggregate address publication, or
call-source-selection behavior.

## Watchouts

Do not broaden the classification follow-up into shared frame/address
authority. Keep the Step 2 runtime probe because it is the evidence that
current callers use prepared frame-address materialization and fail closed
without it. If Step 4 deletes the dead helper, preserve
`local_aggregate_address_frame_offset` unless separately delegated and proven,
even though it showed the same declaration/definition-only shape in this scan.

## Proof

Read-only classification proof:
- Re-read the Step 2 todo evidence and current source/test scan state.
- `rg -n "local_slot_address_frame_offset\\(|local_aggregate_address_frame_offset\\(|local_slot_address_offset_probe_uses_prepared_frame_materialization|prepared_with_local_slot_address_offset_probe" src tests -g '!review/**'`
  confirmed `local_slot_address_frame_offset` remains declaration/definition
  only, `local_aggregate_address_frame_offset` remains declaration/definition
  only, and the only runtime probe hits are the Step 2 test helper and call.

`git diff --check` passed. No build/test proof was required for this
classification-only packet.
