# 151 Phase A BIR normalization candidate audit

## Goal

Audit `Prepared*` facts that are really BIR normalization relationships and
produce follow-up ideas for moving those relationships forward into BIR-owned
semantics.

This is Phase A of the BIR/prealloc thinning program. It is analysis-only and
must create smaller follow-up ideas instead of directly rewriting BIR,
prealloc, or MIR consumers.

## Shared Artifact Contract

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`

The closure note should summarize that document, but later phases must be able
to consume the artifact directly without mining the closed idea text. Phase A's
artifact is the required input for Phase B and an input dependency for all
later phases.

## Direction

The long-term direction is to make prealloc as thin as possible. BIR should own
normalizable semantic relationships before prealloc runs. Prealloc should not
be the place where target-neutral producer, dependency, publication, call, or
edge relationships first become understandable.

## Questions

- Which `Prepared*` facts are actually BIR semantic relationships?
- Which relationships are currently computed in prealloc only because AArch64
  needed a stable lowering fact first?
- Which facts should become BIR node, block, function, or edge relationships
  before any allocation/layout pass?
- Which facts must stay out of BIR because they encode layout, ABI placement,
  target routing, or final emission?

## Required Analysis

Inspect at least these domains:

- same-block producer and same-block materialization relationships
- select-chain dependency relationships
- current-block entry publication relationships
- edge-publication and parallel-copy source relationships
- call-boundary semantic relationships that are not ABI placement
- memory/addressing relationships that identify semantic access identity rather
  than frame offsets or target addressing modes
- comparison/materialized-condition producer relationships

Relevant files include:

- `src/backend/bir/`
- `src/backend/prealloc/`
- `src/backend/mir/aarch64/codegen/`
- recent closure notes for ideas 130-150

## Expected Output

The closure note must contain:

- a link to `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`;
- a table of BIR-normalization candidates;
- a table of facts rejected from BIR normalization, with reasons;
- a dependency order for moving candidates forward safely;
- follow-up ideas for each concrete normalization route;
- explicit proof-route recommendations for each follow-up idea.

The artifact must contain the same data in durable handoff form, including
proposed follow-up idea filenames or filename prefixes for each candidate route.

## Reject Signals

- Treating every `Prepared*` fact as BIR normalization.
- Moving ABI placement, stack offsets, target register spelling, scratch
  policy, or final instruction routing into BIR.
- Creating broad ideas that say only "move prepared into BIR".
- Deleting prepared facts before MIR consumers have an equivalent BIR-owned
  relationship.
