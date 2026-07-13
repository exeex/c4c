# Variadic Plan

Status: converged design contract (unimplemented).

## Contract

Variadic planning is `C5`. It consumes the exact `VerifiedPreparationInput`
borrow, matching `VerifiedTargetLayout`, `AbiPlan`, and `CallPlan`. It derives
immutable facts for variadic function entry, promoted arguments, named/unnamed
boundaries, argument-save areas, `va_list` state and traversal, register/stack
exhaustion, alignment, and aggregate or floating variadic transport.

The layout owns eligible classes and capacities; ABI owns classification; calls
owns per-call requirements. This stage extends those facts for variadic
semantics without recreating them. It plans abstract save-area objects and
traversal requirements, never offsets, instructions, register assignments, or
prologue/epilogue code.

## Binding and consumers

The `VariadicPlan` key contains the complete Canonical `PipelineStageStamp`,
exact `TargetFingerprint`, layout and variadic schema fingerprints, and the
exact ordered `AbiPlan` and `CallPlan` fingerprints. `AddressPlan` is the
immediate consumer; call lowering and later frame planning consume the
published facts.

## Publication

One transaction covers all variadic definitions and call sites. Unsupported
ABI traversal, impossible alignment/capacity, inconsistent named boundaries,
missing promotion facts, stale IDs, predecessor mismatch, or diagnostics
publish no product. Inputs remain unchanged.

Any change to the Canonical stage stamp, target fingerprint, layout/variadic
schema, either predecessor-plan fingerprint, or a variadic definition/call
identity invalidates the complete plan and all C6-C9 successors.

Legacy coverage: `variadic.hpp`, `variadic_entry_plans.*`, target variadic
emission, aggregate and floating variadic arguments.
