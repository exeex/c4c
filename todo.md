Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Remove Or Justify Call-Specific Dispatch Glue

# Current Packet

## Just Finished

Step 5 - Remove Or Justify Call-Specific Dispatch Glue started by moving simple
call-boundary scalar-state helper ownership out of `dispatch.cpp` into
`calls_dispatch_bridge`.

`record_call_boundary_destination`,
`record_call_boundary_source_in_destination`, and
`call_boundary_move_reloads_prepared_stack_source` now live in
`calls_dispatch_bridge.cpp`/`.hpp` and are called from the same dispatch routing
points. No AArch64-local call argument source-selection logic changed.

## Suggested Next

Continue Step 5 by either moving or explicitly justifying the remaining
call-specific dispatch glue in `dispatch.cpp`, with the next likely target being
the call-boundary retarget/materialized-address conflict helper cluster.

## Watchouts

The remaining retargeting helpers still read `scalar_state`, address
materialization records, and ABI register details directly in `dispatch.cpp`.
Keep future extraction semantic and do not add or change call argument
source-selection behavior.

## Proof

Passed. Backend proof log is `test_after.log`: 162/162 backend tests passed.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1
```
