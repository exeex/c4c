# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 4, tighten the semantic unsupported boundary

# Current Packet

## Just Finished
- audited the outer wrapper boundary in `src/backend/lowering/lir_to_bir_module.cpp` after the inner local-memory note pass and confirmed the remaining wrapper buckets are already the honest step-4 boundary
- kept the result diagnostic-only: `lower_alloca_insts()` owns `local-memory semantic family`, `lower_block_insts()` owns the broader non-terminator `scalar/local-memory semantic family`, and this packet made no code change or lowering-capability change

## Suggested Next
- if the supervisor keeps step 4 active, move out of `lir_to_bir_memory.cpp`/wrapper note audits and decide whether the current unsupported-boundary work is exhausted enough for a plan-owner close-or-switch decision
- otherwise, only reopen the wrapper boundary if a later packet finds a real cross-cutting unsupported path outside the existing function-signature, control-flow, call/runtime, local-memory, or scalar/local-memory families

## Watchouts
- do not split `scalar/local-memory semantic family` further just to mirror helper boundaries; at the wrapper level it now means the instruction failed to enter the compare/cast/binop/runtime/call/local-memory route at all, which is the honest residual bucket
- `memcpy`/`memset` runtime handling, direct/indirect call-family failures, and scalar-control-flow failures already own separate semantic-family notes, so this wrapper audit should not be read as justification to reopen those routes without a new concrete miss

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=54 failed=0 total=54`
- delegated proof was conservative for this packet because the resulting diff is `todo.md` only, but it preserves the current backend baseline while recording that the wrapper-level unsupported-boundary audit is exhausted
