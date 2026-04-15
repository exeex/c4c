# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 4, tighten the semantic unsupported boundary

# Current Packet

## Just Finished
- tightened step-4 note coverage around module failure summaries without widening lowering behavior: the direct backend notes test now proves both runtime and semantic-call failures stay visible in the module note even when later wrapper notes also fire
- added a nearby direct-call regression beside the existing inline-asm note case, proving `direct-call semantic family` survives into the module-level `latest function failure` summary instead of being replaced by `scalar/local-memory semantic family`

## Suggested Next
- keep step 4 on planner-facing unsupported-boundary cleanup and audit whether non-call/non-runtime semantic families still need the same summary-precedence treatment before considering any route reset or closure checkpoint
- if another note-level regression is added, keep it adjacent to the summary contract rather than widening inline asm, call ABI, or other backend capability work

## Watchouts
- the current module-summary helper still gives special precedence only to runtime/intrinsic and semantic-call failures; any follow-on broadening to other semantic families should be justified as unsupported-boundary work, not folded into capability claims
- this packet remains note-contract hardening only; it does not expand inline-asm carriers, call ABI legality, or any other lowering surface

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed; the backend subset now includes the expanded `backend_lir_to_bir_notes` regression covering both the inline-asm runtime-family case and the unknown-global direct-call semantic-family case
- proof log preserved at `test_after.log`
