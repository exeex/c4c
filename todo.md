Status: Active
Source Idea Path: ideas/open/24_riscv_prepared_edge_publication_register_destination_consumer.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Broaden to Additional Ready Source Homes

# Current Packet

## Just Finished

Completed Step 4 by choosing the next RISC-V register-destination source-home
family for this route.

Decision: fold `RematerializableImmediate -> Register` into idea 24 next.
It can reuse the current shared lookup authority:
`prepare::find_unique_indexed_prepared_edge_publication(...)` still selects the
edge-publication fact, while RISC-V owns only target-local source rendering and
instruction emission. The likely target-local policy is to render the immediate
from `source_home.immediate_i32` and emit the appropriate RISC-V register-load
form for the destination register, without rescanning BIR edges or creating a
RISC-V-local edge fact table.

Compared candidate homes:

- `RematerializableImmediate -> Register` is the best next fold-in because it
  only needs a new source operand rendering branch inside the existing
  register-destination helper. It preserves the same shared lookup model as the
  implemented `Register -> Register` path and keeps instruction spelling inside
  RISC-V.
- `StackSlot -> Register` should be deferred. It also has shared prepared edge
  facts, but safe emission requires RISC-V stack-slot address/base policy and
  load instruction selection. That is more than the current helper's simple
  source rendering rule and should be its own packet after the immediate source
  case is proven.

## Suggested Next

Proceed with a code packet for `RematerializableImmediate -> Register` in the
RISC-V prepared edge-publication helper and focused tests. Keep the proof
focused on shared lookup consumption by including a positive immediate-source
case plus a missing-lookup or removed-index negative case.

## Watchouts

- `PointerBasePlusOffset -> Register` and every source-to-`StackSlot`
  destination still fail closed and remain out of scope for this route unless
  lifecycle state changes.
- `StackSlot -> Register` remains ready in the shared prepared data model but
  deferred until RISC-V stack-slot load/address emission policy is handled as a
  separate packet.
- Do not broaden Step 4 into pointer-base addressing, stack destinations, local
  edge rediscovery, or expectation downgrades.

## Proof

Proof type: `docs/decision-only`.

No build or tests were run for this decision-only packet, and no
`test_after.log` was created.
