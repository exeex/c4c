# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`tests/backend/backend_x86_handoff_boundary_test.cpp` by extending
`check_join_route_consumes_prepared_control_flow` to the trailing join-`ashr`
family, so the compare-join route is now proven to keep following prepared
true/false transfer ownership after one trailing arithmetic-right-shift use of
the joined value instead of stopping at the base, trailing-`add`,
trailing-`xor`, trailing-`and`, trailing-`or`, trailing-`mul`,
trailing-`shl`, and trailing-`lshr` cases.

## Suggested Next

The next small Step 3 packet is broadening the same compare-join ownership
proof beyond the current arithmetic and shift follow-on families to another
prepared join consumer that still relies on the same generalized helper path,
so the label-based fallback can keep shrinking without introducing x86
testcase-shaped ownership logic.

## Watchouts

- Do not activate umbrella idea 57 as executable work.
- Do not pull in idea 59 instruction-selection scope.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth.
- Keep the compare-join lane aligned with the continuation lane: only empty
  branch-only carriers on the way to the prepared join are in scope.
- Shared lowering now publishes ordinary compare-join true/false transfer
  ownership for the base phi-lowered lane, but x86 still keeps a label-based
  fallback for routes that do not yet expose those indices; remove that
  fallback only when the remaining families are covered by the same contract.
- The trailing-`ashr` ownership proof reuses the same helper and mutation path
  as the base, trailing-`add`, trailing-`xor`, trailing-`and`,
  trailing-`or`, trailing-`mul`, trailing-`shl`, and trailing-`lshr` cases;
  keep future families on that generalized route instead of cloning
  testcase-shaped ownership logic.
- Treat any future fix here as capability repair, not expectation weakening:
  the joined branch route is covered by `backend_x86_handoff_boundary`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The build and narrow proof both passed; `test_after.log` is the canonical proof
log and was sufficient for this handoff-boundary Step 3 ownership packet.
