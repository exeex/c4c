# 183 Phase E Route 5 edge join-source view consumer migration

## Goal

Switch one AArch64 edge-copy join-source reader to a Route 5 edge/join-source
view.

## Why This Exists

Phase D identified Route 5 as a bounded follow-up for edge-copy and join
parallel-copy source recovery. The migration should prove one selected
consumer boundary while retaining prepared helpers as oracles.

Source: `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`.

## In Scope

- One AArch64 edge-copy or join parallel-copy source recovery consumer.
- Route 5 records keyed by edge, predecessor, destination block, and copied
  value role.
- Missing-predecessor, no-source, memory-source, and absent-route cases.
- Prepared edge-publication, edge-publication source-producer, current-block
  join-source, and move-bundle helpers as fallback/oracle surfaces.

## Out Of Scope

- Moving parallel-copy scheduling, out-of-SSA placement, source/destination
  homes, move-bundle selection, branch spelling, or final edge-copy records
  into BIR.
- Hiding prepared edge-copy helpers after one consumer migration.
- Broad aggregate contraction.

## Acceptance Criteria

- The selected consumer uses Route 5 semantic join-source records where present
  and preserves prepared fallback behavior where absent.
- Route/prepared equivalence tests cover normal predecessor,
  missing-predecessor, no-source, memory-source, and absent-route cases.
- Existing route-index fail-closed coverage remains intact.

## Reviewer Reject Signals

- The implementation rescans predecessors or BIR nodes instead of using Route 5
  owned records.
- Helper renames or wrapper reshaping are claimed as migration progress.
- Prepared edge-copy or move-bundle policy is silently moved into BIR.
- Tests weaken oracle expectations or cover only the selected happy path.
- The slice claims broad prepared surface contraction.
