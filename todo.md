# Current Packet

Status: Active
Source Idea Path: ideas/open/25_riscv_prepared_edge_publication_stack_source_register_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory RISC-V Stack Source Path

## Just Finished

Lifecycle activation created the active runbook for idea 25. No implementation
work has been performed in this packet.

## Suggested Next

Execute Step 1 by inventorying the current RISC-V prepared edge-publication
consumer and choosing the focused proof subset for `StackSlot -> Register`
register-destination support.

## Watchouts

Do not implement pointer-base sources or source-to-`StackSlot` destinations in
this route. Do not rediscover edge facts locally; the shared `edge_publications`
lookup remains the semantic authority.

## Proof

Lifecycle-only activation; no build or test proof required.
