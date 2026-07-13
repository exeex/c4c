# ABI Plan

Status: converged design contract (unimplemented).

## Contract

ABI planning is `C3`, the first preparation dependency. It consumes the exact
`VerifiedPreparationInput` borrow and matching `VerifiedTargetLayout`. It
classifies each Canonical parameter, result, call-visible aggregate, by-value
object, hidden-result carrier, HFA-style value, and calling-convention boundary
against layout-owned ABI eligibility.

Its immutable `AbiPlan` records typed class/group, register-eligible versus
stack-required, width/alignment, splitting, hidden-carrier, preservation, and
formal/result requirements. These are placement requirements for later call
lowering, not moves, selected slots, stack offsets, or machine instructions.
Canonical value and instruction IDs remain the only semantic identities.

## Binding and consumers

The product key contains the complete Canonical `PipelineStageStamp`, exact
`TargetFingerprint`, target-layout schema fingerprint, ABI-plan schema
fingerprint, and no predecessor-plan fingerprint because this is the first
planner. `CallPlan` is its immediate consumer; variadic, runtime-helper, call
lowering, allocation, and verification may consume the same published product.

## Publication

One private transaction classifies the entire module. It rejects unsupported
calling conventions, unclassifiable types, illegal split/group shapes,
ineligible classes, stale IDs, incomplete formals/results, key mismatch, or
non-deterministic classification. Failure publishes no `AbiPlan`; Canonical
storage and the layout remain unchanged.

Legacy coverage: call-return ABI classification, target register profiles,
formal publications, value locations, special carriers, and aggregate ABI.
