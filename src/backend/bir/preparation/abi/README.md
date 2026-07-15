# C3 Immutable ABI Requirement Product Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: C3
Upstream: exact Canonical/C1/C2 tuple
Downstream: immutable `AbiPlan` for C4-C9 and D/E consumers

## Purpose

C3 classifies semantic parameters, results, calls, by-value/sret, preservation,
and ABI requirements for one exact tuple without assigning locations or
mutating Canonical nodes.

## Owns

Versioned ABI rule selection, total site classification, typed requirements,
product validation, fingerprinting, and atomic publication.

## Does Not Own

C3 does not select target/layout, lower calls, split graph values, insert
moves/stores, emit save/restore, bind constraints, allocate, or build frames.

## Inputs

The unchanged B8 Canonical owner, full C1 fingerprint, and exact C2
`VerifiedTargetLayout`, including all common predecessor/schema keys.

## Input NodeKind/Tag Vocabulary

All five Canonical groups by immutable reference. Only declared function/call/
aggregate/value sites receive ABI requirement rows; no node tags change.

## Required Analyses and Products

Exact-Canonical `PublicationValueFlow` plus C2. The analysis remains
target-independent; the ABI product combines it with target facts under a new
key rather than retagging the analysis.

## Ordered Behavior

1. Validate Canonical/C1/C2 and exact analysis keys.
2. Select one versioned ABI registry row.
3. Classify every definition/declaration/call/result/site in stable order.
4. Validate total mutually consistent requirements and atomically publish.

## NodeKind/Tag Lowering Matrix

| Canonical input subset | Reference outcome | ABI product outcome | Node tags added/removed | Identity | Failure |
|---|---|---|---|---|---|
| ordinary scalar/value parameter/result/call site | retain reference | exact class/extension/transport requirement | none | unchanged Canonical ID | missing rule rejects |
| aggregate/by-value/sret site | retain reference | exact semantic aggregate/byval/sret requirement, no lane nodes | none | unchanged | incomplete topology/classification rejects |
| call/effect site | retain reference | call preservation/clobber/return requirement root | none | unchanged | signature mismatch rejects |
| control/phi site | retain reference | no ABI row except declared signature/value relationship | none | unchanged | inferred transport forbidden |
| opaque token/inline asm | retain reference | declared ABI-facing ordinary value requirements only | none | unchanged | constraint parsing forbidden |
| unknown/illegal/omitted/later-stage kind | reject product | none | none | none | `AbiVocabularyInvalid` |

## Identity and Provenance

ABI rows use stable Canonical site/value IDs as keys; they do not mint aliases,
split values, or assign semantic identity to physical lanes.

## Outputs

One immutable total `AbiPlan` with exact common key, ABI registry/version,
typed per-site requirements, coverage digest, and fingerprint.

## Verification and Publication

Validate exact predecessor/analysis keys, signature/type/topology agreement,
unique total site coverage, rule consistency, and absence of graph/location/
allocation facts. Publish once or not at all.

## Analysis Preservation and Invalidation

Any Canonical, target, layout, value-flow, ABI registry, or observed-site
change invalidates C3 and all later products. It cannot be patched or relabelled.

## Failure and Diagnostics

Stale/mixed keys, unsupported ABI rule, incomplete classification, conflict,
cancellation, or resource failure publishes no subset or default ABI plan.

## Adjacent-Stage Contract

C2 supplies the exact layout. C4 consumes the same Canonical/C1/C2 tuple plus
this exact plan and does not reclassify ABI.

## Implementation State

Absent. Legacy ABI lowering and target tables are evidence, not this product.

## Proof Requirements

Prove every site family, neighboring unsupported forms, total coverage,
stale/mixed rejection, determinism, no graph/location mutation, and rollback.

## Open Questions

New ABI forms require a versioned registry and explicit product row.
