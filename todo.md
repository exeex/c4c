# Current Packet

Status: Active
Source Idea Path: ideas/open/26_riscv_prepared_edge_publication_pointer_base_register_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory RISC-V Pointer-Base Path

## Just Finished

Lifecycle activation created the active runbook for idea 26. No implementation
work has been performed in this packet.

## Suggested Next

Execute Step 1 by inventorying the current RISC-V prepared edge-publication
consumer and identifying the `PointerBasePlusOffset` source-home data and
focused proof subset for register-destination support.

## Watchouts

Do not implement source-to-`StackSlot` destinations or stack-source policy
broadening in this route. Do not rediscover edge facts locally; the shared
`edge_publications` lookup remains the semantic authority.

## Proof

Lifecycle-only activation; no build or test proof required.
