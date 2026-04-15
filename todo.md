# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- widened semantic local `memset` lowering so exact local scalar-slot fills no longer escape when the destination is a direct pointer to one local leaf slot rather than an aggregate or array view
- added `backend_codegen_route_x86_64_builtin_memset_nonzero_local_i32_scalar_observe_semantic_bir`, proving `memset(&dst, 255, sizeof(dst))` now stays in semantic BIR as a direct `bir.store_local %lv.dst, i32 -1` update without falling back to `@memset`
- kept the widening on the shared local-memory route by resolving scalar pointer destinations from existing local slot metadata instead of adding a call-family-specific exception

## Suggested Next
- decide whether step 3 still needs another honest `memset` widening packet, such as exact scalar leaf fills reached through nested aggregate member pointers, or whether the route should move on to the next uncovered runtime-family boundary instead of extending `memset` by testcase inventory

## Watchouts
- keep `memset` widening leaf-aligned and semantic: this repair only accepts exact fills of one known local scalar slot, so byte-partial writes or multi-slot spans should still fail honestly instead of being approximated
- the scalar-slot path reuses existing local pointer-slot metadata, so any follow-on `memset` work should continue generalizing through shared local-memory metadata rather than through new direct-call special cases
- nearby packets should continue grouping runtime families (`memcpy`, `memset`, `va_*`, `stacksave`, `stackrestore`, `abs`) by semantic capability, not by isolated testcase names

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=50 failed=0 total=50`
- verified `backend_codegen_route_x86_64_builtin_memset_nonzero_local_i32_scalar_observe_semantic_bir` inside the passing run, proving `memset(&dst, 255, sizeof(dst))` stays on the semantic BIR local-slot route without escaping to `@memset`
