Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Remove Or Justify Call-Specific Dispatch Glue

# Current Packet

## Just Finished

Step 5 - Remove Or Justify Call-Specific Dispatch Glue continued by moving the
before-call retarget/materialized-address conflict helper cluster out of
`dispatch.cpp` into `calls_dispatch_bridge`.

`retarget_call_boundary_source_to_emitted_scalar`,
`source_value_is_materialized_address`, and
`source_register_conflicts_with_materialized_address` now live in
`calls_dispatch_bridge.cpp`/`.hpp`, with `dispatch.cpp` calling the bridge at
the same before-call routing points. No AArch64-local call argument
source-selection logic was added or changed.

## Suggested Next

Continue Step 5 by either moving or explicitly justifying the remaining
call-specific dispatch glue in `dispatch.cpp`. The next likely target is the
small before-call/after-call sequencing code that still decides when bridge
helpers are invoked around materialized callees, materialized addresses, and
missing frame-slot arguments.

## Watchouts

The moved conflict helper intentionally preserves the old behavior: a
materialized address source is not treated as a register conflict, while a
different source register that aliases an address materialization result is
emitted before the address materialization. Keep future extraction semantic and
do not add or change call argument source-selection behavior.

## Proof

Passed. Backend proof log is `test_after.log`: 162/162 backend tests passed.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1
```
