# C4 Immutable Call Requirement Product Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: C4
Upstream: exact Canonical/C1/C2/C3 tuple
Downstream: immutable `CallPlan` for C5-C9 and D2

## Purpose

C4 binds every canonical direct/indirect/helper-eligible call site to immutable
typed call requirements. It does not lower, replace, or insert call nodes.

## Owns

Versioned call-rule selection, total call-site requirements, preservation/
clobber/return/tail facts, product fingerprint, validation, and publication.

## Does Not Own

C4 does not classify ABI anew, select helpers, lower calls, insert moves/
stores, assign locations, bind asm constraints, allocate, or mutate the graph.

## Inputs

The unchanged Canonical owner, C1 fingerprint, C2 layout, C3 `AbiPlan`, and all
common schema/predecessor versions.

## Input NodeKind/Tag Vocabulary

All five Canonical groups by reference; call-family/refinement queries select
declared call/helper-eligible sites without changing admission or tags.

## Required Analyses and Products

Fresh exact-Canonical `CallGraph` plus C3. Unknown external body facts remain
explicit; they cannot be guessed from names or cause partial lowering.

## Ordered Behavior

1. Validate the full tuple and exact CallGraph key.
2. Select one versioned call-rule registry from C1/C3 identity.
3. Classify every direct/indirect/tail/helper-eligible site in stable order.
4. Validate complete requirements and atomically publish one product.

## NodeKind/Tag Lowering Matrix

| Canonical input subset | Reference outcome | Call product outcome | Node tags added/removed | Identity | Failure |
|---|---|---|---|---|---|
| direct call | retain exact call node | typed callee/signature/args/result/preservation/clobber requirements | none | unchanged | unresolved/mismatched symbol rejects |
| indirect call | retain exact call node | typed callee-value/signature/args/result requirements | none | unchanged | missing signature/value rejects |
| helper-eligible semantic operation | retain semantic node | eligibility requirement only; no helper chosen | none | unchanged | helper selection is forbidden |
| non-call value/effect/control/phi | retain reference | no row except declared call-graph relationship | none | unchanged | inferred call behavior forbidden |
| opaque token/inline asm | retain reference | ordinary call-like effect only if schema explicitly declares it | none | unchanged | text parsing forbidden |
| unknown/illegal/omitted/later-stage kind | reject product | none | none | none | `CallVocabularyInvalid` |

## Identity and Provenance

Product sites cite Canonical IDs and call-site ordinals; they never replace
nodes or become call identity. Names and rendered callee strings are not keys.

## Outputs

One immutable total `CallPlan` with the exact tuple, rule/analysis versions,
per-site requirements, external/absent facts, coverage digest, and fingerprint.

## Verification and Publication

Validate exact keys, total call coverage, callee/signature/result consistency,
agreement with C3, and absence of helper choice, graph edits, physical
locations, or lowering actions. Publish all-or-nothing.

## Analysis Preservation and Invalidation

Any Canonical, target, layout, ABI, CallGraph, call registry, or call-site
change invalidates C4 and successors. A similar call set cannot be rekeyed.

## Failure and Diagnostics

Stale/mixed products, incomplete call facts, unsupported rule, conflict,
cancellation, or resource failure publishes no partial site set or fallback.

## Adjacent-Stage Contract

C3 supplies ABI requirements. C5 consumes the exact unchanged tuple and C4
fingerprint. D2 later lowers calls; C4 never does.

## Implementation State

Absent. Existing call import/legacy lowering is not this immutable product.

## Proof Requirements

Prove direct/indirect/void/value/tail/external neighbors, exact keys, total
coverage, C3 agreement, no graph mutation/lowering, and atomic failure.

## Open Questions

New call forms require explicit versioned rule and product rows.
