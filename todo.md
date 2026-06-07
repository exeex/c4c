Status: Active
Source Idea Path: ideas/open/116_aarch64_dispatch_prepared_producer_contract_surface.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Characterize Dispatch Producer Contract Residue

# Current Packet

## Just Finished

Activation complete. `plan.md` now transcribes idea 116 into an executable
runbook, and this `todo.md` points at Step 1 for the first supervisor-delegated
executor packet.

## Suggested Next

Delegate Step 1 - Characterize Dispatch Producer Contract Residue.

The executor should inspect `dispatch_edge_copies.cpp`,
`dispatch_producers.cpp`, and the relevant shared prepared/BIR prealloc query
owners, then update this file with a helper-to-contract map, candidate shared
facts, and proof recommendations. No implementation edits are required for
Step 1 unless the supervisor explicitly includes them in the packet.

## Watchouts

- Do not edit implementation files during Step 1 unless the delegated packet
  explicitly authorizes it.
- Do not reopen idea 47 or idea 64 repairs without current-code evidence of a
  new uncovered gap.
- Reject predecessor rescans, BIR-name matching, same-block named-case
  matching, and select-chain special cases as permanent authority.
- Keep AArch64 instruction spelling, target register policy, memory operand
  emission, branch emission, and true machine-register hazard checks local.
- Any progress claim must cover both the edge-publication/parallel-copy family
  and the current-block producer or join-routing family before closure.

## Proof

Not run. Activation is lifecycle-only.
