# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: step-4 unsupported-boundary cleanup after the broader backend checkpoint

# Current Packet

## Just Finished
- closed the remaining step-4 umbrella-family note gap in `backend_lir_to_bir_notes` by adding explicit failure-note coverage for `function-signature`, `scalar-control-flow`, `local-memory`, and `scalar/local-memory` failures already emitted by `lir_to_bir_module.cpp`

## Suggested Next
- inspect whether any semantic unsupported-boundary contract gap remains beyond the now-covered narrow and umbrella family notes, and if not, prepare close-routing evidence for step 4 rather than expanding the proof surface

## Watchouts
- keep step-4 work on semantic-family wording and failure buckets, not testcase inventory churn or new observer probes
- do not reopen `tests/backend/CMakeLists.txt` unless a real unsupported-boundary gap appears outside the refreshed notes contract
- keep new work under `BirFunctionLowerer` and the split `lir_to_bir_*` owners; do not revive `call_decode.cpp` or push ABI legality back into semantic lowering

## Proof
- executor packet proof: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- proof passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'` -> `test_after.log`
