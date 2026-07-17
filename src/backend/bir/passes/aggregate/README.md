# B6 / P06 Aggregate Canonicalization Pass Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: B6 / P06
Upstream: exact B5 `MemoryCanonical` checkpoint
Downstream: exact B6 `AggregatesCanonical` checkpoint for B7

## Purpose

B6 normalizes target-independent aggregate values, projections, insertions,
copies, and multi-result representations into closed canonical forms with total
result mappings.

## Owns

Aggregate/projection shape normalization, copy canonicalization, reviewed
aggregate-result decomposition, typed total RAUW, and `AggregatesCanonical`.

## Does Not Own

B6 does not choose ABI lanes/layout, alter CFG, weaken SSA/memory semantics,
normalize intrinsics, form pseudos, allocate, or publish Canonical BIR.

## Inputs

One exact B5 checkpoint with `MemoryCanonical`, refreshed dynamic SSA proof,
complete typed aggregate topology, and exact revisions/products.

## Input NodeKind/Tag Vocabulary

Exactly the eight B5 output groups. Aggregate-family/refinement and result-form
queries partition applicable value/effect members; every other group has an
explicit retention row.

## Required Analyses and Products

Exact-B5 PublicationValueFlow plus current CFG/dominance facts used by SSA.
Source-semantic aggregate topology is core/type authority, not target layout.

## Ordered Behavior

1. Validate B5 capability, SSA proof, and product keys.
2. Inventory aggregate producers, copies, projections, insertions, and uses.
3. Require one complete result/use mapping before any mutation.
4. Build a private candidate; apply replacements/projections and total typed
   RAUW in deterministic order.
5. Re-prove type/def-use/SSA and atomically advance B6 or roll back.

## NodeKind/Tag Lowering Matrix

| Closed input subset | Outcome and closed B6 output | Tags retained | Tags added | Tags removed | Identity / provenance | Failure |
|---|---|---|---|---|---|---|
| non-aggregate `B5.SsaValue`/`B5.MemoryValue` | retain as `B6.SsaValue`/`B6.MemoryValue` | all six-axis facts | none | none | preserve | mutation forbidden |
| canonical aggregate producer | retain as `B6.AggregateValue` | aggregate result/type/SSA/effects | canonical aggregate checkpoint fact only | none | preserve | malformed topology rejects |
| noncanonical aggregate/multi-result producer | replace/expand into one aggregate value plus explicit `B6.ProjectionValue` nodes | semantic components/types/effects | aggregate/projection refinements and exact one-result roles | noncanonical multi-result form | source retires; every output fresh with role/ordinal provenance and total mapping | incomplete mapping rejects |
| extract/projection | retain or replace with canonical `B6.ProjectionValue`; merge only when exact | projected component/type/SSA intent | canonical projection refinement | noncanonical path | identity gate; folded projection uses total RAUW | invalid path/type rejects |
| insert/aggregate update | retain or replace with canonical `B6.AggregateValue` | aggregate topology, value/type/effects | canonical insert/aggregate refinement | noncanonical update form | fresh ID on role/shape change | partial update rejects |
| aggregate copy | retain explicit semantic copy, merge identity copy, or replace with canonical copy by registered rule | type/value/effect semantics | canonical copy classification | redundant/noncanonical copy | deletion only after total RAUW and effect proof | hidden ABI/memory expansion forbidden |
| `B5.MemoryEffect`/`B5.AtomicEffect`/`B5.Effect` | retain as `B6.Effect` with exact subfamily | all effects/roles/order | none | none | preserve | mutation forbidden |
| `B5.PhiMerge` | retain as `B6.PhiMerge`; aggregate phi remains ordinary SSA | phi/type/predecessor roles | refreshed dynamic proof only | none | preserve unless roles change | incomplete incoming mapping rejects |
| `B5.CanonicalControl` | retain as `B6.CanonicalControl` | control/successor facts | none | none | preserve | CFG mutation forbidden |
| `B5.OpaqueToken` | retain as `B6.OpaqueToken` | opaque payload/roles/effects | none | none | preserve | interpretation forbidden |
| unknown, illegal, omitted, import-only, ABI-lane, target, or machine kind | reject | none | none | none | no publication | `UnknownAggregateForm` |

## Identity and Provenance

One-to-many decomposition always retires the source and creates fresh IDs.
Every old result maps totally to one canonical aggregate/projection result.
Copies disappear only after typed RAUW and proof of no effect obligation.

## Outputs

One B6 checkpoint containing the closed matrix outputs, cumulative properties
through `AggregatesCanonical`, exact revisions, mutation summary, and refreshed
whole-graph SSA proof.

## Verification and Publication

Verify aggregate topology/path/type, one ordinary result policy, total result
mapping, reciprocal def-use, unchanged CFG/effects, and dynamic SSA. Reject ABI
lanes, target layout, preparation, pseudo, allocation, frame, or machine facts.

## Analysis Preservation and Invalidation

Producer/result/path/copy/use changes invalidate PublicationValueFlow,
Provenance, MemoryEffects when observed, SSA dependents, and liveness. Only
exact registered preservation may install fresh-key results.

## Failure and Diagnostics

Invalid topology/path, incomplete result mapping, stale products, semantic
drift, unknown vocabulary, cancellation, or verifier failure publishes no
partial projection set or checkpoint.

## Adjacent-Stage Contract

B5 supplies exact memory- and SSA-valid input. B7 receives only the complete B6
checkpoint; C3 later owns ABI classification and physical decomposition facts.

## Implementation State

Absent. Aggregate storage/import support is not P06 implementation.

## Proof Requirements

Prove nested/empty/multi-component aggregates, projection/insert/copy neighbors,
total mappings, SSA re-proof, rollback, invalidation, and ABI exclusion.

## Open Questions

Any genuinely new result form requires normative schema review before a row.
