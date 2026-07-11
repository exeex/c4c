# Current Packet

Status: Active
Source Idea Path: ideas/open/713_current_block_edge_bound_routing_consumption_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Correct and prove bounded AArch64 consumption

## Just Finished

- Step 4 review rejected the uncommitted route because AArch64 can still invoke
  a function-wide fallback builder and supported integration expectations were
  downgraded. Step 4 remains active for correction.

## Suggested Next

- Attach precomputed current-block routing facts at the
  prealloc/function-context owner boundary before AArch64 consumption.
- Delete the AArch64-triggered fallback builder and fail closed when owner-owned
  lookups are absent.
- Restore the existing supported integration expectations, then run the focused
  probes, restored integration test, and fresh broader backend proof.

## Watchouts

- Do not call `make_prepared_function_lookups` or any equivalent function-wide
  reconstruction from AArch64.
- Do not weaken, reclassify, or rewrite supported integration expectations to
  obtain green proof.
- Route 5 compatibility payload retirement remains outside this Step 4 packet.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Pending: rerun after the owner-boundary attachment, fallback deletion, and
  integration-expectation restoration. Prior green proof does not accept the
  reviewer-identified route-quality failures.
