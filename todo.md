Status: Active
Source Idea Path: ideas/open/171_route5_current_block_join_source_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Residual Route 5 Consumers

# Current Packet

## Just Finished

Route 5 current-block join-source migration was activated from
`ideas/open/171_route5_current_block_join_source_migration.md`.

## Suggested Next

Execute Step 1 from `plan.md`: inventory residual Route 5 consumers, select the
first join-source consumer, and record the target files, proof command, and
positive/negative cases before implementation begins.

## Watchouts

- Keep Route 5 limited to edge publication identity and current-block
  join-source semantic identity.
- Do not import move bundle order, parallel-copy scheduling, cycle temporary
  routing, execution site policy, carrier/phase policy, coalescing,
  redundancy, destination registers, storage-sharing checks, prepared move
  records, or AArch64 edge-copy emission policy into BIR.
- Do not hide prepared current-block join-source helpers until direct
  consumers have been re-scanned and migrated or proven out of scope.

## Proof

No build or test run was required for this activation-only packet.
