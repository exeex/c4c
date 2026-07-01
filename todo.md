Status: Active
Source Idea Path: ideas/open/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Real Bounded Effect Scan Or Classify Lower Blocker

# Current Packet

## Just Finished

Step 3 remains accepted for plan 497. It implemented the lower endpoint bridge
as `LocalArrayEndpointBridgeRecord`, keyed by `lir_producer_lookup_key` and
producer row identity, then bound supported rows to a unique ordered prepared
address materialization when available or to the equivalent ordered BIR
address-derivation endpoint.

The implementation preserves `lir_producer_instruction_index` only as a LIR
producer-site coordinate. It fails closed on missing producer rows, duplicate
producer rows, invalid producer coordinates, coordinate confusion, missing
prepared/BIR endpoint bridges, duplicate endpoints, mismatched
function/result/source/derivation/dynamic-index identity, unsupported
operation roles, and missing endpoint order. It does not publish available
same-value/no-clobber interval facts.

## Suggested Next

Execute rewritten Step 4 by first deciding whether the real prepared/BIR
effect-source population can be built for the selected proof-source-to-endpoint
interval.

The next packet must do one of these:

- build a production scanner/builder that consumes the Step 3
  `LocalArrayEndpointBridgeRecord`, owns the ordered interval boundaries, and
  populates effect-source records from real prepared/BIR sources before
  publishing available no-clobber; or
- explicitly classify the exact lower blocker or owner, such as missing
  proof-source coordinates, missing ordered effect-source stream population,
  unsupported boundary ordering, or an unmodeled effect family, without
  claiming Step 4 available facts.

The interval classifier must consume a valid bounded scan record without also
requiring the legacy `endpoint_bridge_available` compatibility boolean.

## Watchouts

- Step 3 intentionally added endpoint records only; available interval facts
  still require Step 4's bounded effect scan.
- Do not treat `lir_producer_instruction_index` as a prepared traversal index,
  BIR instruction index, or ordered effect endpoint.
- The prepared local-array fixture currently bridges through the ordered BIR
  address-derivation endpoint (`bir_address_derivation`) because no prepared
  frame-slot address materialization exists for the dynamic `%elt.ptr`
  producer.
- The reviewer rejected the attempted Step 4 route because it allowed
  synthetic availability from caller-supplied coverage booleans and
  caller-supplied effect vectors without production prepared/BIR effect-source
  population.
- Do not treat an externally supplied `LocalArrayBoundedEffectScanRecord`,
  synthetic effect-source list, synthetic coverage booleans, selected-path
  availability, or the legacy `endpoint_bridge_available` flag as proof of a
  real bounded no-clobber scan.
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
