# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 4, tighten the semantic unsupported boundary

# Current Packet

## Just Finished
- taught `BirFunctionLowerer` to record function-scoped runtime/intrinsic family notes when recognized semantic lowering misses abort `va_*`, inline-asm, stack-state, absolute-value, `memcpy`, or `memset` lowering
- kept the slice inside `src/backend/lowering/lir_to_bir.hpp`, `src/backend/lowering/lir_to_bir_calling.cpp`, and `src/backend/lowering/lir_to_bir_memory.cpp`, without adding new semantic capability or testcase-shaped fallback handling

## Suggested Next
- continue step 4 by threading the same semantic-family note discipline into non-runtime function-lowering misses that still surface only as generic module-bucket failures
- keep future unsupported-boundary work centered on stable semantic families rather than reviving feature inventories, testcase names, or backend-target shortcuts

## Watchouts
- this packet only preserves runtime/intrinsic family diagnostics when lowering already fails; it does not widen admitted semantic capability and must not be treated as prepare/legalize or target-backend progress
- generic non-runtime function-lowering failures still collapse to the module-level capability bucket, so the remaining step-4 work is broader unsupported-boundary plumbing rather than another note-only tweak

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=54 failed=0 total=54`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed` passed with equal `54/54` results before and after
