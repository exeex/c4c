# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 2, broaden semantic call lowering beyond the current baseline

# Current Packet

## Just Finished
- widened ordered step 2 coverage to the next nearby aggregate-backed callee-provenance family by proving that a constant-index global aggregate-array function-pointer value stays on the same honest storage-to-indirect-call route through semantic BIR
- added `backend_codegen_route_x86_64_global_aggregate_array_function_pointer_call_observe_semantic_bir` to prove `struct Holder gs[2] = {{dec}, {inc}}; return gs[1].fn(4);` lowers as `bir.load_global ptr @gs, offset 8` plus an indirect `bir.call`, extending the prior single-global-holder checkpoint to the next nearby global aggregate-array storage family without reviving `call_decode.cpp`

## Suggested Next
- stay on ordered step 2 and check a nearby nested global aggregate-contained function-pointer family only if it still lowers as the same semantic storage-to-indirect-call route without taking on the broader local array-decay repair that still affects scalar and function-pointer member arrays
- leave step 4 note work parked unless a later call/runtime packet exposes a genuinely unbucketed unsupported path that the current semantic-family boundary no longer describes honestly

## Watchouts
- this slice is a coverage checkpoint for an already-supported nearby callee-provenance family; it does not widen generic pointer constants, repair local member-array decay, or change `call_decode.cpp` ownership
- the new route uses a fixed global byte offset (`offset 8`) for the selected array element's function-pointer field; that should be treated as the expected honest-address result for this constant-index global aggregate-array case, not as a fallback escape

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with canonical regression guard on `test_before.log` vs `test_after.log`; result is `before: passed=56 failed=0 total=56`, `after: passed=57 failed=0 total=57`, `delta: passed=1 failed=0`
- accepted baseline rolled forward by replacing `test_before.log` with the passing `test_after.log`
