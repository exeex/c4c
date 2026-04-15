# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 2, broaden semantic call lowering beyond the current baseline

# Current Packet

## Just Finished
- widened ordered step 2 coverage to the next nearby nested global aggregate-contained function-pointer family by proving `struct Outer g = {7, {inc}}; return g.inner.fn(4);` stays on the same honest storage-to-indirect-call route through semantic BIR
- added `backend_codegen_route_x86_64_global_nested_aggregate_function_pointer_call_observe_semantic_bir` to prove the nested global aggregate field lowers as `bir.load_global ptr @g, offset 8` plus an indirect `bir.call`, extending the prior single-global-holder and global aggregate-array checkpoints without reviving `call_decode.cpp` or taking on local array-decay repair

## Suggested Next
- stay on ordered step 2 and check a nearby nested global aggregate-array contained function-pointer family only if it still lowers as the same semantic storage-to-indirect-call route without taking on the broader local array-decay repair that still affects scalar and function-pointer member arrays
- leave step 4 note work parked unless a later call/runtime packet exposes a genuinely unbucketed unsupported path that the current semantic-family boundary no longer describes honestly

## Watchouts
- this slice is a coverage checkpoint for an already-supported nearby callee-provenance family; it does not widen generic pointer constants, repair local member-array decay, or change `call_decode.cpp` ownership
- the new route uses a fixed global byte offset (`offset 8`) for the nested function-pointer field inside `struct Outer`; treat that as the honest-address result for this nested global aggregate case, not as a fallback escape

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with canonical regression guard on `test_before.log` vs `test_after.log`; result is `before: passed=57 failed=0 total=57`, `after: passed=58 failed=0 total=58`, `delta: passed=1 failed=0`
- accepted baseline rolled forward by replacing `test_before.log` with the passing `test_after.log`
