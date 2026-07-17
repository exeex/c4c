# C8 Immutable Runtime-Helper Requirement and Cumulative Bundle Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: C8
Upstream: exact Canonical/C1/C2/C3/C4/C5/C6/C7 tuple
Downstream: immutable `VerifiedPreparationBundle` for C9 and D1/D2

## Purpose

C8 selects declared runtime-helper interface requirements for eligible
Canonical semantics and atomically publishes the cumulative C2-C8 bundle. It
does not replace operations with calls or perform call lowering.

## Owns

Versioned helper-registry selection, eligibility-to-interface requirements,
total helper/no-helper coverage, cumulative predecessor binding, validation,
fingerprinting, and bundle publication.

## Does Not Own

C8 does not rewrite nodes, insert calls, lower ABI transport, create symbols,
assign locations, bind constraints, allocate, or emit helper bodies.

## Inputs

The unchanged Canonical owner plus exact C1 fingerprint and C2-C7 products,
with all common schema/registry/options/coverage fingerprints.

## Input NodeKind/Tag Vocabulary

All five Canonical groups by immutable reference. Registered helper-eligible
semantic members are queried without adding helper/call/preparation node tags.

## Required Analyses and Products

Exact C2-C7 products plus fresh exact-Canonical CallGraph and MemoryEffects
where helper eligibility/effects require them. Unknown facts cannot select a
helper.

## Ordered Behavior

1. Validate the unchanged complete tuple and analysis keys.
2. Select one versioned helper registry from C1/ABI identity.
3. Derive helper-interface or explicit no-helper rows for every eligible site.
4. Validate cross-product agreement and atomically publish the cumulative
   C2-C8 bundle.

## NodeKind/Tag Lowering Matrix

| Canonical input subset | Reference outcome | Helper/bundle outcome | Node tags added/removed | Identity | Failure |
|---|---|---|---|---|---|
| registered helper-eligible value/effect operation | retain semantic node | exact helper interface requirement and eligibility reason | none | unchanged | unavailable/ambiguous helper rejects |
| operation natively retained by later lowering | retain reference | explicit no-helper requirement row | none | unchanged | implicit fallback forbidden |
| call/intrinsic member already semantic | retain reference | helper requirement only when registry explicitly owns it | none | unchanged | call insertion/lowering forbidden |
| control/phi/opaque token | retain reference | explicit no-helper row unless a reviewed semantic rule applies | none | unchanged | asm text cannot select helper |
| unknown/illegal/omitted/later-stage kind | reject bundle | none | none | none | `RuntimeHelperVocabularyInvalid` |

## Identity and Provenance

Helper rows cite exact Canonical site IDs and registry interface IDs. They are
requirements/products, not replacement call nodes or semantic identities.

## Outputs

One immutable `VerifiedPreparationBundle` containing exact C2-C8 product
fingerprints, helper/no-helper rows, total coverage, common key, and bundle
fingerprint. Canonical storage remains unchanged.

## Verification and Publication

Validate all predecessor/analysis keys, helper signature/effect agreement with
C3/C4, total eligible-site coverage, and absence of graph edits/call lowering/
locations/pseudos. Publish one complete bundle or nothing.

## Analysis Preservation and Invalidation

Any Canonical/target/C2-C7/helper-registry/CallGraph/MemoryEffects/site change
invalidates C8 and C9/D consumers. A bundle cannot be refreshed in place.

## Failure and Diagnostics

Stale/mixed keys, missing/ambiguous interface, analysis unknown, predecessor
conflict, cancellation, or resource failure publishes no partial/default bundle.

## Adjacent-Stage Contract

C7 supplies the exact context. C9 consumes the unchanged Canonical owner,
complete C2-C8 bundle, and original opaque constraint bytes. D1/D2 later
materialize helper calls/transport; C8 never lowers them.

## Implementation State

Absent. Legacy helper selection/lowering does not implement this product.

## Proof Requirements

Prove helper/no-helper neighbors, signatures/effects, exact analysis/product
keys, total coverage, unavailable helper failure, no call lowering, and atomicity.

## Open Questions

New helpers require a versioned registry and explicit eligibility row.
