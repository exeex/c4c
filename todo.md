# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 4, tighten the semantic unsupported boundary

# Current Packet

## Just Finished
- tightened the module-level `lir_to_bir` note surface so function-lowering success and failure no longer claim the obsolete scalar-only route after the backend has already admitted semantic local/global memory, direct and indirect calls, `va_*`, inline asm placeholders, stack save/restore, and the current runtime-family work
- kept the boundary cleanup in `src/backend/lowering/lir_to_bir_module.cpp` only, avoiding any testcase-shaped matcher growth or new semantic-lowering shortcuts just to manufacture more step-3 churn

## Suggested Next
- if new semantic-BIR misses appear, continue step 4 by replacing any remaining stale scalar-only or testcase-history-oriented lowering notes with capability-bucket language that matches the actual admitted route
- keep any future boundary cleanup honest about the current contract: semantic lowering can admit the supported family, or it should fail with a stable capability description instead of pretending the backend is still pre-call/pre-memory

## Watchouts
- this packet only fixes the planner-facing lowering notes; it does not add new semantic capability and must not be treated as evidence that later prepare/legalize or target-specific backend work is complete
- the module-level note remains a coarse contract summary, so any future family-specific unsupported work should tighten messages toward stable semantic buckets rather than reintroducing testcase names or obsolete scalar-only wording

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=54 failed=0 total=54`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed` passed; this packet only retargets planner-facing lowering notes, so equal pass counts were the expected non-regression outcome
