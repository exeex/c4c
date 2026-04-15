# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 4, tighten the semantic unsupported boundary

# Current Packet

## Just Finished
- taught `BirFunctionLowerer` to record function-scoped non-call lowering notes for function-signature, scalar-control-flow, and local-memory wrapper failures before they collapse to the module-level capability bucket
- kept the slice inside `src/backend/lowering/lir_to_bir.hpp` and `src/backend/lowering/lir_to_bir_module.cpp`, without widening admitted semantic capability or adding testcase-shaped fallback handling

## Suggested Next
- continue step 4 by pushing the same stable semantic-family reporting into any still-unannotated instruction-level non-call misses inside `lower_scalar_or_local_memory_inst`, so wrapper-level notes can narrow from mixed buckets toward more exact family labels
- keep unsupported-boundary work diagnostic-only unless a later packet explicitly widens semantic capability under the current runbook

## Watchouts
- this packet only preserves non-call function-family diagnostics when lowering already fails; it does not widen admitted semantic capability and must not be treated as prepare/legalize or target-backend progress
- instruction-level misses inside `lower_scalar_or_local_memory_inst` can still roll up into the new mixed wrapper families, so a follow-on packet should avoid overfitting the note taxonomy while tightening those remaining buckets

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=54 failed=0 total=54`
- delegated proof was sufficient for this packet because the diff stays inside wrapper-level unsupported-boundary note plumbing and does not widen lowering capability
