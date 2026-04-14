# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- kept the current contiguous-prefix local `memcpy` lowering intact, restored the legacy mixed local `memcpy` cases to their original shapes, and added dedicated backend route tests for the new prefix-capable wrapper routes so backend coverage increased additively

## Suggested Next
- stay on plan item 3 and extend mixed local `memcpy` lowering to non-zero-start subviews on both sides, preferably a suffix-shaped nested copy such as `&src.inner -> dst[2]` or `src[2] -> &dst.inner`

## Watchouts
- keep follow-on work inside shared semantic lowering under `BirFunctionLowerer`; do not reopen `call_decode.cpp`, `prepare`, or target-shaped handling
- the new helper only accepts contiguous requested prefixes with matching scalar leaf coverage; non-zero-start suffix copies, padded mismatches, and non-local bases still fall back out of this route
- the prefix-capable wrapper coverage now lives in additive `builtin_memcpy_prefix_local_*` route tests, while the pre-existing `builtin_memcpy_local_*` tests remain baseline same-size mixed copies
- this packet fixes proof monotonicity by increasing the backend subset from 30 passing tests to 32, without changing the lowering contract itself

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=32 failed=0 total=32`
