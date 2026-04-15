# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 2, broaden semantic call lowering beyond the current baseline

# Current Packet

## Just Finished
- widened ordered step 2 coverage to the next nearby aggregate-backed callee-provenance family by proving that a global aggregate-held function-pointer value already stays on the same honest storage-to-indirect-call route through semantic BIR
- added `backend_codegen_route_x86_64_global_aggregate_function_pointer_call_observe_semantic_bir` to prove a global `struct Holder { int (*fn)(int); }` lowers as `bir.load_global ptr @g` plus an indirect `bir.call`, extending the earlier local aggregate-member function-pointer proof to a nearby same-shape global storage family without reviving `call_decode.cpp`

## Suggested Next
- stay on ordered step 2 and check the next nearby callee-provenance family around aggregate-contained function pointers only if it can be proved as the same semantic storage-to-indirect-call route without taking on the broader local array-decay repair that still affects scalar and function-pointer member arrays
- leave step 4 note work parked unless a later call/runtime packet exposes a genuinely unbucketed unsupported path that the current semantic-family boundary no longer describes honestly

## Watchouts
- this slice is a coverage checkpoint for an already-supported nearby callee-provenance family; it does not widen generic pointer constants, repair local member-array decay, or change `call_decode.cpp` ownership
- the new route still shows the normal zero-initialization store before the later `ptr @inc` overwrite in `main`; that is acceptable and should not be mistaken for a remaining call-provenance failure

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with canonical regression guard on `test_before.log` vs `test_after.log`; result is `before: passed=55 failed=0 total=55`, `after: passed=56 failed=0 total=56`, `delta: passed=1 failed=0`
- accepted baseline rolled forward by replacing `test_before.log` with the passing `test_after.log`
