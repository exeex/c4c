# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- finished the semantic local `memset` subview slice by aligning the new nested non-zero backend route expectation with the generated semantic BIR and reproving the route without falling back to raw `memset`

## Suggested Next
- stay on plan item 3 and move to the next shared semantic runtime-family miss, preferably another honest local view or intrinsic/runtime lowering gap that can reuse existing `BirFunctionLowerer` metadata instead of widening fallback handling

## Watchouts
- keep follow-on work inside shared semantic lowering under `BirFunctionLowerer`; do not reopen `call_decode.cpp`, `prepare`, or target-shaped handling
- the widened `memset` route now matches the same honest local array/pointer subview family already accepted by semantic `memcpy`, but it still only accepts repeated-byte fills over supported integer leaf slots; pointer-bearing or padding-sensitive locals remain intentionally unsupported
- zero local `memset` keeps the broader zero-initializer coverage, including null pointers; the new non-zero support is narrower by design and should not be stretched into target- or testcase-shaped byte tricks

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=46 failed=0 total=46`
