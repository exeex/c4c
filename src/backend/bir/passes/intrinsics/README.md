# B7 / P07 Intrinsic Canonicalization Pass Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: B7 / P07
Upstream: exact B6 `AggregatesCanonical` checkpoint
Downstream: one frozen B7 candidate submitted unmodified to B8

## Purpose

B7 normalizes target-independent intrinsic and call-semantics forms, preserves
opaque inline assembly as an ordinary typed node, and records semantic helper
eligibility without selecting a helper, target, ABI, or machine realization.

## Owns

Closed intrinsic identities/signatures/effects, target-independent semantic
intrinsic expansion, call/intrinsic normalization, helper-eligibility semantic
markers, opaque-token final Canonical shape, and `IntrinsicsCanonical`.

## Does Not Own

B7 does not parse inline-asm constraints, choose runtime helpers, target
features/opcodes, ABI placement, alter CFG/SSA/memory/aggregate semantics, form
pseudos, allocate, emit, or perform B8 verification/publication.

## Inputs

One exact B6 checkpoint with all cumulative properties through
`AggregatesCanonical`, current dynamic SSA proof, exact revisions, and complete
call/intrinsic/opaque payloads.

## Input NodeKind/Tag Vocabulary

Exactly the closed B6 matrix outputs: value/memory/aggregate/projection,
effect, phi, canonical control, and opaque-token groups. Intrinsic/call-family
queries partition applicable members without creating an implicit remainder.

## Required Analyses and Products

Fresh exact-B6 `CallGraph` and `MemoryEffects`, including their declared
PublicationValueFlow/CFG dependencies. Unknown call/effect facts retain only a
valid conservative form; they cannot justify expansion or purity.

## Ordered Behavior

1. Validate B6 capability, SSA proof, and exact analysis keys.
2. Inventory every intrinsic/call/opaque node and assign one matrix row.
3. Select only target-independent registered rules; reject unknown or
   target-dependent selection requests.
4. Build one private candidate, perform complete expansions/RAUW, and derive
   the mutation summary.
5. Recompute affected call/effect/value-flow and SSA facts, freeze the complete
   B7 candidate, and hand that same revision to B8.

## NodeKind/Tag Lowering Matrix

| Closed input subset | Outcome and closed B7 output | Tags retained | Tags added | Tags removed | Identity / provenance | Failure |
|---|---|---|---|---|---|---|
| non-intrinsic value/memory/aggregate/projection member | retain in corresponding `B7.CanonicalSsaValue` | all six-axis facts and dynamic SSA applicability | none | none | preserve | mutation forbidden |
| canonical target-independent intrinsic | retain as `B7.CanonicalSsaValue` or `B7.CanonicalEffect` by result form | intrinsic ID/signature/effects/types/roles | Canonical intrinsic refinement | noncanonical alias if replaced | preserve exact form; replacement follows identity gate | invalid registry/signature rejects |
| registered target-independent intrinsic expansion | expand into canonical value/effect nodes with total mappings | source semantics/types/effects | exact output family/refinements | source intrinsic form | source retires; all outputs fresh with ordered derivation | partial/target-dependent expansion rejects |
| helper-eligible semantic operation | retain semantic node in value/effect group and record eligibility only in exact analysis/product semantics | operation/type/effect intent | no helper/target node tag | none | preserve | helper symbol/ABI choice is forbidden |
| direct/indirect call semantic form | retain or normalize into canonical call member | signature/callee/arguments/result/effects | canonical call refinement | noncanonical call refinement | identity gate; role/result change creates fresh ID | incomplete call facts reject |
| opaque inline assembly | retain as `B7.CanonicalOpaqueTargetToken` | exact bytes, ordinary operands/results, clobbers, conservative effects | Canonical opaque-token admission | no semantic facts | preserve | text parsing, target eligibility, or constraint binding forbidden |
| effect/atomic member not owned by intrinsic rule | retain as `B7.CanonicalEffect` | exact effects/order/roles | none | none | preserve | mutation forbidden |
| phi member | retain as `B7.CanonicalPhiMerge` and re-prove SSA if uses change | phi/type/predecessor roles | refreshed dynamic proof only | none | preserve unless roles change | mapping failure rejects |
| control member | retain as `B7.CanonicalControl` | control/successor/effect facts | none | none | preserve | CFG mutation forbidden |
| unknown, illegal, omitted, import-only, target-selected, preparation, pseudo, allocation, frame, or machine kind | reject | none | none | none | no publication | `UnknownIntrinsicForm` / `ForbiddenTargetAuthority` |

## Identity and Provenance

Expansions retire the intrinsic and create fresh result-mapped nodes. Call or
intrinsic replacement preserves identity only under all eight conditions.
Opaque asm bytes and spelling are payload/provenance, never node identity.

## Outputs

One frozen B7 candidate containing exactly the five normative Canonical groups:
`B.CanonicalSsaValue`, `B.CanonicalEffect`, `B.CanonicalControl`,
`B.CanonicalPhiMerge`, and `B.CanonicalOpaqueTargetToken`, plus cumulative
P01-P07 properties, exact revisions, analyses, and mutation summary.

## Verification and Publication

P07 verifies its local/cumulative postconditions and freezes the candidate but
publishes no `CanonicalBir`. B8 alone runs the full Canonical gate against this
same revision and may issue the private token consumed by the pipeline.

## Analysis Preservation and Invalidation

Intrinsic/call/kind/payload/effect/use changes invalidate CallGraph,
MemoryEffects, PublicationValueFlow, SSA dependents, and transitive consumers.
Fresh results bind the exact B7 key; no B6 handle is retargeted.

## Failure and Diagnostics

Unknown registry entry, stale analysis, incomplete expansion, target/helper
selection, effect drift, SSA failure, cancellation, or verifier failure
publishes no partial B7 wave, product, or stage token.

## Adjacent-Stage Contract

B6 supplies the only input. The pipeline freezes the complete B7 candidate and
submits that identical owner/revision to B8. C1 receives only B8-published
`CanonicalBir`, never a B7 checkpoint or green local report.

## Implementation State

Absent. Intrinsic importer support, call graph documents, and legacy helper
selection are not this pass implementation.

## Proof Requirements

Prove every intrinsic/call/opaque row, conservative unknown effects, complete
expansions, exact product keys, SSA re-proof, target/helper exclusion, and
same-revision B7-to-B8 handoff.

## Open Questions

New intrinsic/helper-eligibility semantics require a closed registry row;
target-specific choice remains C/D ownership.
