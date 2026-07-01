Status: Active
Source Idea Path: ideas/open/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Producer-To-Endpoint Surfaces

# Current Packet

## Just Finished

Lifecycle switched from blocked plan 494 to new lower-owner plan 497.

## Suggested Next

Execute Step 1 by auditing the producer-to-endpoint surfaces named in
`plan.md`.

## Watchouts

- Do not treat `lir_producer_instruction_index` as a prepared traversal index,
  BIR instruction index, or ordered effect endpoint.
- Do not publish available same-value/no-clobber without an authoritative
  prepared/BIR endpoint bridge and real bounded effect scan.
- Do not populate idea 489 proof facts, idea 486 checker inputs, idea 490
  certification, idea 484 packaging, scalar local-load consumption, or RV64/MIR
  lowering.

## Proof

Lifecycle-only switch; no code proof has been run yet.
