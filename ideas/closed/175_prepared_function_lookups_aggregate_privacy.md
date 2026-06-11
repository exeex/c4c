# 175 PreparedFunctionLookups aggregate privacy

## Goal

Plan and execute a bounded contraction of direct `PreparedFunctionLookups`
field consumers after the relevant projected field has either a route-owned BIR
replacement or a target-local owner.

## Source

This idea comes from Phase C:

- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `PreparedFunctionLookups aggregate privacy` row under
  `## Step 4 Follow-Up Ideas And Proof Routes`

## Scope

- Per-function aggregate construction in AArch64 traversal.
- Direct context lookup pointers and include exposure tied to one projected
  field group.
- Narrow BIR-backed or target-local projections for the selected group.

## Out Of Scope

- Removing the aggregate before every projected field has a route-owned or
  target-local replacement.
- Hiding return-chain through this idea.
- Creating a new BIR lowering-plan aggregate.
- Broad context/module rewrites unrelated to the selected field group.

## Acceptance Criteria

- One direct aggregate field consumer group is replaced by a narrow projection.
- The aggregate remains available for fields not yet migrated.
- Any include/context contraction is behavior-preserving and proven.

## Completion Note

Closed after the `move_bundles` aggregate field group was replaced by the
standalone `prepare::PreparedMoveBundleLookups` projection and traversal include
exposure was contracted. Proof handoff is recorded in commit `d2f41dbeb`.

Remaining aggregate exposure is intentionally outside this closed idea:
`memory_accesses`, `edge_publications`,
`edge_publication_source_producers`, and `return_chains`.

## Proof Route

Use compile proof for AArch64 module/context include contraction plus the
migrated route subset. Run broad backend regression when the aggregate type or
module context projection changes.

## Reviewer Reject Signals

- The slice hides `PreparedFunctionLookups` while public consumers still need
  projected fields.
- Return-chain is swept into the aggregate privacy route.
- The replacement is a renamed aggregate with the same lowering-plan shape.
