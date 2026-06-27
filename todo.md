# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broaden Validation and Decide the Next Route

## Just Finished

Completed Step 5 broad validation for
`LocalFrameAddressMaterialization`.

Build plus default CTest passed after the typed payload, verifier, and consumer
migration slices.

Broad validation exposed stale hand-authored AArch64 instruction-dispatch
fixtures that omitted local-materialization pointer-delta and materialization
payload facts. Those fixtures now publish coherent typed route facts.

## Suggested Next

Request lifecycle review/closure for the active
`LocalFrameAddressMaterialization` runbook.

## Watchouts

- Do not add optional fields to `PreparedCallArgumentSourceSelection`.
- Do not merge `LocalFrameAddressMaterialization` with `ByvalRegisterLane`
  unless lifecycle review creates a separate coupling plan.
- Preserve completed `FrameSlotAddress` and `FrameSlotValue` typed contracts.
- The previous broad validation before regeneration was 3356/3356 passing.
- Keep byval aggregate transport as a consumer of the local-materialization
  route; do not absorb byval register-lane payloads into this typed route.
- RV64/AArch64 consumers still read this route through the compatibility bag;
  Step 4 owns that migration after verifier coverage exists.

## Proof

Broad proof passed: 3356/3356 tests, with monotonic regression guard PASS
against the accepted 3356/3356 full-suite baseline. Proof command:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure ) > test_after.log 2>&1`
