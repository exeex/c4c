Status: Active
Source Idea Path: ideas/open/42_riscv_prepared_edge_publication_aggregate_stack_source_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Aggregate Stack-Source Authority

# Current Packet

## Just Finished

Plan activated from
`ideas/open/42_riscv_prepared_edge_publication_aggregate_stack_source_policy.md`.

## Suggested Next

Run Step 1 inventory for RISC-V aggregate-width `StackSlot -> Register`
prepared edge-publication authority. Record current scalar concrete behavior,
available prepared facts for aggregate width/lane/destination/scratch policy,
focused tests, and the recommended first implementation packet or exact
blocker.

## Watchouts

Do not treat aggregate stack sources as scalar loads based only on size,
fixture names, value ids, stack slot ids, offsets, or test names. Preserve
existing scalar 4-byte, 8-byte, and large-offset behavior.

## Proof

Activation-only lifecycle change; no build required.
