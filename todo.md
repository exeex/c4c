Status: Active
Source Idea Path: ideas/open/644_rv64_object_route_stack_parameter_abi_residual.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement One Prepared Incoming Stack Formal Authority Path

# Current Packet

## Just Finished

Lifecycle repair consumed the blocking route review in `review/reviewA.md`.
The review rejects the current callee-side Step 3 route as source-idea drift:
`object_emission.cpp` and `prepared_local_memory_emit.cpp` recompute incoming
stack-passed formal offsets from formal order plus ABI size/alignment, then
load via `stack_frame_bytes + incoming_offset`.

That path violates idea 644's prepared ABI/home authority rule and closed idea
512's rejection of RV64-side stack argument offset inference. The source idea
remains valid; the active Step 3 runbook was tightened to require explicit
prepared incoming stack-parameter/home authority.

## Suggested Next

Next packet must remove or replace the RV64-side incoming-offset
reconstruction in the callee-side formal load paths. It should consume an
explicit prepared incoming stack-parameter/home fact for the caller-stack
address, while retaining local spill-slot/home checks only as fail-closed
coherence validation.

If no explicit incoming stack formal authority is published, stop the RV64
consumer work and classify the residual as a producer/prealloc publication gap
instead of deriving offsets from formal order, ABI folklore, source layout, or
final assembly.

## Watchouts

- Do not build on helpers that compute `incoming_offset` by walking
  `function.params` and then add `stack_frame_bytes`; `review/reviewA.md`
  marks that route as high-severity drift.
- Positive coverage must prove the explicit prepared incoming-stack authority,
  not merely an expected load offset such as `sp + 64`.
- Negative coverage should cover missing or ambiguous incoming-stack authority;
  local spill-slot malformed-home rejection alone is not enough.
- The local address helper intentionally reuses
  `prepared_frame_slot_address_call_argument_offset`, so missing prepared
  addressing/frame-plan authority remains fail-closed instead of falling back to
  source syntax or final assembly.
- Stack-argument local frame-address publication must account for an active
  outgoing call-stack adjustment; the focused test checks this with an adjusted
  `s2` materialization before storing the stack argument.
- The prior caller-side local frame-address residual is gone in the linked
  `main`: it now emits `addi t0, sp, 0x10`, adjusted `addi t0, sp, 0x28`,
  `addi s1, sp, 0x18`, and `addi s2, sp, 0x20` before consuming those pointer
  sources as register or stack call arguments.
- The new residual appears callee-side in `bug`, where the first comparison
  loads `0x58(sp)`, the saved `ra` slot, before comparing with `a0` and
  branching to `abort`.

## Proof

Lifecycle-only repair; no implementation validation was run.

Review state: `review/reviewA.md` is the code-review artifact for the pending
review. The rejected baseline candidate introduced 20 failures, so it is not
accepted as proof for this route.
