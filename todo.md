Status: Active
Source Idea Path: ideas/open/168_route2_select_chain_direct_global_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Candidate Route 2 Consumers

# Current Packet

## Just Finished

Lifecycle activation initialized this packet from Step 1 of `plan.md`.

## Suggested Next

Execute Step 1: inventory prepared select-chain/direct-global consumers,
classify candidates, and select one bounded consumer family for the first
Route 2 migration packet.

## Watchouts

- Keep the first packet analysis-only unless the supervisor delegates
  implementation.
- Do not move materialization cost, hazard decisions, register availability,
  publication routing, call ABI behavior, or final move/branch choices into
  BIR.
- Do not delete prepared select-chain helpers while unaudited consumers still
  depend on them.
- Reject call-specific, AArch64-specific, or named-case-only shortcuts.

## Proof

Not run; lifecycle activation only.
