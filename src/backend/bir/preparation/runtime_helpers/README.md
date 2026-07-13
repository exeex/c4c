# Runtime Helper Plan

Status: converged design contract (unimplemented).

## Contract

Runtime-helper planning is `C8`, the final preparation dependency. It consumes
the exact prepared-input borrow, matching target layout, and the published ABI,
call, variadic, address, and inline-assembly-table products. It selects a
declared helper interface only for a Canonical semantic operation whose target
profile explicitly requires that helper route.

The immutable `RuntimeHelperPlan` records semantic operation identity, helper
interface/symbol identity, typed arguments and results, ABI/call requirements,
abstract clobber requirements, recursion guards, and required generic pseudo-
call formation. Shared pseudo and call lowering later create the explicit
nodes. This planner does not pattern-match a testcase, emit a call, choose
general assignments, create spill state, or select machine instructions.

## Binding and consumers

The product key contains the complete Canonical `PipelineStageStamp`, exact
`TargetFingerprint`, layout and helper schema fingerprints, and exact ordered
ABI/call/variadic/address/inline-assembly-table fingerprints. The preparation
publication gate is the immediate consumer. Pseudo lowering and shared call
lowering consume the helper plan only through the verified cumulative bundle.

## Publication

All eligible operations are processed in one transaction. Missing declarations,
signature or calling-convention mismatch, helper recursion, ambiguous routes,
unsupported semantics, stale identities, predecessor/key mismatch, or
diagnostics publish no `RuntimeHelperPlan` and therefore no cumulative bundle.
Inputs remain unchanged.

Legacy coverage: i128/f128, atomic, and intrinsic helpers; declarations,
clobbers, returns, and recursion guards. Exact audit anchors include
`i128_runtime_helpers.*`, `f128_runtime_helpers.*`, and
`regalloc/runtime_helpers.*`.
