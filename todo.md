Status: Active
Source Idea Path: ideas/open/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Endpoint Bridge And Effect Scan Contract

# Current Packet

## Just Finished

Step 2 defined the endpoint bridge and bounded effect-scan contract for plan
497 and recorded it at
`build/agent_state/497_step2_endpoint_bridge_effect_scan_contract/contract.md`.

Current conclusion: the bridge is implementable only as a real lower-owned
prepared/BIR endpoint binding keyed by `lir_producer_lookup_key` plus producer
row identity. `lir_producer_instruction_index` remains only a LIR
producer-site coordinate, and available same-value/no-clobber remains blocked
until Step 3 adds the authoritative endpoint bridge and Step 4 scans the
bounded effect interval.

## Suggested Next

Execute Step 3 by implementing or routing the lower endpoint bridge. The next
packet should create or identify an authoritative binding from each supported
dynamic local-array `lir_producer_lookup_key` row to a unique ordered
prepared/BIR address-derivation endpoint, or stop on the exact bridge blocker.

## Watchouts

- The Step 2 contract requires producer key fields, endpoint identity,
  proof-source/endpoint interval boundaries, one ordered effect stream,
  distinguishable effect classes, no-clobber criteria, and fail-closed
  statuses before any available result can be published.
- Do not treat `lir_producer_instruction_index` as a prepared traversal index,
  BIR instruction index, or ordered effect endpoint.
- Do not publish available same-value/no-clobber without both an authoritative
  prepared/BIR endpoint bridge and a real bounded effect scan.
- Step 3 should fail closed on missing/duplicate endpoints, mismatched
  result/source/derivation/dynamic index, unsupported operation role, missing
  endpoint order, or coordinate confusion.
- Step 4 must cover assignments/redefinitions, memory clobbers, aliases/phi
  transfers, calls/helpers, inline asm, publications, move bundles, parallel
  copies, and unknown modeled effects.
- Do not populate idea 489 proof facts, idea 486 checker inputs, idea 490
  certification, idea 484 packaging, scalar local-load consumption, or RV64/MIR
  lowering.

## Proof

`git diff --check` passed for the Step 2 contract-only slice. The delegated
proof did not include a build or test run, which is sufficient for this
classification-only packet.
