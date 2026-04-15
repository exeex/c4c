# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- repaired semantic local `memset` lowering so exact leaf-aligned prefix fills no longer escape when the destination points into the middle of a repeated local array or nested aggregate extent
- added `backend_codegen_route_x86_64_builtin_memset_nonzero_prefix_nested_i32_subarray_field_observe_semantic_bir`, proving `memset(&dst.inner[1], 255, sizeof(int) * 2)` now stays in semantic BIR as two `bir.store_local ... i32 -1` updates while preserving the untouched prefix and suffix slots
- added `backend_codegen_route_x86_64_builtin_memset_nonzero_prefix_local_i32_subarray_observe_semantic_bir`, extending that proof to the plain local-array branch with `memset(&dst[1], 255, sizeof(int) * 2)` so both local-array and nested-aggregate subarray prefixes now stay on the shared semantic local-slot route

## Suggested Next
- decide whether the remaining honest `memset` work is a real semantic widening task or whether step 3 should move to the next uncovered runtime family such as `stacksave`/`stackrestore`, because the nearby element-aligned local-subarray prefix shapes are now covered on both the plain-array and nested-aggregate branches

## Watchouts
- keep `memset` widening leaf-aligned and semantic: this repair only accepts prefixes that exactly cover known local leaf slots, so byte-partial scalar writes should still fail honestly instead of being approximated
- the current proofs now cover both nested-aggregate and plain local-array nonzero subarray prefixes; any follow-on `memset` work should still stay in shared local-memory metadata rather than adding ad hoc call-family exceptions or target-specific lowering
- nearby packets should continue grouping runtime families (`memcpy`, `memset`, `va_*`, `stacksave`, `stackrestore`, `abs`) by semantic capability, not by isolated testcase names

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=49 failed=0 total=49`
- verified `backend_codegen_route_x86_64_builtin_memset_nonzero_prefix_local_i32_subarray_observe_semantic_bir` inside the passing run, proving `memset(&dst[1], 255, sizeof(int) * 2)` stays on the semantic BIR local-slot route without escaping to `@memset`
