# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 2, broaden semantic call lowering beyond the current baseline

# Current Packet

## Just Finished
- widened ordered step 2 for one nearby indirect-call provenance shape by preserving direct function-symbol stores into tracked aggregate pointer-value slots, so aggregate-contained function pointers now survive into honest semantic BIR memory before the later indirect call
- added `backend_codegen_route_x86_64_aggregate_member_function_pointer_call_observe_semantic_bir` to prove an aggregate member function-pointer call now lowers as a stored `ptr @inc` plus an indirect `bir.call`, without reviving `call_decode.cpp` or adding testcase-shaped call ladders

## Suggested Next
- stay on ordered step 2 and check the next nearby callee-provenance family around aggregate-contained function pointers, such as array-held or global-aggregate-held function-pointer values, only if it can be proved as the same semantic storage-to-indirect-call route
- leave step 4 note work parked unless a later call/runtime packet exposes a genuinely unbucketed unsupported path that the current semantic-family boundary no longer describes honestly

## Watchouts
- this slice fixed tracked local pointer-value slots in the memory route so aggregate-backed call provenance reaches semantic BIR honestly; it did not widen generic pointer constants or change `call_decode.cpp` ownership
- the new route still shows the normal zero-initialization store before the later `ptr @inc` overwrite in `main`; that is acceptable and should not be mistaken for a remaining call-provenance failure

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=55 failed=0 total=55`
- delegated proof was sufficient for this packet because the diff stays inside semantic lowering plus one nearby backend-route test for the newly admitted aggregate-contained function-pointer call shape
