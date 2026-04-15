# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 4, tighten the semantic unsupported boundary

# Current Packet

## Just Finished
- refreshed the step-4 module capability-bucket summary wording so it names current function-signature plus scalar/local-memory coverage directly without claiming new lowering behavior
- tightened the adjacent `backend_lir_to_bir_notes` regression so each failure case now proves both the refreshed module-summary contract and the carried latest-function-failure note
- kept the slice on planner-facing unsupported-boundary wording only; no call/runtime lowering, ABI legality, or note-ranking behavior changed

## Suggested Next
- keep step 4 on unsupported-boundary cleanup and audit whether any other planner-facing module notes still describe admitted semantic families too generically compared with the backend's current family names
- if another notes regression is added, keep it adjacent to the module-summary contract instead of widening lowering, ABI, or runtime capability work

## Watchouts
- this packet only refreshes planner-facing module wording and its adjacent regression; it does not change what semantic families lower successfully
- any later note cleanup should avoid smuggling new family precedence or capability claims into the module summary text

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_lir_to_bir_notes$" > test_after.log 2>&1'`
- passed; `backend_lir_to_bir_notes` now locks the refreshed module capability-bucket summary alongside the latest-function-failure note for each focused failure case
- proof log preserved at `test_after.log`
