Status: Active
Source Idea Path: ideas/open/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Consume Stored Ordered Effect Stream

# Current Packet

## Just Finished

Lifecycle resumed idea 497 after closed idea 498. The new runbook starts at
Step 4 because Step 3 endpoint bridging is already complete and idea 498 says
the stored ordered effect-source stream is ready for 497 consumption.

## Suggested Next

Execute Step 4 from `plan.md`: consume exactly one matching production
`local_array_ordered_effect_source_streams` record through the Step 3 endpoint
bridge, and reject the prior synthetic bounded-scan route.

## Watchouts

- Do not treat `lir_producer_instruction_index` as a prepared traversal index,
  BIR instruction index, or ordered effect endpoint.
- Do not publish available same-value/no-clobber from caller-supplied coverage
  booleans, caller-supplied effect vectors, selected-path availability, or the
  legacy `endpoint_bridge_available` flag.
- Keep this lower-owner work out of idea 489 proof facts, idea 486 checker
  inputs, idea 490 certification, idea 484 packaging, scalar local-load
  consumption, and RV64/MIR lowering.

## Proof

Lifecycle-only activation:

```sh
git diff --check
```
