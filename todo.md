# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm FrameSlotValue Route Boundary

## Just Finished

Lifecycle review kept idea 414 active because the completed
`FrameSlotAddress` runbook migrated only the first route family. Regenerated
`plan.md` for the next selected route, `FrameSlotValue`, using the same typed
payload -> verifier -> consumer -> broad-validation ladder.

## Suggested Next

Begin Step 1 by confirming the exact `FrameSlotValue` producer fields, rejected
cross-route payloads, and RV64/AArch64 consumer entry points. Record the
inventory in this file before Step 2 implementation.

## Watchouts

- Do not add optional fields to `PreparedCallArgumentSourceSelection`.
- Do not disturb the completed `FrameSlotAddress` route except where a shared
  helper needs a behavior-preserving cleanup.
- `FrameSlotValue` should require source value/home identity plus source slot,
  stack offset, extent, and alignment.
- Keep `FrameSlotAddress`, `PriorPreservation`,
  `LocalFrameAddressMaterialization`, and `ByvalRegisterLane` route behavior
  unchanged unless the active step explicitly owns a shared boundary.

## Proof

Lifecycle-only plan regeneration. No build or CTest required for this packet.
Previous broad validation before regeneration: `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure ) > test_after.log 2>&1`, 3356/3356 tests passed.
