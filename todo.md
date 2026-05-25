Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Remove Or Justify Call-Specific Dispatch Glue

# Current Packet

## Just Finished

Step 5 - Remove Or Justify Call-Specific Dispatch Glue continued by moving
`retarget_fpr_call_result_store_value_to_emitted_scalar` out of `dispatch.cpp`
into `calls_dispatch_bridge.cpp`/`.hpp`.

`dispatch.cpp` still calls the helper at the same memory-store routing point
after ordinary memory lowering, so the FPR call-result store-value retargeting
behavior is preserved. No call argument source-selection logic was added or
changed.

## Suggested Next

Continue Step 5 by either moving or explicitly justifying the remaining
call-specific dispatch glue in `dispatch.cpp`. The remaining justification
target is the small sequencing code that decides when bridge helpers are invoked
around materialized callees, materialized addresses, missing frame-slot
arguments, and call-result recording.

## Watchouts

The moved FPR helper intentionally preserves the old behavior: it retargets
only named `f32`/`f64` `StoreLocal` values whose emitted scalar is a call ABI
FPR in the FP/SIMD bank and whose memory record already matches the store value
type. Keep future extraction semantic and do not add or change call argument
source-selection behavior.

## Proof

Passed. Backend proof log is `test_after.log`: 162/162 backend tests passed.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1
```
