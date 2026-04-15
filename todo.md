# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 4, tighten the semantic unsupported boundary

# Current Packet

## Just Finished
- refreshed the step-4 module capability-bucket summary wording so it now names the admitted scalar/local-memory, semantic-call, and runtime/intrinsic family buckets more precisely without claiming new lowering behavior
- kept `backend_lir_to_bir_notes` focused on proving that refreshed module-summary contract alongside nearby latest-function-failure notes for inline-asm, direct-call, scalar-cast, and alloca failures
- kept the slice on planner-facing unsupported-boundary wording only; no call/runtime lowering, ABI legality, or note-ranking behavior changed

## Suggested Next
- keep step 4 on unsupported-boundary cleanup and audit whether any remaining planner-facing module notes still collapse specific semantic-call or runtime/intrinsic family buckets too aggressively
- if another notes regression is added, keep it adjacent to the module-summary contract instead of widening lowering, ABI, or runtime capability work

## Watchouts
- this packet only refreshes planner-facing module wording; it does not change what semantic families lower successfully
- the refreshed summary now names family buckets more specifically, so later note cleanup should avoid treating those examples as a promise that every nearby leaf family already lowers successfully

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_lir_to_bir_notes$" > test_after.log 2>&1'`
- passed; `backend_lir_to_bir_notes` now locks the refreshed module capability-bucket summary alongside the latest-function-failure note for each focused failure case
- proof log preserved at `test_after.log`
