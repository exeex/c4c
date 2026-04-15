# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- repaired semantic local `memset` lowering so exact leaf-aligned prefix fills no longer escape when the destination points into the middle of a repeated local array or nested aggregate extent
- added `backend_codegen_route_x86_64_builtin_memset_nonzero_prefix_nested_i32_subarray_field_observe_semantic_bir`, proving `memset(&dst.inner[1], 255, sizeof(int) * 2)` now stays in semantic BIR as two `bir.store_local ... i32 -1` updates while preserving the untouched prefix and suffix slots

## Suggested Next
- stay on the `memset` runtime family long enough to prove one nearby partial-range shape beyond the nested local-array field case, or move to the next uncovered step-3 family only if that broader `memset` route now looks exhausted

## Watchouts
- keep `memset` widening leaf-aligned and semantic: this repair only accepts prefixes that exactly cover known local leaf slots, so byte-partial scalar writes should still fail honestly instead of being approximated
- the new proof covers a nested aggregate field with a nonzero array base offset; follow-on shapes should stay in shared local-memory metadata rather than adding ad hoc call-family exceptions or target-specific lowering
- nearby packets should continue grouping runtime families (`memcpy`, `memset`, `va_*`, `stacksave`, `stackrestore`, `abs`) by semantic capability, not by isolated testcase names

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=48 failed=0 total=48`
- verified `backend_codegen_route_x86_64_builtin_memset_nonzero_prefix_nested_i32_subarray_field_observe_semantic_bir` inside the passing run, proving the nonzero `memset` subarray fill stays on the semantic BIR local-slot route instead of escaping to `@memset`
