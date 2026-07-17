# D1 Generic Pseudo Lowering Pass Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: D1
Upstream: exact Prepared admission tuple
Downstream: first private PseudoPreallocation candidate plus exact projection

## Purpose

D1 consumes the unchanged Canonical owner, C1 fingerprint, verified C2-C8
bundle, and C9 binding, then lowers Canonical semantics into the closed generic
pseudo planning vocabulary without concrete registers, frame offsets, or MIR.

## Owns

Generic pseudo selection, complete Canonical disposition, private candidate,
total result/provenance map, first exact-revision constraint projection, and
D1 postconditions.

## Does Not Own

D1 does not lower ABI call transport (D2), select concrete registers/frame
offsets, allocate/spill, remove SSA generally, perform target expansion (D4),
or publish `PseudoBir` (D3).

## Inputs

Exactly the D1 seam in `preparation/README.md`: unchanged B8 Canonical token,
complete C1 target fingerprint, verified C2-C8 bundle, Canonical
`BoundConstraintSet`, and normative Prepared admission envelope.

## Input NodeKind/Tag Vocabulary

Exactly the five normative Canonical groups. Every member receives one matrix
outcome. Prospective pseudo names are documentation vocabulary, not claims that
production `NodeKind` enum entries exist.

## Required Analyses and Products

Exact C2-C8 products, C9 binding, Canonical value-flow/CFG/SSA proof, and
versioned generic pseudo rule registry. No compatible-looking substitute.

## Ordered Behavior

1. Validate the exact Prepared tuple and complete Canonical inventory.
2. Assign every node one explicit matrix row and reserve all output IDs.
3. Build a private candidate with total typed result/provenance mapping.
4. Verify candidate graph/SSA and request C9 projection for its exact revision.
5. Publish no stage; hand the candidate and projection privately to D2.

## NodeKind/Tag Lowering Matrix

| Canonical input group/subset | Outcome and D1 planning output | Tags retained | Tags added | Tags removed | Identity / provenance | Failure |
|---|---|---|---|---|---|---|
| arithmetic/compare/conversion/aggregate `B.CanonicalSsaValue` | replace/expand into prospective `D.GenericValue` operations | semantic/type/effect intent | Pseudo family/admission, virtual def/use roles, exact MIR disposition | Canonical owner/admission; static SSA classification when uses rewritten | fresh IDs for owner/kind/shape change; total result map | missing rule rejects |
| memory/address value/effect | replace/expand into `D.GenericMemoryValue`/`D.GenericEffect` | access/effect/type/provenance intent | pseudo roles and target-requirement references | Canonical owner/admission | fresh IDs + ordered derivation | unrepresented effect rejects |
| ordinary/helper-eligible call | replace with prospective `D.GenericCall` preserving semantic call site | signature/callee/args/result/effects | generic-call pseudo classification and plan references | Canonical call owner/admission | fresh ID and total result map | missing C3/C4/C8 facts reject |
| `B.CanonicalControl` | replace/retain only through explicit pseudo-control rule | control/successor/effects | Pseudo control admission | Canonical control admission | identity preserved only if owner/semantics/roles exact; normally fresh | unknown control rejects |
| `B.CanonicalPhiMerge` | retain semantic merge as prospective `D.PhiPending` for D5 | phi/type/predecessor roles and dynamic SSA proof | PseudoPreallocation admission | Canonical admission | preserve only if roles/result exact | altered edge map rejects |
| `B.CanonicalOpaqueTargetToken` | replace with prospective opaque `D.InlineAsm` carrying exact bytes and C9 bindings | opaque bytes, ordinary roles, effects | pseudo admission/binding reference | Canonical admission | fresh owner ID, source provenance | parsing/changing bytes rejects |
| Canonical node proven to disappear/merge | delete/merge only by registered rule with total result/effect realization | exact surviving semantics | output classifications | source classification | retire source; map every live result | live unmapped obligation rejects |
| unknown/illegal/omitted/premature/later-stage kind | reject | none | none | none | no candidate | `D1VocabularyInvalid` |

## Identity and Provenance

Stage-owner changes normally require fresh IDs. Expansion creates fresh ordered
outputs; deletion/merge retires source only after total typed mapping. C9
projection consumes this derivation but cannot bless incomplete identity work.

## Outputs

One private exact D1 candidate containing only the prospective generic pseudo
groups named by the matrix and one exact-current `ProjectedConstraintSet`.

## Verification and Publication

The private gate checks complete Canonical elimination/disposition, prospective
pseudo schema closure, exact keys, payload/roles/types/effects, def-use/CFG/SSA,
total mapping, and no concrete register/frame/allocation/machine facts. D1 does
not publish `PseudoBir`.

## Analysis Preservation and Invalidation

The new graph revision invalidates Canonical analyses/products as graph facts;
immutable preparation remains lineage input only. D1 recomputes required
candidate facts and obtains a fresh projection keyed to that revision.

## Failure and Diagnostics

Stale/mixed Prepared input, missing rule/product, incomplete mapping,
projection failure, unknown vocabulary, cancellation, or verifier failure
discards the entire candidate and publishes nothing.

## Adjacent-Stage Contract

D2 receives exactly the D1 candidate, its parent tuple, mutation lineage, and
exact projection. It must eliminate every `D.GenericCall` before D3.

## Implementation State

Absent. Prospective pseudo vocabulary is Markdown planning only.

## Proof Requirements

Prove every Canonical member exactly once, identity/result mapping, C9
projection, helper/call adjacency, rollback, and absence of concrete resources.

## Open Questions

New pseudo kinds require a separate schema/implementation idea before code use.
