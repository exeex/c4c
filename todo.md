# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 2, broaden semantic call lowering beyond the current baseline

# Current Packet

## Just Finished
- lifecycle check: the recent inner local-memory and wrapper-boundary audits are enough to treat ordered step 4 as checkpointed for now, but they do not satisfy the full source idea
- kept the active runbook instead of closing or switching because common direct/indirect call widening and additional runtime-family work still remain inside this same idea and plan

## Suggested Next
- return execution to ordered step 2 and choose one bounded semantic-call packet under `src/backend/lowering/lir_to_bir_calling.cpp` or closely related helpers that widens common direct/indirect callee provenance without reviving `call_decode.cpp` or introducing testcase-shaped ladders
- leave step 4 note work parked unless a later call/runtime packet exposes a genuinely unbucketed unsupported path that the current semantic-family boundary no longer describes honestly

## Watchouts
- do not treat the step-4 checkpoint as idea completion; the source idea still requires additional semantic call and runtime capability work before close can be considered
- do not reopen wrapper-note taxonomy work as a default next step; the next packet should add or clarify real semantic call capability, not further relabel already-honest residual buckets

## Proof
- no new proof run; this is a lifecycle-only focus reset that keeps the existing active plan in place
- latest accepted backend proof remains `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1` with `passed=54 failed=0 total=54`
