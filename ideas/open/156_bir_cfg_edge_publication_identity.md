# 156 BIR CFG edge publication identity

## Goal

Add BIR-owned CFG edge publication and join-source identity after producer,
publication, and needed memory identity routes exist.

## Why This Exists

Phase A found that the semantic subset of edge publication and join-source
facts belongs with BIR CFG/value relationships, while parallel-copy execution
and target move routing must remain outside BIR.

Input artifact: `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`.

## In Scope

- Add a CFG edge publication surface keyed by predecessor label, successor
  label, and destination value/name.
- Store source BIR value/name/kind, source producer block/instruction,
  optional source memory-access identity, and current-block join incoming
  expression/source identity.
- Bridge the semantic parts of `PreparedEdgePublication`,
  `PreparedEdgeCopySourceFacts`, and current-block join source facts.
- Switch edge-copy source discovery only after the required BIR identities are
  equivalent to the prepared oracle.

## Out Of Scope

- Move bundle or step order, cycle temporary routing, execution site/block
  placement, phase/carrier policy, coalescing or redundancy flags, destination
  register names, storage-sharing checks, or prepared move records.

## Acceptance Criteria

- BIR edge queries match prepared edge-copy/publication source identity across
  predecessor/successor and join-source cases.
- Edge-copy consumers retain parallel-copy scheduling and move execution in
  prealloc/MIR codegen.

## Proof Route

Run matching before/after backend coverage for edge copies and dispatch
publication. Compare BIR edge source answers with prepared edge publication,
edge-copy source facts, and join-source facts for normal edge, join-source,
optional memory-source, and no-source paths. Escalate if parallel-copy planning
files change.

## Reviewer Reject Signals

- Imports parallel-copy scheduling, cycle temporaries, execution-site
  placement, coalescing flags, destination registers, or storage-sharing
  decisions into BIR.
- Treats edge publication identity as permission to rewrite move execution
  policy.
- Proves only one edge-copy shape without nearby join-source and no-source
  coverage.
