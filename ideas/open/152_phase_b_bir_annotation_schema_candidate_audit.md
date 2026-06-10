# 152 Phase B BIR annotation schema candidate audit

## Goal

Audit which `Prepared*` fields should become BIR annotation schema after Phase A
identifies the BIR-owned semantic relationships.

This is Phase B of the BIR/prealloc thinning program. It is analysis-only and
must produce follow-up ideas for annotation schema work rather than directly
changing the schema.

## Shared Artifact Contract

This phase must read:

- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `ideas/closed/151_phase_a_bir_normalization_candidate_audit.md`
- the closure notes for Phase A implementation follow-ups:
  - `ideas/closed/152_bir_producer_source_identity_foundation.md`
  - `ideas/closed/153_bir_select_chain_direct_global_identity.md`
  - `ideas/closed/154_bir_memory_access_identity.md`
  - `ideas/closed/155_bir_block_entry_publication_identity.md`
  - `ideas/closed/156_bir_cfg_edge_publication_identity.md`
  - `ideas/closed/157_bir_call_boundary_source_facts.md`
  - `ideas/closed/158_bir_comparison_condition_producer_identity.md`

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`

The closure note should summarize that document. Phase C, Phase D, and Phase E
must be able to consume the artifact directly.

## Phase A Handoff Requirement

Phase A produced seven BIR-owned normalization routes and the implementation
follow-ups 152-158 closed those routes. This Phase B audit must use the Phase A
artifact and those closure notes as the source of truth when deciding schema
shape.

For each Phase A route, Phase B must decide:

- which BIR relationship fields are durable schema candidates;
- which fields are analysis/query cache and should stay private;
- which fields were only bridge/oracle data used to prove equivalence against
  prepared queries;
- which fields remain blocked because Phase A proved identity but not yet a
  stable annotation container;
- which follow-up idea should prototype or formalize the schema.

Phase B should produce follow-up ideas for schema work only where Phase A has
already established a BIR-owned semantic relationship or where a closure note
identifies a schema blocker. It must not invent new normalization routes that
belong back in Phase A.

## Direction

The desired final shape is that MIR/codegen can consume BIR nodes plus typed
annotations, instead of consuming a parallel `PreparedBirModule` universe.
Prealloc may still fill annotations, but the container and identity should be
BIR-owned.

## Questions

- Which `Prepared*` fields should be copied or fused into BIR node, block,
  edge, function, or module annotations?
- Which fields are derived caches that should never become durable schema?
- Which fields are layout/ABI decisions that should be annotations produced by
  a thin prealloc pass?
- Which fields are target-facing conveniences and should remain outside BIR?
- What annotation identity model avoids duplicating value ids, names, block
  labels, and instruction indexes?

## Required Analysis

Classify candidate annotation locations:

- BIR value annotations
- BIR instruction annotations
- BIR terminator annotations
- BIR block annotations
- BIR edge annotations
- BIR function annotations
- BIR module-level lowering metadata

Inspect at least:

- value-home and storage facts
- call argument/result/outgoing-stack facts
- publication and edge-publication facts
- move-bundle and parallel-copy facts
- address/materialization facts
- comparison and materialized-condition facts
- return-chain facts

## Expected Output

The closure note must contain:

- a link to `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`;
- a Phase A route coverage table showing how the seven Phase A routes map to
  schema candidates, rejects, or blockers;
- a candidate BIR annotation schema map;
- a list of `Prepared*` fields that should stay private/cache-only;
- a list of fields that are target-facing and must not enter BIR;
- follow-up ideas for schema prototypes by domain;
- migration constraints for keeping AArch64 behavior stable while annotations
  are introduced.

The artifact must include traceability back to the Phase A candidate ids or
sections that justify each proposed annotation.

## Reject Signals

- Copying whole `Prepared*` structs into BIR without field classification.
- Creating annotations that expose AArch64 register names, scratch policy, or
  instruction spelling.
- Losing cheap lookup identity or forcing consumers into expensive scans.
- Ignoring future x86/riscv consumers when defining schema shape.
