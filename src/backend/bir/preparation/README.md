# Phase C Immutable Preparation Contract

Contract-Status: common C1-C9 product architecture converging under idea 732
Implementation-Status: absent

## Boundary and exact order

Phase C borrows one B8-published `CanonicalBir` and never mutates or
republishes its graph. The strict order is C1 validated `TargetProfile`, C2
target layout, C3 ABI, C4 calls, C5 variadic, C6 address, C7 inline-assembly
context, C8 runtime-helper bundle, and C9 constraint binding/projection.

C1 remains externally owned by [`target_profile`](../../../target_profile/README.md).
BIR receives only its immutable validated profile and complete fingerprint; it
cannot select defaults or reconstruct a compatible profile.

## Prepared admission envelope

The normative `Prepared` vocabulary means immutable admission-by-reference:
the exact five Canonical groups remain owned by the same Canonical graph while
exact target/preparation products are attached by key. It is not a mutable
graph stage, a copy of Canonical storage, or an E4 readiness alias/type. A
prospective preparation-owned node would require separate
schema admission and a new identity; none is introduced by current C1-C9.

## Common exact product key

Every C product key contains, without omission or compatibility fallback:

- the exact owning Canonical module epoch/revision, ordered function-revision
  digest, B1-B7 lineage, and B8 verifier token/fingerprint;
- the complete C1 `TargetFingerprint`, target-profile schema/capability registry
  versions, and normalized request identity;
- the product schema/options/algorithm/registry versions;
- every predecessor product fingerprint in C order; and
- the complete admitted Canonical entity/site coverage digest.

Missing, stale, foreign, mixed-target, cross-revision, reconstructed, subset,
or merely equal-looking keys fail closed. A product is immutable and cannot be
relabelled onto another graph or target.

## Common vocabulary and query rule

C1-C9 may query exactly `B.CanonicalSsaValue`, `B.CanonicalEffect`,
`B.CanonicalControl`, `B.CanonicalPhiMerge`, and
`B.CanonicalOpaqueTargetToken` by reference through the single NodeKind schema.
Queries classify sites for a product; they do not add tags or prove a graph
mutation. Unknown, illegal, omitted, or later-stage vocabulary rejects product
publication.

## Publication, invalidation, and failure

Each owner derives one private complete candidate product in deterministic
stable-ID/site order, validates total coverage and predecessor keys, and
atomically publishes it. Failure publishes no partial table, cumulative bundle,
freshened key, fallback target, or graph capability. Any change to Canonical,
C1, a predecessor product, schema/options, or observed site invalidates that
product and every transitive successor. Recalculation always starts from the
earliest changed owner.

## D1 adjacency

C9 will publish the final exact constraint binding and cumulative preparation
identity. D1 must consume that exact tuple with the unchanged Canonical owner;
it cannot infer readiness from `NodeKind` admission alone, a green report, or a
compatible target. C1-C9 publish no semantic graph revision.
