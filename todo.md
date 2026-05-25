Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Remove Or Justify Call-Specific Dispatch Glue

# Current Packet

## Just Finished

Step 5 - Remove Or Justify Call-Specific Dispatch Glue moved the remaining
extractable call-boundary helper mechanics out of `dispatch.cpp` into
`calls_dispatch_bridge.cpp`/`.hpp`.

The final helper moved was
`retarget_fpr_call_result_store_value_to_emitted_scalar`; `dispatch.cpp` still
calls it at the same memory-store routing point after ordinary memory lowering,
so behavior is preserved.

Remaining call-specific code in `dispatch.cpp` is justified as routing:
dispatch decides when already-owned bridge helpers run around the call
instruction, materialized callee, materialized addresses, before-call moves,
missing frame-slot arguments, call emission, after-call moves, and call-result
recording. The source-selection mechanics remain in `calls_*` /
`calls_dispatch_bridge` owners. No AArch64-local call argument source-selection
logic was added or changed.

## Suggested Next

Move to Step 6 for boundary comments/documentation review and final validation.

## Watchouts

The moved FPR helper intentionally preserves the old behavior: it retargets
only named `f32`/`f64` `StoreLocal` values whose emitted scalar is a call ABI
FPR in the FP/SIMD bank and whose memory record already matches the store value
type. The remaining call routing is intentionally not moved until a shared
prepared call-argument source fact exists; do not add AArch64-local source
selection to force more consolidation.

## Proof

Passed. Backend proof log is `test_after.log`: 162/162 backend tests passed.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1
```
