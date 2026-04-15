# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- added `backend_codegen_route_x86_64_builtin_memset_nonzero_nested_i32_scalar_field_observe_semantic_bir`, proving `memset(&dst.inner.value, 255, sizeof(dst.inner.value))` stays in semantic BIR as a direct `bir.store_local %lv.dst.4, i32 -1` update with the sibling field preserved
- confirmed this nested aggregate scalar-field pointer shape was already handled by the shared local-slot metadata route, so the packet locked in the nearby semantic family with dedicated backend coverage instead of adding a new `memset` special case

## Suggested Next
- move step 3 to the next uncovered runtime-family boundary instead of extending `memset` by inventory; the nearby nested scalar-field pointer shape is now covered on the same shared local-memory route as local scalars, arrays, and nested array fields

## Watchouts
- this packet found no new lowering gap: nested aggregate scalar-field pointers already reuse the same local pointer-slot metadata as the earlier local scalar case, so follow-on runtime work should avoid reopening `memset` unless a genuinely shared semantic miss appears
- keep `memset` support leaf-aligned and semantic: exact single-slot fills are fine, but byte-partial writes or multi-slot spans should still fail honestly instead of being approximated
- nearby packets should continue grouping runtime families (`memcpy`, `memset`, `va_*`, `stacksave`, `stackrestore`, `abs`) by semantic capability, not by isolated testcase names

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=51 failed=0 total=51`
- verified `backend_codegen_route_x86_64_builtin_memset_nonzero_nested_i32_scalar_field_observe_semantic_bir` inside the passing run, proving `memset(&dst.inner.value, 255, sizeof(dst.inner.value))` stays on the semantic BIR local-slot route without escaping to `@memset`
