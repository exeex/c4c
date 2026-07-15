# F1 Apply-Only Machine Construction Boundary

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent as this exact boundary; existing target code is
evidence only and is not claimed to implement this contract

Phase: F1
Upstream: exact E4 `MirReadyBirView` and all products named by it
Downstream: one private target machine-graph candidate for F2

## Purpose

F1 closes the BIR-to-MIR handoff by applying registered target mappings. This
document owns admission and disposition completeness; external MIR owns the
machine graph and target instruction semantics.

## Owns

Exact-view admission, one registered mapping lookup per admitted explicit BIR
node, fixed-home application, fresh machine identity, provenance attachment,
and failure-atomic delivery of the complete private candidate.

## Does Not Own

Instruction expansion, legalization, allocation, eviction, spilling, copy
resolution, ABI classification, frame planning, prologue/epilogue invention,
machine verification, encoding, assembly parsing, object layout, or linking.

## Inputs

One borrowing E4 `MirReadyBirView` over its owning `AllocatedBir`, exact target
mapping registry, materialized revision, assignments, projection, E1/E2/E3,
copy/frame fingerprints, `FrameRealizationPlan`, and realizability product.

## Input NodeKind/Tag Vocabulary

The exact E4-admitted `Allocated` vocabulary: ordinary allocated operations,
resolved `EdgeCopy`, explicit `Spill`/`Reload`, opaque `InlineAsm`, bounded
explicit frame actions, and retained source-semantic stack operations. No
implicit remainder is admitted.

## Required Analyses and Products

Every product named by the view must be exact-current for the identical graph
revision and target key. F1 derives no analysis and accepts no reconstructed,
equal-looking, predecessor-keyed, or target-compatible substitute.

## Ordered Behavior

1. Freeze and validate the exact borrowed view, target key, registry version,
   fingerprints, and product keys.
2. Visit every explicit admitted node once in stable graph order.
3. Select exactly one registered one-record mapping and apply already-fixed
   opcode, operand, home, location, frame, and relocation-intent facts.
4. Mint one fresh machine record ID and attach the BIR ID only as provenance.
5. Seal the complete private machine graph for F2, or discard it entirely.

## NodeKind/Tag Lowering Matrix

| F1 input subset | Machine disposition | Retained facts | Added facts | Removed facts | Identity / provenance | Failure |
|---|---|---|---|---|---|---|
| ordinary allocated operation | exactly one registered machine record | semantic effects, concrete operands, source/debug provenance | target opcode/form and fresh machine ID | BIR stage admission | distinct identity; BIR ID provenance only | absent/ambiguous/non-one-record mapping rejects |
| resolved `EdgeCopy` | exactly one move record | endpoints, type, edge provenance, fixed homes | selected target move form | copy-resolution state | fresh machine ID | illegal move rejects |
| explicit `Spill`/`Reload` | exactly one memory-transfer record | object, type, effects, exact frame location | selected load/store form | spill-planning state | fresh machine ID | unencodable fixed transfer rejects |
| opaque `InlineAsm` | exactly one opaque MIR inline-asm record | bytes/text, effects, concrete bindings, clobbers | machine operand spellings | BIR admission | fresh machine ID; payload unchanged | parsing or expansion attempt rejects |
| explicit frame action | exactly one registered frame record | action/point/requirement provenance and fixed roles | selected machine form | frame-action admission | fresh machine ID | hidden, missing, duplicate, or expanding action rejects |
| retained source-semantic stack operation | exactly one registered semantic machine record | original semantics/effects and fixed locations | selected machine form | BIR admission | fresh machine ID | reinterpretation as frame repair rejects |
| unknown, stale, omitted, residual pseudo, `ParallelCopy`, `CopyScratch`, or machine input | reject | none | none | none | none | `F1VocabularyInvalid` |

## Identity and Provenance

Machine storage and IDs are always new. BIR IDs, occurrence lineage, source
locations, and mapping-rule IDs may be explicit provenance but never machine
identity, ownership, ordering authority, or reusable record storage.

## Outputs

One complete private target machine graph plus an exact construction manifest
that bijects admitted BIR nodes with machine records and names the source view,
target, registry, frame plan, fingerprints, and revision. Nothing is public.

## Verification and Publication

The F1 gate proves exact input identity, total one-to-one mapping, no unvisited
or extra record, fixed-home fidelity, opaque-inline-asm preservation, and fresh
machine identity. Success hands the sealed candidate to F2; failure hands off
nothing.

## Analysis Preservation and Invalidation

F1 consumes immutable E4 products without preserving them as machine analyses.
Any source view, registry, target, mapping, order, operand, or record mutation
invalidates the candidate and manifest; it cannot be patched in place.

## Failure and Diagnostics

Unknown/stale input, missing or ambiguous mapping, one-to-many result, illegal
fixed operand, hidden action, identity reuse, cancellation, or exhaustion
rejects the transaction. Diagnostics identify the node and mapping rule but do
not request or perform upstream repair.

## Adjacent-Stage Contract

E4 alone supplies MIR readiness and all phase-E decisions. F1 supplies only a
private graph to F2. The external [MIR boundary](../../mir/README.md) owns graph
semantics; this document owns the exact BIR admission/disposition handoff.

## Implementation State

No checked-in path is proven to consume the exact E4 capability and satisfy
this manifest/gate. Existing target lowering must be audited or adapted later;
its presence is not implementation evidence for F1.

## Proof Requirements

Prove closed vocabulary coverage, exactly one record per admitted node, no
extra records, exact fixed-home/frame application, fresh IDs, opaque payload
transport, transactional failure, and absence of E-phase repair authority.

## Open Questions

None for this contract. Target registry population and implementation adoption
are later implementation work, not permission to weaken F1.
