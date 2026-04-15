# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 2, broaden semantic call lowering beyond the current baseline

# Current Packet

## Just Finished
- widened ordered step 2 coverage to the nearby nested global aggregate-array contained function-pointer family by proving `struct Outer g = {7, {{dec}, {inc}}}; return g.inner[1].fn(4);` stays on the same honest storage-to-indirect-call route through semantic BIR
- added `backend_codegen_route_x86_64_global_nested_aggregate_array_function_pointer_call_observe_semantic_bir` to prove the nested global aggregate-array member lowers as `bir.load_global ptr @g, offset 16` plus an indirect `bir.call`, extending the prior nested global aggregate checkpoint without reviving `call_decode.cpp` or taking on the broader local array-decay repair

## Suggested Next
- stay on ordered step 2 only if there is another nearby global-storage semantic-call family that still lowers through an honest aggregate-address-to-indirect-call path without widening into local member-array decay or unrelated pointer-constant handling
- leave step 4 note work parked unless a later call/runtime packet exposes a genuinely unbucketed unsupported path that the current semantic-family boundary no longer describes honestly

## Watchouts
- this slice is a coverage checkpoint for an already-supported nearby callee-provenance family; it does not widen generic pointer constants, repair local member-array decay, or change `call_decode.cpp` ownership
- the new route uses a fixed global byte offset (`offset 16`) for the second nested function-pointer field inside `struct Outer`; treat that as the honest-address result for this nested global aggregate-array case, not as a fallback escape

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed for this slice; `test_after.log` shows `100% tests passed, 0 tests failed out of 59`, including `backend_codegen_route_x86_64_global_nested_aggregate_array_function_pointer_call_observe_semantic_bir`
- proof log preserved at `test_after.log`
