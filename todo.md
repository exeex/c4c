# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 4, tighten the semantic unsupported boundary

# Current Packet

## Just Finished
- tightened the module-level `lir_to_bir` success/failure notes so they now describe semantic lowering in stable capability buckets instead of enumerating a coarse evolving feature list
- kept the step-4 cleanup in `src/backend/lowering/lir_to_bir_module.cpp` only, avoiding any testcase-shaped matcher growth or new semantic-lowering shortcuts just to manufacture more runtime-boundary churn

## Suggested Next
- if new semantic-BIR misses appear, continue step 4 by replacing any remaining stale feature-checklist notes with capability-bucket language that matches the actual admitted route
- keep any future boundary cleanup honest about the current contract: semantic lowering can admit the supported capability bucket, or it should fail with a stable semantic-family description instead of restating testcase history

## Watchouts
- this packet only fixes the planner-facing lowering notes; it does not add new semantic capability and must not be treated as evidence that later prepare/legalize or target-specific backend work is complete
- the module-level note is still a coarse contract summary, so any future family-specific unsupported work should tighten messages toward stable semantic buckets rather than reintroducing testcase names or drifting back to feature inventory wording

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=54 failed=0 total=54`
- delegated proof was sufficient for this packet because the diff is limited to planner-facing module notes in `src/backend/lowering/lir_to_bir_module.cpp`
