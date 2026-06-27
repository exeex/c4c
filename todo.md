# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm LocalFrameAddressMaterialization Boundary

## Just Finished

Lifecycle review kept idea 414 active because `FrameSlotAddress` and
`FrameSlotValue` are migrated but other route families remain. Regenerated
`plan.md` for the next selected route,
`LocalFrameAddressMaterialization`.

## Suggested Next

Begin Step 1 by confirming required producer fields, rejected cross-route
payloads, byval/aggregate coupling, and RV64/AArch64 consumer entry points.

## Watchouts

- Do not add optional fields to `PreparedCallArgumentSourceSelection`.
- Do not merge `LocalFrameAddressMaterialization` with `ByvalRegisterLane`
  unless lifecycle review creates a separate coupling plan.
- Preserve completed `FrameSlotAddress` and `FrameSlotValue` typed contracts.
- The previous broad validation before regeneration was 3356/3356 passing.

## Proof

Lifecycle-only plan regeneration. No build or CTest required for this packet.
Previous broad validation before regeneration:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure ) > test_after.log 2>&1`, 3356/3356 tests passed.
