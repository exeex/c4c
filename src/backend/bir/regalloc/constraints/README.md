# C9 Constraint Binding and Exact-Revision Projection Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: C9
Upstream: unchanged Canonical/C1 plus exact C2-C8 bundle
Downstream: immutable `BoundConstraintSet` and per-later-revision `ProjectedConstraintSet`

## Purpose

C9 is the sole owner that parses, types, and binds constraint descriptions to
ordinary Canonical operands/results. It also owns deterministic projection of
those bindings onto later graph revisions, but never mutates either graph.

## Owns

Closed constraint grammar/typing, ties/groups/clobber binding, total site
coverage, Canonical binding fingerprint, and the sole exact-revision projection
algorithm/product authority.

## Does Not Own

C9 does not parse assembly templates, mutate nodes, create ties by merging SSA
identity, choose concrete registers, lower inline asm/calls, allocate, spill,
or decide graph revision/transaction order.

## Inputs

The unchanged Canonical owner, complete C1 fingerprint, exact C2-C8 bundle,
original opaque constraint bytes, and ordinary typed operand/result identities.

## Input NodeKind/Tag Vocabulary

All five Canonical groups by reference. Constraint parsing applies only to
registered opaque/call-like sites and their ordinary roles; no node tags change.

## Required Analyses and Products

Exact C2 class/group domain, C3-C8 bundle components, and Canonical
PublicationValueFlow. Projection additionally requires the source binding,
exact source/target revision IDs, total lowering provenance/result map, and the
owning mutator's completed candidate view.

## Ordered Behavior

1. Validate unchanged Canonical/C1/C2-C8 and value-flow keys.
2. Parse bytes with the exact C7 grammar/vocabulary; type and bind every role,
   tie, group, early-clobber, and clobber in stable site order.
3. Validate total coverage and atomically publish `BoundConstraintSet`.
4. For a later mutator request, validate its exact completed candidate and
   total provenance/result map, derive a new projection product, and return it
   without editing or publishing the graph.

## NodeKind/Tag Lowering Matrix

| Input subset/product | Reference outcome | C9 outcome | Node tags added/removed | Identity | Failure |
|---|---|---|---|---|---|
| constrained opaque inline-asm site | retain node/bytes | parsed typed bindings for ordinary operands/results, ties/groups/clobbers | none | Canonical IDs remain distinct; ties constrain homes only | grammar/type/role mismatch rejects |
| registered constrained call/operation | retain reference | exact typed requirement binding | none | unchanged | unsupported constraint rejects |
| unconstrained Canonical site | retain reference | explicit no-binding coverage row | none | unchanged | inferred binding forbidden |
| later replacement with total provenance/result map | borrow candidate | `ProjectedConstraintSet` keyed to exact new revision | none | map old binding to declared new identities; no identity merge | incomplete/ambiguous map rejects |
| later deletion/expansion | borrow candidate | remove/project/split bindings only by total registered rule | none | deleted IDs retire; outputs remain distinct | live unmapped constraint rejects |
| unknown/illegal/omitted/later-stage kind without projection rule | reject product | none | none | none | `ConstraintVocabularyInvalid` |

## Identity and Provenance

Constraint equality is not SSA or node identity. Projection consumes the
mutator's authoritative derivation map and creates a product for its exact
revision; it cannot cause, repair, or bless the mutation.

## Outputs

One immutable Canonical-keyed `BoundConstraintSet`. Each later successful
request produces a separate immutable `ProjectedConstraintSet` keyed by the C9
fingerprint, exact target revision, source/target lineage, projection schema,
and complete binding coverage.

## Verification and Publication

Validate exact tuple/grammar/type/role/tie/group/clobber coverage and C2
eligibility. Projection validates exact candidate revision, total provenance,
no dangling source binding, and unique output coverage. Product publication is
atomic and cannot publish or mutate a graph.

## Analysis Preservation and Invalidation

Any Canonical/C1/C2-C8/grammar/value-flow/site change invalidates the binding
and every projection. Any graph revision invalidates its predecessor projection;
the owning mutator must request a fresh exact-revision product.

## Failure and Diagnostics

Stale/mixed keys, parse/type/role error, unsupported class, ambiguous tie,
coverage hole, incomplete projection map, cancellation, or resource failure
publishes no partial binding/projection and leaves graphs untouched.

## Adjacent-Stage Contract

D1 admission requires the unchanged B8 Canonical owner, exact C1 fingerprint,
complete verified C2-C8 bundle, exact Canonical `BoundConstraintSet`, and the
normative `Prepared` admission envelope. D1's private mutation requests the
first projected set only after its complete candidate/mapping exists; C9 never
creates that candidate or relabels Canonical facts.

## Implementation State

Absent. Existing parsers/allocator constraints do not implement this keyed
binding and projection authority.

## Proof Requirements

Prove grammar neighbors, ordinary input/output/read-write roles, ties/groups/
clobbers, exact keys, total coverage, projection across retain/replace/expand/
delete, stale/mixed rejection, and no graph mutation.

## Open Questions

New constraint forms or projection transformations require versioned explicit
grammar/schema rows; no fallback interpretation is allowed.
