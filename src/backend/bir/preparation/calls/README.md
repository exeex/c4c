# Call Plan

Status: converged design contract (unimplemented).

## Contract

Call planning is `C4`. It consumes the exact `VerifiedPreparationInput` borrow,
matching `VerifiedTargetLayout`, and published `AbiPlan`. For every direct,
indirect, and tail-call candidate it derives immutable typed requirements for
ordered inputs and results, parallel moves, outgoing stack objects,
preservation, abstract caller clobbers, hidden carriers, and return recovery.
It also records whether tail-call eligibility is proved or rejected.

The plan describes requirements only. [D2 shared ABI-aware BIR call lowering](../../passes/call_lowering/README.md)
later creates explicit pseudo moves, stores, calls, and result moves from the
exact plan key. This planner does not add instructions, select slots, assign
general values, or create frame offsets. It does not reinterpret any
inline-assembly payload.

Its outgoing-stack identities include complete size, alignment, lifetime,
static/dynamic-region interaction, and access requirements. E4's private
frame-action draft later consumes those identities with D2 operations and
allocation products, fixes their placements, and materializes every required
call/frame action as one or more explicit one-record nodes. The final plan
covers them; unrepresentable actions fail before `MirReadyBirView`.

## Binding and consumers

Every `CallPlan` entry names ordinary Canonical instruction/value identities.
Its product key contains the complete Canonical `PipelineStageStamp`, exact
`TargetFingerprint`, layout and call-plan schema fingerprints, and the exact
`AbiPlan` fingerprint. `VariadicPlan` is the immediate consumer; runtime-helper
planning, D2 call lowering, liveness/allocation, and verification are later
consumers. D2 must match this complete C4 key and the referenced C3 key; it
cannot rebuild a call plan from a D1 graph or select a structurally equal plan.

## Publication

Planning is one module transaction. Unknown callees are handled only by an
explicit ABI rule; unsupported conventions, incomplete operand/result
coverage, illegal parallel requirements, stale identities, incompatible ABI
facts, key mismatch, or diagnostics publish no `CallPlan`. Inputs remain
unchanged.

Any change to the Canonical stage stamp, target fingerprint, layout/call
schema, `AbiPlan` fingerprint, or a named call instruction/value identity
invalidates the complete plan and all C5-C9 successors.

Legacy coverage: `call_plans.*`, `regalloc/call_moves.*`, consumer moves,
call-return ABI, direct/indirect calls, tail-call eligibility, and call-boundary
publications.
