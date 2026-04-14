# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- added backend route coverage for the reverse mixed local `memcpy` shape `struct Pair -> int[2]` and confirmed the existing ordered leaf-view lowering already handles the same-offset reverse direction without falling back to raw `memcpy`

## Suggested Next
- stay on plan item 3 and extend the adjacent runtime-memory lane to the next honest mixed local `memcpy` shape with differing base offsets, preferably a nested aggregate/array copy such as `&src.inner -> dst[2]` or `src[2] -> &dst.inner`

## Watchouts
- keep follow-on work inside shared semantic lowering under `BirFunctionLowerer`; do not reopen `call_decode.cpp`, `prepare`, or target-shaped handling
- the newly covered reverse mixed shape only proves the existing same-offset ordered leaf route; the remaining gap is still offset-mismatched nested copies, plus padded mismatches and non-local bases

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=30 failed=0 total=30`
