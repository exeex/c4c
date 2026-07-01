Status: Active
Source Idea Path: ideas/open/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Producer-To-Endpoint Surfaces

# Current Packet

## Just Finished

Step 1 audited the producer-to-endpoint surfaces for plan 497 and recorded the
classification artifact at
`build/agent_state/497_step1_producer_endpoint_audit/audit.md`.

Current conclusion: a bounded bridge is contractable but not available from
existing facts alone. The exact lower blocker is the missing explicit
prepared/BIR endpoint binding for each local-array `lir_producer_lookup_key`
row; `lir_producer_instruction_index` remains only a LIR producer coordinate.

## Suggested Next

Execute Step 2 by defining the endpoint bridge and bounded effect-scan
contract: key fields, endpoint identity, interval boundary semantics, effect
classes, and fail-closed statuses.

## Watchouts

- Do not treat `lir_producer_instruction_index` as a prepared traversal index,
  BIR instruction index, or ordered effect endpoint.
- Do not publish available same-value/no-clobber without an authoritative
  prepared/BIR endpoint bridge and real bounded effect scan.
- The closest endpoint candidates are prepared address materializations and BIR
  instruction coordinates, but neither is currently tied to local-array
  producer rows by an authoritative record.
- Effect scanning must cover BIR instruction order, prepared memory/access
  records, call boundary effects, move bundles, parallel copies, publications,
  inline asm, helpers, aliases/phi transfers, and unknown modeled effects.
- Do not populate idea 489 proof facts, idea 486 checker inputs, idea 490
  certification, idea 484 packaging, scalar local-load consumption, or RV64/MIR
  lowering.

## Proof

`git diff --check` passed.
