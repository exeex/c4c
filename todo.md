# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 4, tighten the semantic unsupported boundary

# Current Packet

## Just Finished
- threaded stable instruction-level failure notes through the front half of `lower_scalar_or_local_memory_inst`, so cast, scalar-binop, `alloca`, and `gep` misses now record exact semantic-family notes before the wrapper-level mixed buckets take over
- kept the slice inside `src/backend/lowering/lir_to_bir_memory.cpp`, without widening admitted semantic capability or adding testcase-shaped fallback handling

## Suggested Next
- continue step 4 by threading the same instruction-level semantic-family note discipline into the remaining `load` and `store` exits inside `lower_scalar_or_local_memory_inst`, so local-memory failures stop falling back to the broader wrapper buckets
- keep unsupported-boundary work diagnostic-only unless a later packet explicitly widens semantic capability under the current runbook

## Watchouts
- this packet only tightens diagnostic bucketing for existing cast/binop/alloca/`gep` misses; it does not widen admitted semantic capability and must not be treated as prepare/legalize or target-backend progress
- the remaining local-memory surface is mostly `load` and `store`, and a follow-on packet should keep the note taxonomy semantic and stable instead of mirroring individual helper names or testcase shapes

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=54 failed=0 total=54`
- delegated proof was sufficient for this packet because the diff stays inside instruction-level unsupported-boundary note plumbing and does not widen lowering capability
