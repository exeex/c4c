# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broaden Validation and Decide the Next Route

## Just Finished

Completed Step 5 broad validation for the migrated `FrameSlotValue` route.
Ran default CTest after the typed route payload, producer-side verifier, and
RV64/AArch64 consumer migration slices.

Validation result: 3356/3356 tests passed.

Next route candidate: `LocalFrameAddressMaterialization`. It builds on the
frame-slot address/storage authority already normalized for `FrameSlotAddress`
and `FrameSlotValue`, but it should remain a separate typed payload because it
carries pointer/base identity, byte-delta, materialization location, and byval
address-publication constraints.

## Suggested Next

Lifecycle review: the `FrameSlotValue` route runbook completed all five steps.
The plan owner should regenerate the active runbook for
`LocalFrameAddressMaterialization` or decide whether a separate idea is needed
for byval/local-address materialization coupling.

## Watchouts

- Do not add optional fields to `PreparedCallArgumentSourceSelection`.
- Do not disturb the completed `FrameSlotAddress` route except where a shared
  helper needs a behavior-preserving cleanup.
- `FrameSlotValue` should require source value/home identity plus source slot,
  stack offset, extent, and alignment.
- Keep `FrameSlotAddress`, `PriorPreservation`,
  `LocalFrameAddressMaterialization`, and `ByvalRegisterLane` route behavior
  unchanged unless the active step explicitly owns a shared boundary.
- Do not preserve the old typed-query behavior that accepts only slot or only
  stack offset; that compatibility should remain outside the typed API.
- `LocalFrameAddressMaterialization` overlaps byval aggregate transport; keep
  the route payload narrow and do not absorb `ByvalRegisterLane` unless the
  plan-owner split says to.
- `PriorPreservation` and `ByvalRegisterLane` remain unmigrated route families.

## Proof

Passed broad validation in `test_after.log`:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure ) > test_after.log 2>&1`

Result: 3356/3356 tests passed.
