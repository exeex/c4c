# 171 Route 5 current-block join-source migration

## Goal

Replace `mir::find_bir_current_block_join_source_identity(...)` and a selected
join-source consumer with Route 5 BIR edge/join-source records before hiding
prepared current-block join-source helpers.

## Source

This idea comes from Phase C:

- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- Route 5 row under `## Step 4 Follow-Up Ideas And Proof Routes`

## Scope

- Current-block join-source semantic identity.
- Edge/join value records.
- Source/no-source and optional memory-source cases.
- One selected edge-copy/join/publication consumer.

## Out Of Scope

- Move bundle order, cycle temporary routing, execution site, phase/carrier
  policy, coalescing, redundancy, destination registers, storage-sharing
  checks, prepared move records, and AArch64 edge-copy emission policy.

## Acceptance Criteria

- The selected helper/consumer reads `Route5EdgeJoinSourceIndex`.
- Edge publication and join-source oracle equivalence is proven.
- Missing predecessor, no-source, and memory-source cases are covered.
- Prepared current-block join-source helpers remain visible until consumers no
  longer need them.

## Proof Route

Run edge publication and join-source oracle tests, then the edge-copy/join or
publication target subset that observes the migrated helper.

## Reviewer Reject Signals

- Encoding parallel-copy scheduling or register-sharing decisions in BIR.
- Contracting prepared join-source helpers before consumer migration.
- Proving only one predecessor/successor happy path.
