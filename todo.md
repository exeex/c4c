# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

## Current Packet

Just Finished:
- generalized direct local `memcpy` lowering around shared ordered leaf views so matching local aggregate slots and local scalar-array views can copy through the same semantic route
- proved the widened route with `builtin_memcpy_local_i32_array_to_pair.c` without falling back to raw `memcpy`

Suggested Next:
- stay on plan item 3 and extend the adjacent runtime-memory lane to the next honest mixed local `memcpy` shape, preferably reverse or nested mixed aggregate/array copies that should share the same ordered leaf-view route

Watchouts:
- keep follow-on work inside shared semantic lowering under `BirFunctionLowerer`; do not reopen `call_decode.cpp`, `prepare`, or target-shaped handling
- the current mixed local `memcpy` route still assumes matching ordered leaf offsets, scalar types, and exact byte size; padded mismatches and non-local bases remain outside the supported lane

Proof:
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- latest accepted comparison passed with `before: passed=28 failed=0 total=28` and `after: passed=29 failed=0 total=29`
