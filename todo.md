Status: Active
Source Idea Path: ideas/open/24_riscv_prepared_edge_publication_register_destination_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory RISC-V Edge Publication Consumer Path

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for
`ideas/open/24_riscv_prepared_edge_publication_register_destination_consumer.md`.

## Suggested Next

Start Step 1 by inventorying the RISC-V prepared edge publication consumer path
and choosing the first register-destination source-home family to implement.

## Watchouts

- Keep this plan scoped to register-destination edge-publication consumers.
- Preserve fail-closed behavior for pointer-base sources and stack-slot
  destinations unless a later lifecycle decision changes scope.
- Do not rediscover edge-copy facts in RISC-V; consume shared prepared
  `edge_publications`.

## Proof

Lifecycle-only activation. No build or test proof required for this packet.
