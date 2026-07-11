# Current Packet

Status: Active
Source Idea Path: ideas/open/717_current_block_routed_value_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Prove join-transfer destination consistency

## Just Finished

- Step 2 added and registered
  `backend_prealloc_join_transfer_destination_consistency_test.cpp`.
- The focused probe proves that transfer result, edge-transfer destination, and
  publication destination must match, and that every mismatch or missing
  authority link fails closed.
- The existing prepared join-transfer completeness path now uses the same
  generic destination-consistency predicate; no routed-operand or AArch64
  behavior or expectations changed.

## Suggested Next

- Execute the bounded Step 3 routed-operand authority packet and keep it
  independent of predecessor/all-edge invariance and AArch64 consumption.

## Watchouts

- Destination consistency is now a separate proven seam; Step 3 must not fold
  predecessor or parallel-edge invariance into routed-operand authority.
- Keep AArch64 integration tests unchanged until the planned consumer step.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: 318/318 backend tests passed, including
  `backend_prealloc_join_transfer_destination_consistency`.
- Canonical proof log: `test_after.log`.
