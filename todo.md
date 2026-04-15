# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 4, tighten the semantic unsupported boundary

# Current Packet

## Just Finished
- audited `lower_scalar_or_local_memory_inst` end to end after the `load`/`store` note pass and confirmed there is no remaining real non-call, non-runtime unsupported exit inside that function that still bypasses the existing semantic-family note buckets
- kept the result diagnostic-only: the surviving `return false` sites belong to runtime-family handling, call-family handling, or the intentional outer `scalar/local-memory semantic family` wrapper catch-all, so this packet made no code change and did not widen lowering capability

## Suggested Next
- if step 4 continues, audit the outer wrapper boundary in `src/backend/lowering/lir_to_bir_module.cpp` and adjacent lowering entry points to decide whether the remaining `scalar/local-memory semantic family` catch-all is the intended final bucket or whether a still-honest cross-cutting family is missing outside `lower_scalar_or_local_memory_inst`
- avoid reopening `lir_to_bir_memory.cpp` for more note plumbing unless a later packet finds a genuinely unbucketed unsupported path beyond runtime/call handling or the wrapper-level fallthrough

## Watchouts
- do not manufacture a new helper-shaped family for the final fallthrough in `lower_scalar_or_local_memory_inst`; that path now means the instruction never matched the scalar/local-memory/runtime/call route, and the outer wrapper bucket is the honest boundary unless a broader cross-cutting family is identified elsewhere
- `memcpy`/`memset` runtime handling and direct/indirect call-family failures are intentionally out of scope for this audit and already own their own semantic-family notes

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=54 failed=0 total=54`
- delegated proof was conservative for this packet because the resulting diff is `todo.md` only, but it preserves the current backend baseline while recording that the audited note-plumbing sub-slice is exhausted
