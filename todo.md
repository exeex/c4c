# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 2, broaden semantic call lowering beyond the current baseline

# Current Packet

## Just Finished
- shifted execution focus back to step 2 and wired the uncovered `local_arg_call.c` plus `call_helper.c` cases into backend semantic-BIR route tests
- proved two common direct-call shapes already admitted by the semantic route: a local direct call with a loaded scalar argument and a direct call through an extern declaration
- kept the slice on semantic call proof only; no lowering behavior, ABI legality, or unsupported-boundary wording changed

## Suggested Next
- keep step 2 on direct-call proof/capability work and check whether another nearby direct-call shape still lacks semantic-BIR route coverage, especially one involving extern or aggregate-bearing arguments
- if step 2 proof is dense enough, switch the next packet to a real uncovered call/runtime lowering gap instead of adding more adjacent route tests

## Watchouts
- this packet only adds route proof for already-supported direct-call shapes; it does not widen semantic call lowering behavior on its own
- `tests/backend/CMakeLists.txt` is test-harness surface, so acceptance should keep at least one broader backend check in mind once a few more call-proof slices accumulate

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_codegen_route_x86_64_local_arg_call_observe_semantic_bir|backend_codegen_route_x86_64_call_helper_observe_semantic_bir)$" > test_after.log 2>&1'`
- passed; the targeted backend route tests now lock semantic-BIR output for both uncovered direct-call cases
- proof log preserved at `test_after.log`
