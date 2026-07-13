# Shared BIR Register Allocation

Status: scaffold (unimplemented).

## Owns

One shared RV64, AArch64, and x86 flow for liveness, interference, allocation,
eviction, spill-slot management, and coordination of explicit spill/reload
insertion. Target differences enter as verified layout and constraint data,
not as separate allocators.

## Does not own

Target-specific allocator implementations, concrete target register names,
machine instruction selection, frame-offset encoding, or parsing inline
assembly instruction text.

## Input

One immutable Canonical-BIR revision, a target-context-keyed verified abstract
layout, and preparation facts bound to that exact revision/layout: typed
operand/result requirements, ABI eligibility, resolved clobbers, ties,
early-clobbers, and group rules.

## Output

A new immutable BIR revision plus complete abstract
`(category, class/group, slot)` assignments, abstract spill-slot identities,
and explicit `Spill`/`Reload` nodes suitable for allocated-boundary
verification. Canonical BIR remains unchanged.

## Verification and publication gate

Allocation publishes only when all live ranges obey class, alias, reserved,
ABI, tie, clobber, group-width, alignment, and capacity rules and all evicted
values have consistent explicit spill state. Initial choice, retry, eviction,
spill, reload, and fallback use one legality predicate. Stale keys, exhaustion
without a legal spill route, incomplete assignments, or candidate-verifier
failure discard the entire private candidate and publish no revision.
