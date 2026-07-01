# Idea 494 Step 4 - Residual Disposition

## Decision

Idea 490 path/no-clobber certification cannot resume yet.

Idea 494 should stop blocked on the lower endpoint/effect owner. The landed
status surface can publish precise fail-closed interval statuses, but it does
not and should not publish available same-value/no-clobber interval facts from
the current evidence.

## Exact Blocker

The missing prerequisite is an authoritative bridge from
`lir_producer_lookup_key` / `lir_producer_instruction_index` to the ordered
prepared/BIR address-derivation endpoint for the same local-array producer.
That endpoint must be in the effect stream scanned by the classifier.

`lir_producer_instruction_index` remains only a LIR producer-site coordinate.
Using it as a prepared traversal index, BIR instruction index, or ordered
effect endpoint would be coordinate confusion.

## Required Lower-Owned Follow-Up

The lower owner should provide both:

- a producer-key-to-prepared/BIR endpoint bridge for local-array address
  derivation; and
- a real bounded effect scan over the selected proof-source-to-endpoint
  interval covering assignments/redefinitions, phi/alias transfers,
  calls/helpers, inline asm, publications, move bundles, parallel copies, and
  unknown modeled effects.

Until those exist, selected proof-edge availability plus a dynamic local-array
producer row remains `missing_prepared_bir_endpoint_bridge` or another precise
fail-closed status. A row with synthetic bridge flags but no real lower-owned
effect scan remains `selected_path_only_inference`, not `available`.

## Close Readiness

Idea 494 is close/block-ready as a fail-closed interval status-surface slice.
It has identified the next lower prerequisite and should not absorb that bridge
or downstream idea 490 certification work.

## Proof

```sh
git diff --check
```

Result: passed.
