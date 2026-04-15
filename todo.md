# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: step-4 unsupported-boundary cleanup after the broader backend checkpoint

# Current Packet

## Just Finished
- accepted a fresh `^backend_` checkpoint from a clean build and rolled it forward into `test_before.log`
- tightened the semantic unsupported-boundary transcript so the module capability summary now names the admitted runtime/intrinsic families already on this route: variadic, stack-state, absolute-value, memcpy, memset, and inline-asm placeholders
- extended `backend_lir_to_bir_notes` coverage with explicit failure-note checks for `variadic runtime family`, `stack-state runtime family`, and `absolute-value intrinsic family`

## Suggested Next
- inspect the remaining step-4 unsupported surface for any semantic family that is still implemented in lowering but missing explicit failure-bucket coverage or planner-facing wording
- prefer a bounded note-contract cleanup packet over adding more observer proofs unless a real unsupported family gap turns up in code or tests

## Watchouts
- keep step-4 packets on semantic-family wording and failure buckets, not testcase inventory churn
- do not reopen `tests/backend/CMakeLists.txt` proof expansion unless a real lowering gap appears beyond the now-refreshed module-note coverage
- keep new work under `BirFunctionLowerer` and the split `lir_to_bir_*` owners; do not revive `call_decode.cpp` or push ABI legality back into semantic lowering

## Proof
- supervisor broader checkpoint: `cmake --preset default && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'` passed and was rolled forward to `test_before.log`
- executor packet proof: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- executor packet wrote `test_after.log`, and the accepted baseline now lives at `test_before.log`
