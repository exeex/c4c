# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 4, tighten the semantic unsupported boundary

# Current Packet

## Just Finished
- threaded stable instruction-level failure notes through the remaining `load` and `store` exits in `lower_scalar_or_local_memory_inst`, so those local-memory misses now record exact semantic-family notes before the wrapper-level buckets take over
- kept the slice inside `src/backend/lowering/lir_to_bir_memory.cpp`, without widening admitted semantic capability or adding testcase-shaped fallback handling

## Suggested Next
- continue step 4 by checking whether any non-call, non-runtime misses remain outside the now-annotated function-signature, control-flow, cast/binop/alloca/`gep`, and `load`/`store` buckets, and only add more note plumbing if a real generic fallback path is still left
- keep unsupported-boundary work diagnostic-only unless a later packet explicitly widens semantic capability under the current runbook

## Watchouts
- this packet only tightens diagnostic bucketing for existing `load`/`store` misses; it does not widen admitted semantic capability and must not be treated as prepare/legalize or target-backend progress
- if another follow-on note packet is needed, it should confirm a real remaining generic fallback path first instead of over-segmenting the taxonomy into helper-shaped labels

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=54 failed=0 total=54`
- delegated proof was sufficient for this packet because the diff stays inside instruction-level unsupported-boundary note plumbing and does not widen lowering capability
