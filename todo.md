# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- extended semantic local `memcpy` lowering so aggregate-backed scalar subviews recovered through local pointer-slot metadata can still resolve as contiguous local array views, and added additive backend coverage for a mixed local non-zero-start source subarray to nested destination field route

## Suggested Next
- stay on plan item 3 and extend mixed local `memcpy` lowering to the opposite mixed-offset direction, preferably a nested aggregate source into a non-zero-start destination array subview such as `&src.inner -> dst[2]`

## Watchouts
- keep follow-on work inside shared semantic lowering under `BirFunctionLowerer`; do not reopen `call_decode.cpp`, `prepare`, or target-shaped handling
- the widened array-view recovery now covers aggregate-backed scalar subviews reached through `local_pointer_slots`, but it still depends on repeated scalar leaf extents with matching stride and contiguous requested-prefix coverage
- the additive wrapper coverage now includes `builtin_memcpy_prefix_local_i32_subarray_to_nested_pair_field`, while the pre-existing mixed local tests remain baseline same-size copies and the earlier `prefix_local_*` tests still cover one-sided non-zero-start wrappers
- non-zero-start suffix copies that do not present as contiguous scalar prefixes, padded mismatches, and non-local bases still fall out of this semantic route

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=33 failed=0 total=33`
