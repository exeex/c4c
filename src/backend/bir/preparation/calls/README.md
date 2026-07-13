# Call Plan

Status: converged design contract (unimplemented).

## Contract

Call planning is `C4`. It consumes the exact prepared-input borrow, matching
target layout, and published `AbiPlan`. For every direct, indirect, and
tail-call candidate it derives immutable typed requirements for ordered inputs
and results, parallel moves, outgoing stack objects, preservation, abstract
caller clobbers, hidden carriers, and return recovery. It also records whether
tail-call eligibility is proved or rejected.

The plan describes requirements only. Shared ABI-aware BIR call lowering later
creates explicit pseudo moves, stores, calls, and result moves. This planner
does not add instructions, select slots, assign general values, or create frame
offsets. It does not reinterpret any inline-assembly payload.

## Binding and consumers

Every `CallPlan` entry names ordinary Canonical instruction/value identities.
Its product key contains the complete Canonical `PipelineStageStamp`, exact
`TargetFingerprint`, layout and call-plan schema fingerprints, and the exact
`AbiPlan` fingerprint. `VariadicPlan` is the immediate consumer; runtime-helper
planning, call lowering, liveness/allocation, and verification are later
consumers.

## Publication

Planning is one module transaction. Unknown callees are handled only by an
explicit ABI rule; unsupported conventions, incomplete operand/result
coverage, illegal parallel requirements, stale identities, incompatible ABI
facts, key mismatch, or diagnostics publish no `CallPlan`. Inputs remain
unchanged.

Legacy coverage: `call_plans.*`, `regalloc/call_moves.*`, consumer moves,
call-return ABI, direct/indirect calls, tail-call eligibility, and call-boundary
publications.
