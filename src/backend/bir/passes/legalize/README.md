# B1 / P01 Legalize Pass Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: B1 / P01
Upstream: one verified target-independent `RawBir`
Downstream: one private `TypesLegal` checkpoint for B2

## Purpose

B1 converts the verified Raw vocabulary into portable, type/opcode-legal forms
without target selection or scalar, CFG, SSA, memory, aggregate, or intrinsic
canonicalization. The normative group meanings come from the
[NodeKind contract](../../../../../docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md).

## Owns

- exact-bit constant and portable type/opcode/predicate/cast legalization;
- explicit target-independent truth-boundary formation;
- complete disposition of `B.RawImportOnly`;
- the private B1 candidate and `TypesLegal` postcondition request.

## Does Not Own

B1 does not repair malformed Raw input, select target/ABI/helper facts, perform
B2-B7 canonicalization, establish graph SSA, allocate, form pseudos, or publish
`CanonicalBir`.

## Inputs

One move-only verified `RawBir` with exact epoch/module/function revisions,
Raw verifier token, deterministic order, typed stable IDs, exact def-use and
terminator-owned CFG. The final emitted-kind set is provisional until idea 732
Step 8 audits the landed importer; no parallel importer behavior is assumed.

## Input NodeKind/Tag Vocabulary

The closed planned input is exactly `B.RawValueSemantic`,
`B.RawEffectSemantic`, `B.RawControl`, `B.RawPhiMerge`, and
`B.RawImportOnly`. Each known admitted kind must match exactly one matrix row.
Raw admission is checked through the single production registry; storage reuse
does not admit a kind.

## Required Analyses and Products

No analysis product is required. B1 consumes only the exact Raw capability and
core schema queries. Cached facts grant no mutation authority and stale facts
are rejected by the analysis owner.

## Ordered Behavior

1. Validate the Raw capability, revision, schema admission, and forbidden-fact
   absence.
2. Inventory every kind and payload in stable order and assign one matrix row.
3. Reject all unknown, illegal, unowned, or losslessly unrepresentable forms.
4. Build one private candidate, apply exact replacements/RAUW, and derive the
   mutation summary from actual edits.
5. Run cumulative Raw plus `TypesLegal` checks and atomically advance the B1
   checkpoint; otherwise discard the candidate.

## NodeKind/Tag Lowering Matrix

Pass-local output names are closed checkpoint groups, not production enum names
or new published stages.

| Closed input subset | Outcome and closed B1 output | Tags retained | Tags added | Tags removed | Identity / provenance | Failure |
|---|---|---|---|---|---|---|
| legal portable members of `B.RawValueSemantic` | retain as `B1.LegalValue` | value/SSA participation, semantic/effect/shape/MIR axes | `TypesLegal` checkpoint fact, not a node tag | none | preserve only if all eight identity conditions hold | illegal payload/type is `LegalizeInvalid` |
| Raw alias/noncanonical members of `B.RawValueSemantic` | replace with exact portable `B1.LegalValue` | only proved semantic/type/effect intent | legal portable kind classification | Raw-only kind/admission | fresh ID when kind/shape/result/role/owner changes; record source derivation | no lossless rule is `UnsupportedPortableSemantics` |
| `B.RawEffectSemantic` | retain or replace with `B1.LegalEffect` by its named kind rule | effect/control and exact roles | legal portable classification when replaced | Raw-only classification when replaced | normative identity gate; effects cannot disappear | unhandled effect is `UnknownLegalizeForm` |
| `B.RawControl` | retain exact typed terminator as `B1.RawControl` for B3 | control/effect/successor roles | none | none | preserve; B1 cannot rewrite topology | malformed/illegal control rejects |
| `B.RawPhiMerge` | retain as `B1.RawPhiMerge` for B4 | value, predecessor roles, static SSA eligibility | none | none | preserve exact incoming identities/edges | malformed phi rejects |
| target-independent opaque intrinsic/inline-asm member | retain as `B1.OpaqueToken` | opaque bytes, ordinary roles, conservative effects | none | none | preserve | interpreted target meaning is forbidden |
| losslessly lowerable `B.RawImportOnly` | replace, expand, merge, or delete into one or more groups above according to its registered rule | only semantics proved equal | exact output classifications | import-only classification | replacement/expansion uses fresh IDs and total result mapping; deletion requires no result/effect obligation | incomplete mapping rejects |
| non-lowerable, unknown, or omitted `B.RawImportOnly` | reject | none | none | none | no candidate publication | `UnknownLegalizeForm` / `MissingDownstreamDisposition` |

## Identity and Provenance

The normative eight-condition identity gate applies. Insertions and expansions
receive fresh IDs; deletion retires identity only after all result/effect duties
are realized. Names, positions, pointers, equal rendering, and slot reuse are
never identity evidence.

## Outputs

One immutable private B1 checkpoint with exact new revisions, retained
`RawVerified`, established `TypesLegal`, complete mutation summary, and only
the five closed B1 output groups above. It is not a published stage token.

## Verification and Publication

The candidate must satisfy Raw structural rules, registry payload/arity and
Raw admission, complete matrix coverage, exact type/def-use/CFG integrity, and
absence of target/preparation/pseudo/allocation/frame/machine facts. The pass
framework publishes the checkpoint atomically; B1 cannot mint `CanonicalBir`.

## Analysis Preservation and Invalidation

Any changed type, kind, payload, operand, result, effect, or use invalidates
every observing analysis and dependent. Preservation requires registered
traits plus exact semantic validation under the new key; old handles are never
retargeted.

## Failure and Diagnostics

Failure identifies the exact revision, kind, payload, entity, and matrix row in
stable order. Cancellation, resource exhaustion, verifier rejection, unknown
vocabulary, or partial rewrite publishes no checkpoint or cache entry.

## Adjacent-Stage Contract

A2 alone supplies verified Raw. B2 receives only the exact successful B1
checkpoint and must reject any remaining `B.RawImportOnly` or stale stamp. Step
8 must reconcile this planned input matrix with the final landed importer.

## Implementation State

Absent. Existing core/importer/foundation canonicalize code is not evidence of
this complete B1 pass, matrix, transaction, or proof.

## Proof Requirements

- mechanically cover the final Step 8 Raw kinds exactly once;
- prove every positive row and unknown/illegal/omitted negatives;
- prove exact-bit/type semantics, identity mapping, rollback, and invalidation;
- prove no target fact or catch-all retention enters B1.

## Open Questions

Only Step 8's factual importer reconciliation remains open. A mismatch changes
Markdown or creates a separate implementation requirement; it cannot be filled
by speculation.
