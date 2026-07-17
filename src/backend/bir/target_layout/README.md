# C2 Verified Target Layout Product Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: C2
Upstream: exact B8 Canonical owner plus external C1 validated target
Downstream: immutable `VerifiedTargetLayout` for C3-C9 and later allocation

## Purpose

C2 derives the finite target layout/category/class/group/slot/alias/capacity
domain for one exact Canonical/C1 tuple without mutating Canonical BIR.

## Owns

Layout schema selection, finite abstract allocation-domain IDs, aliases,
reservations, capacities, ABI eligibility inputs, validation, and publication.

## Does Not Own

C2 does not select/default C1, classify ABI values, mutate nodes, bind
constraints, assign homes, lower calls, choose opcodes, or lay out frames.

## Inputs

One B8 `CanonicalBir` owner/stamp and one external C1 validated profile plus
complete `TargetFingerprint`, with exact common key versions.

## Input NodeKind/Tag Vocabulary

All five normative Canonical groups are admitted by immutable reference. C2
may inspect type/category requirements but creates no node tag or stage change.

## Required Analyses and Products

No semantic analysis is required. C1 profile/capability registries and C2
mapping-schema versions are explicit keyed inputs, never environment defaults.

## Ordered Behavior

1. Bind exact Canonical and C1 identities.
2. Select one versioned mapping row from the complete target fingerprint.
3. Derive stable categories/classes/groups/slots, aliases, reservations, and
   capacities in deterministic registry order.
4. Validate total references and publish one immutable product atomically.

## NodeKind/Tag Lowering Matrix

This is an immutable reference/product matrix; C2 performs no node lowering.

| Canonical input group | Reference outcome | Product facts | Node tags added/removed | Identity | Failure |
|---|---|---|---|---|---|
| `B.CanonicalSsaValue` | retain exact reference | type/category eligibility requirements | none | same Canonical `NodeId` | unsupported typed requirement is explicit failure/absence per registry |
| `B.CanonicalEffect` | retain exact reference | effect-relevant class/reservation inputs where declared | none | unchanged | unregistered requirement rejects |
| `B.CanonicalControl` | retain exact reference | none unless a declared target-layout requirement exists | none | unchanged | no inferred layout from control spelling |
| `B.CanonicalPhiMerge` | retain exact reference | value type/category only; no SSA mutation | none | unchanged | incomplete type rejects |
| `B.CanonicalOpaqueTargetToken` | retain exact reference | opaque eligibility vocabulary only, no constraint binding | none | unchanged | unsupported token is explicit, never guessed |
| unknown/illegal/omitted/later-stage kind | reject product | none | none | none | `TargetLayoutVocabularyInvalid` |

## Identity and Provenance

C2 allocates product IDs, not BIR node IDs. Product rows cite Canonical IDs as
foreign immutable keys; they cannot alias or replace semantic identity.

## Outputs

One complete `VerifiedTargetLayout` keyed by the common tuple, with stable
finite tables, mapping fingerprint, total references, and no graph ownership.

## Verification and Publication

Validate exact keys, closed registries, unique IDs, valid alias units,
reservation/capacity consistency, total table references, and complete site
coverage. Publish all-or-nothing.

## Analysis Preservation and Invalidation

Any Canonical/C1/schema/mapping change invalidates C2 and all C3-C9/later
consumers. Equal tables under another key cannot be preserved by relabelling.

## Failure and Diagnostics

Unsupported target capability, missing row, stale/mixed key, invalid alias or
capacity, cancellation, or resource failure publishes no table or fallback.

## Adjacent-Stage Contract

C1 alone supplies the target identity. C3 consumes this exact C2 fingerprint
with the unchanged Canonical/C1 tuple and may not rederive layout.

## Implementation State

Absent. Existing target profiles or backend register tables are not this keyed
product implementation.

## Proof Requirements

Prove supported profiles plus missing/mixed/stale neighbors, table closure,
determinism, total Canonical references, failure atomicity, and no graph edit.

## Open Questions

New mapping families require versioned C1/C2 review; no fallback is implicit.
