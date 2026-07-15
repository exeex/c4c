# Current Packet

Status: Active
Source Idea Path: ideas/open/801_bir_node_kind_tag_algebra_and_phase_vocabulary_lowering_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Define identity and publication-verifier obligations

## Just Finished

- Completed `plan.md` Step 5 in normative Sections 13 and 14.
- Defined the eight-condition `NodeId` preservation gate across operation and
  result identity, count/type, operand roles, effects/control, stage owner,
  product authority, payload, and provenance/verifier obligations. Arena-slot
  stability is explicitly non-evidence; owner or semantic changes require
  replacement.
- Defined retain, one-to-one lower/replace, insert, delete/disappear, expand,
  project, split, and merge consequences for fresh/retired IDs, total use/result
  mappings, provenance-only derivation, revision advancement, analysis/product
  invalidation, and transaction failure atomicity.
- Clarified C as immutable admission-by-reference: Canonical nodes keep their
  kind, `NodeId`, owner, and revision while external products reference them.
  F machine records own distinct identities; source BIR IDs may be provenance
  only.
- Added common and stage-specific fail-closed verifier gates for Raw,
  Canonical/B4 SSA, Prepared, PseudoPreallocation, Allocated, and
  MirReadyMachine, covering vocabulary/tags, payload/shape/type,
  effects/control, identities, product lineage, and dynamic invariants.

## Suggested Next

- Execute only Step 6: review the complete normative artifact against idea 801
  and decide whether the bounded C++ schema/query proof is necessary.

## Watchouts

- Do not revise or activate idea 732, reopen idea 746, or treat tag
  classification as proof of graph-stage SSA validity.
- Step 6 must reject ornamental code: require Step 7 only if a concrete
  feasibility or single-authority claim remains unproved by the landed schema
  and this artifact.

## Proof

- Passed: `git diff --check > test_after.log 2>&1`.
- Proof log: `test_after.log`.
