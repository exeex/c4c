# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- widened semantic local `memset` beyond zero-only fills by lowering repeated-byte immediate fills over supported local integer leaf views, and proved the route with non-zero array and pair backend cases

## Suggested Next
- stay on plan item 3, but push the same semantic `memset` family into an honest remaining miss such as local subviews with non-zero base indices, or another runtime family with similarly shared lowering value

## Watchouts
- keep follow-on work inside shared semantic lowering under `BirFunctionLowerer`; do not reopen `call_decode.cpp`, `prepare`, or target-shaped handling
- the widened `memset` route still only accepts immediate repeated-byte fills over supported local integer leaf views; non-zero fills for pointer-bearing or padding-sensitive locals remain intentionally unsupported
- zero local `memset` keeps the broader zero-initializer coverage, including null pointers; the new non-zero support is narrower by design and should not be stretched into target- or testcase-shaped byte tricks

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=35 failed=0 total=35`
