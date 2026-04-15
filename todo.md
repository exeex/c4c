# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 4, tighten the semantic unsupported boundary

# Current Packet

## Just Finished
- tightened the step-4 module latest-failure selector around semantic-family wording without widening lowering behavior: the summary helper now ranks by the extracted failure family instead of brittle free-form substring matching
- added a nearby backend notes regression for `alloca local-memory semantic family`, proving a specific local-memory failure survives the later umbrella `local-memory semantic family` wrapper note in the module-level summary
- kept the slice on unsupported-boundary cleanup only; no call/runtime lowering or target legality behavior changed

## Suggested Next
- keep step 4 on planner-facing unsupported-boundary cleanup and audit whether the module-level admitted-buckets wording itself should be refreshed to mention function-signature and semantic-family-specific scalar/local-memory coverage more directly
- if another note-level regression is added, keep it adjacent to the summary contract rather than widening lowering, ABI, or inline-asm capability work

## Watchouts
- the summary helper still gives top precedence only to runtime/intrinsic and semantic-call families; any broader re-ranking should stay justified as unsupported-boundary wording work, not capability progress
- this packet remains note-contract hardening only; it does not expand local-memory lowering, inline-asm carriers, call ABI legality, or any other lowering surface

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed; the backend subset now includes the added `backend_lir_to_bir_notes` regression covering the specific `alloca local-memory semantic family` versus umbrella `local-memory semantic family` summary contract
- proof log preserved at `test_after.log`
