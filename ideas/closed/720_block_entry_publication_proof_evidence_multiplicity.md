# Block-Entry Publication Proof-Evidence Multiplicity

Status: Complete
Type: decomposition of semantic proof-evidence identity and multiplicity
Parent: `ideas/open/718_block_entry_publication_identity_completion.md`
Returns To: `ideas/open/718_block_entry_publication_identity_completion.md`

## Completion

Completed and handed back to idea 718 after the accepted Step 6.5 audit in
`review/idea720_step6_5_handback_audit.md`. Route4 now owns typed claim
classification over exact destination and attributed claim identity, MIR
consumes that classification without a competing ambiguity scan, stale
coordinates fail closed, and the surviving production compatibility overload
cannot manufacture availability from display-name evidence. Matching focused
before/after proof passed for `backend_prepared_lookup_helper`. Idea 718 retains
the production-facing frame/stack contract and broader backend acceptance work.

## Goal

Define and implement an authoritative block-entry publication evidence model
that separates destination semantic identity from individual attributed proof
claims and can classify zero, one, inconsistent, and duplicate claims without
display-name, source-order, or proximity recovery.

## Why This Exists

Idea 718 Step 5 exposed a representational contradiction rather than a local
classification bug. BIR Route4 currently sees PHI name and type but lacks the
prepared attribution needed to distinguish individual claims. Treating
name-plus-type as identity falsely marks same-spelling/same-type distinct
destinations ambiguous. Treating the complete destination pointer and
coordinate as the duplicate key makes two claims for the same destination
collapse into one identity and leaves a true duplicate unrepresentable.

The rejected route is documented in
`review/idea718_step5_ambiguity_acceptance_review.md`. This idea decomposes the
missing model before idea 718 resumes its production and acceptance steps.

## In Scope

- Define destination semantic identity independently from an individual proof
  claim's attribution and prepared instruction coordinate.
- Represent an explicit collection/result model capable of zero claims, one
  valid attributed claim, inconsistent claims, and duplicate attributed claims
  for the same destination.
- Make authoritative BIR Route4 classify that result without display-name,
  source-order, nearest-PHI, or target-emission recovery.
- Bind MIR prepared attribution and coordinate validation to the authoritative
  result rather than performing a second independent ambiguity scan.
- Add focused internal BIR/backend probes for:
  - same-name/same-type destinations with distinct semantic identity
  - true duplicate attributed claims for one destination
  - stale prepared coordinates
  - missing attribution
- Preserve typed fail-closed outcomes for absent, stale, inconsistent,
  ambiguous, or unattributed evidence.

## Probe Strategy

Use focused backend BIR helper tests rather than `tests/backend/case/` source
programs. The contract under decomposition is internal C++ query state:
destination identity, prepared coordinates, attribution, and claim
multiplicity are not independently expressible or observable in a source
program. Source-level cases would therefore require indirect fixture-shaped
inference and could not decisively distinguish the four result cardinalities.
Keep each internal row focused on one classification contract, and retain the
existing production-facing frame/stack test as later integration proof under
idea 718.

## Execution Shape

1. Inventory current prepared, MIR, and BIR ownership and record the blocked
   truth table before changing code.
2. Define separate destination-identity, proof-claim, collection, and typed
   result contracts.
3. Add focused internal probes that make every cardinality and disagreement
   observable.
4. Move Route4 classification onto the explicit collection/result model.
5. Bind MIR attribution and coordinate validation to the authoritative result.
6. Prove the internal matrix, then hand the resulting seam back to idea 718
   for production and broader acceptance.

## Out Of Scope

- Completing or closing idea 718 as part of this decomposition switch.
- Changing prepared-call plans, join/edge publication identity, ABI policy,
  target materialization, storage policy, or emitted publication behavior.
- Restoring route numbers or changing printer/debug vocabulary.
- Using source-program behavior as a substitute for direct proof of internal
  claim identity and multiplicity.
- Broadly redesigning unrelated BIR queries or prepared-state containers.

## Acceptance Criteria

- A documented truth table distinguishes destination identity from proof-claim
  identity and covers zero, one, inconsistent, and duplicate claims.
- Same-name/same-type distinct destinations remain independently available
  when exact attributed evidence selects one destination.
- Two independently represented attributed claims for the same destination
  classify as ambiguous rather than collapsing into one record.
- Stale coordinates and missing attribution fail closed with precise typed
  results.
- Route4 is the authoritative classifier and does not recover identity from
  display name, source order, nearest PHI, or target emission.
- MIR validates prepared attribution and coordinates against the authoritative
  result without a competing rescan.
- Focused internal backend BIR proof is green with no expectation downgrade,
  and the seam is ready for idea 718 to resume.

## Reviewer Reject Signals

- A `%join.arg`, value-71, instruction-zero, named-fixture, or row-specific
  branch claimed as multiplicity support.
- Treating name-plus-type, PHI order, pointer proximity, or target-emission
  evidence as destination or claim identity.
- Calling two equal destination records a duplicate without independently
  represented attribution/claim identity, or deduplicating claims before
  ambiguity classification.
- Keeping Route4 unable to represent all four cardinalities while moving the
  same ambiguity decision behind a renamed helper or result type.
- A MIR-side PHI rescan or source-order recovery that competes with the
  authoritative BIR result.
- Weakening unavailable/ambiguous expectations, marking supported behavior
  unsupported, or rewriting tests so the old name-plus-type behavior passes.
- Classification-only changes or helper renames claimed as capability progress
  without focused same-name/same-type, true-duplicate, stale-coordinate, and
  missing-attribution proof.
- Broad changes to call preparation, join/edge identity, target
  materialization, storage, emission, or unrelated BIR query families.
