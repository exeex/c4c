# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 4, tighten the semantic unsupported boundary

# Current Packet

## Just Finished
- kept the refreshed step-4 module capability-bucket summary wording unchanged and extended `backend_lir_to_bir_notes` so it now also proves the adjacent indirect-call, call-return, memcpy runtime, and memset runtime family failures
- locked the explicit semantic-call and runtime/intrinsic family examples already named by the module summary without changing lowering behavior
- kept the slice on planner-facing unsupported-boundary wording only; no call/runtime lowering, ABI legality, or note-ranking behavior changed

## Suggested Next
- if step 4 stays active, check whether one final adjacent notes regression is still worth adding for the remaining scalar/local-memory leaf examples already named by the summary, such as scalar-binop or gep/load/store local-memory failures
- otherwise shift the next packet away from summary-note cleanup and back to a concrete semantic capability gap rather than another generic audit

## Watchouts
- this packet only extends planner-facing notes coverage; it does not change what semantic families lower successfully
- the module summary now has focused regression coverage for direct/indirect/call-return and memcpy/memset/inline-asm examples, so any further note cleanup should be justified by a concrete remaining boundary gap rather than another broad wording pass

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_lir_to_bir_notes$" > test_after.log 2>&1'`
- passed; `backend_lir_to_bir_notes` now locks the refreshed module capability-bucket summary alongside latest-function-failure notes for inline-asm, direct-call, indirect-call, call-return, memcpy, memset, scalar-cast, and alloca failure cases
- proof log preserved at `test_after.log`
