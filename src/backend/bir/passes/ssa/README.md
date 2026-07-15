# B4 / P04 SSA Canonicalization Pass Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: B4 / P04
Upstream: exact B3 `CfgCanonical` checkpoint
Downstream: exact B4 `SsaCanonical` checkpoint for B5

## Purpose

B4 is the sole phase-B owner that constructs and proves dynamic graph SSA:
one definition, dominance, use-def, and exact predecessor-qualified phi input.
The static `SsaEligible` kind classification is only an admission predicate and
never proof of these graph facts.

## Owns

SSA renaming/construction, phi insertion/normalization/deletion, complete typed
RAUW, dynamic SSA verification, and `SsaCanonical`.

## Does Not Own

B4 does not alter CFG topology, infer SSA from tags, normalize memory/
aggregates/intrinsics, bind target facts, remove SSA (D5 owns that), allocate,
or publish `CanonicalBir`.

## Inputs

One exact B3 checkpoint with cumulative properties through `CfgCanonical`,
terminator-derived CFG, stable typed definitions/uses, and phi candidates keyed
to exact predecessor edge identities.

## Input NodeKind/Tag Vocabulary

Exactly `B3.Value`, `B3.Effect`, `B3.CanonicalControl`,
`B3.PhiCandidate`, and `B3.OpaqueToken`. Every value-producing kind is queried
for stage-qualified static SSA participation, but the answer grants no dynamic
validity.

## Required Analyses and Products

Fresh exact-B3 `Cfg`, `Dominance`, and `PublicationValueFlow` products with
complete dependency fingerprints. Unknown dominance or incomplete value-flow
is failure, not a reason to publish partial SSA.

## Ordered Behavior

1. Validate B3 capability and all exact analysis keys.
2. Inventory every eligible definition/use and compute dominance frontiers in
   stable block/edge/value order.
3. Assign every node one matrix row; reject non-eligible ordinary SSA use.
4. Build one private candidate, insert/normalize phis, rename definitions and
   uses, and delete redundant phis only through total typed mappings.
5. Recompute CFG/dominance/value-flow on the candidate, prove whole-graph SSA,
   and atomically advance B4 or discard it.

## NodeKind/Tag Lowering Matrix

| Closed input subset | Outcome and closed B4 output | Tags retained | Tags added | Tags removed | Identity / provenance | Failure |
|---|---|---|---|---|---|---|
| `B3.Value` whose kind is stage-qualified `SsaEligible` | retain or rename uses into `B4.SsaValue` | value/type/semantic/effect/shape/static SSA classification | dynamic `SsaCanonical` checkpoint proof, not a node tag | none | preserve definition ID when semantics/roles/results unchanged | missing/duplicate/non-dominating definition rejects |
| value requiring merge at dominance frontier | insert canonical `B4.PhiMerge` plus renamed `B4.SsaValue` uses | incoming value types and semantic provenance | phi family, predecessor-qualified roles; dynamic proof | noncanonical transport form | every inserted phi gets fresh ID and ordered source/edge provenance | incomplete incoming edge rejects |
| `B3.PhiCandidate` already exact | retain as `B4.PhiMerge` | value/type/phi/static SSA/predecessor roles | dynamic graph SSA proof only | candidate checkpoint label | preserve only if every incoming role and result is exact | malformed or duplicate incoming rejects |
| redundant `B3.PhiCandidate` | merge/delete through one total typed replacement | selected value/type semantics | none | phi kind/admission on retired node | retire phi ID after total RAUW; retain source as provenance | live unmapped use rejects |
| ordinary value kind marked `NeverSsa` but used as SSA definition | reject | none | none | none | no publication | `SsaKindParticipationInvalid` |
| `B3.Effect` | retain as `B4.Effect` | all non-value effects/roles | none | none | preserve | treating as ordinary definition rejects |
| `B3.CanonicalControl` | retain as `B4.CanonicalControl` | control/successor/effect facts | none | none | preserve | topology mutation forbidden |
| `B3.OpaqueToken` | retain as `B4.OpaqueToken`; ordinary eligible results enter SSA normally | opaque payload/effects and ordinary roles | dynamic proof for eligible results only | none | preserve node; result definitions obey graph proof | hidden special-value channel rejects |
| unknown, illegal, omitted, or import-only kind | reject | none | none | none | no publication | `UnknownSsaForm` |

## Identity and Provenance

Inserted phis always receive fresh IDs. Renaming uses does not change producer
identity. Phi elimination retires the phi after total RAUW. Every mapping is
typed and edge-qualified; source spelling, pointer, position, or arena slot is
not identity.

## Outputs

One exact B4 checkpoint containing `B4.SsaValue`, `B4.PhiMerge`,
`B4.Effect`, `B4.CanonicalControl`, and `B4.OpaqueToken`, cumulative properties
through `SsaCanonical`, exact graph proof, revisions, and mutation summary.

## Verification and Publication

The B4 gate proves registry/stage legality, one definition for every ordinary
SSA value, complete reciprocal def-use, definition dominance, same-block order,
one typed phi incoming per exact predecessor edge including multiplicity, no
hidden special-value path, and unchanged CFG topology. Only this whole-graph
proof establishes dynamic SSA.

## Analysis Preservation and Invalidation

Definition/use/phi/operand/order changes invalidate value-flow, dominance
consumers, provenance, memory effects that observe values, liveness, and all
dependents. CFG may be preserved only after exact topology equality validation;
fresh B4 products use the new key.

## Failure and Diagnostics

Stale analysis, missing/duplicate definition, dominance violation, phi-edge
mismatch, non-eligible SSA use, unknown vocabulary, cancellation, or verifier
failure publishes no partial function, proof token, analysis, or checkpoint.

## Adjacent-Stage Contract

B3 supplies exact canonical CFG. B5 receives only the complete B4 checkpoint
and may rely on dynamic SSA because it names B4's proof, never because a kind
has `SsaEligible`. D5 later owns SSA removal.

## Implementation State

Absent. Static NodeKind helpers, CFG/dominance documents, or current phi import
support do not implement dynamic B4 construction/proof.

## Proof Requirements

Prove diamonds, loops, parallel edges, unreachable policy, same-block ordering,
phi insertion/elimination, `NeverSsa` negatives, stale analyses, rollback, and
the distinction between static eligibility and dynamic proof.

## Open Questions

Any new value/merge form requires a reviewed schema admission and explicit
matrix row; no tag query may weaken the whole-graph verifier.
