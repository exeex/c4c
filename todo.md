Status: Active
Source Idea Path: ideas/open/24_riscv_prepared_edge_publication_register_destination_consumer.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Handoff Covered Homes and Follow-Ups

# Current Packet

## Just Finished

Completed Step 6 handoff for the RISC-V prepared edge-publication
register-destination route.

Supported RISC-V register-destination edge-publication homes are exactly:

- `Register -> Register`, emitted as `mv <destination-register>, <source-register>`
- `RematerializableImmediate -> Register`, emitted as
  `li <destination-register>, <immediate>`

Both source families consume authority only through the shared
`prepare::find_unique_indexed_prepared_edge_publication(...)` lookup path.
This is not full RISC-V edge-publication support.

## Suggested Next

Close idea 24 if the supervisor accepts the scoped RISC-V consumer handoff.
Recommended follow-up ideas or packets:

- Add `StackSlot -> Register` RISC-V edge-publication consumption after defining
  target-local stack-slot load/address emission policy.
- Add `PointerBasePlusOffset -> Register` RISC-V edge-publication consumption
  only after pointer-base addressing is represented by a real semantic lowering
  rule.
- Add source-to-`StackSlot` destination support as a separate destination-home
  initiative, not as part of this register-destination consumer route.

## Watchouts

- `PointerBasePlusOffset -> Register` remains unsupported/fail-closed.
- Source-to-`StackSlot` destinations remain unsupported/fail-closed.
- `StackSlot -> Register` remains deferred until RISC-V stack-slot load/address
  emission policy is handled as a separate packet.
- Do not claim pointer-base addressing, stack destinations, stack-slot source
  loads, or full RISC-V edge-publication coverage from this handoff.

## Proof

Proof for this packet: `docs/handoff-only`. No tests were run and no
`test_after.log` was created by this handoff packet.
