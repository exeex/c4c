Status: Active
Source Idea Path: ideas/open/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Endpoint Bridge

# Current Packet

## Just Finished

Step 3 implemented the lower endpoint bridge for plan 497. The bridge records
each supported dynamic local-array producer row as a
`LocalArrayEndpointBridgeRecord` keyed by `lir_producer_lookup_key` and row
identity, then binds it to a unique ordered prepared address materialization
when available or to the equivalent ordered BIR address-derivation endpoint.

The implementation preserves `lir_producer_instruction_index` only as a LIR
producer-site coordinate. It fails closed on missing producer rows, duplicate
producer rows, invalid producer coordinates, coordinate confusion, missing
prepared/BIR endpoint bridges, duplicate endpoints, mismatched
function/result/source/derivation/dynamic-index identity, unsupported
operation roles, and missing endpoint order. It does not publish available
same-value/no-clobber interval facts.

## Suggested Next

Execute Step 4 by consuming the bridge record and implementing or routing the
bounded proof-source-to-endpoint effect scan. The next packet should return a
real scanned no-clobber result only when every required ordered effect source
is covered, or a precise fail-closed effect/boundary status otherwise.

## Watchouts

- Step 3 intentionally added endpoint records only; available interval facts
  still require Step 4's bounded effect scan.
- Do not treat `lir_producer_instruction_index` as a prepared traversal index,
  BIR instruction index, or ordered effect endpoint.
- The prepared local-array fixture currently bridges through the ordered BIR
  address-derivation endpoint (`bir_address_derivation`) because no prepared
  frame-slot address materialization exists for the dynamic `%elt.ptr`
  producer.
- Step 4 must cover assignments/redefinitions, memory clobbers, aliases/phi
  transfers, calls/helpers, inline asm, publications, move bundles, parallel
  copies, and unknown modeled effects.
- Do not populate idea 489 proof facts, idea 486 checker inputs, idea 490
  certification, idea 484 packaging, scalar local-load consumption, or RV64/MIR
  lowering.

## Proof

```sh
cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' && git diff --check
```

Result: passed. The proof log path is `test_after.log`.
