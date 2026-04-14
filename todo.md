# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- converted `builtin_memcpy_prefix_local_pair_to_i32_array` into the opposite mixed-offset direction and proved semantic local `memcpy` already handles a nested aggregate source into a non-zero-start destination array subview without escaping to `memcpy`

## Suggested Next
- stay on plan item 3, but move to a real remaining `memcpy` or runtime-family miss rather than more contiguous-prefix local coverage; the best next packet is an unsupported non-prefix local copy, padded mismatch, non-local base, or another intrinsic family such as `memset`

## Watchouts
- keep follow-on work inside shared semantic lowering under `BirFunctionLowerer`; do not reopen `call_decode.cpp`, `prepare`, or target-shaped handling
- the new reverse-direction wrapper case demonstrates that aggregate subobject tracking plus non-zero-start destination array views were already enough for `&src.inner -> &dst.dst[1]`; no `lir_to_bir_memory.cpp` change was required for this packet
- the current semantic local `memcpy` route still depends on repeated scalar leaf extents with matching stride and contiguous requested-prefix coverage
- non-zero-start suffix copies that do not present as contiguous scalar prefixes, padded mismatches, and non-local bases still fall out of this semantic route

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=33 failed=0 total=33`
