# Idea 494 Step 3 - Fail-Closed Interval Effect Status Surface

## Decision

Implemented only the fail-closed interval effect classifier status surface.

No available interval facts are published by this slice. A synthetic row with
an available selected proof-edge path still returns
`missing_prepared_bir_endpoint_bridge` when the only endpoint evidence is the
LIR producer key/site coordinate. Even with synthetic bridge/effect-scan
booleans, the evaluator remains fail-closed as
`selected_path_only_inference` unless a future lower-owned endpoint bridge and
real bounded effect scan are added.

## Lower Blocker

The exact lower blocker remains the missing authoritative bridge from
`lir_producer_lookup_key` / `lir_producer_instruction_index` to an ordered
prepared/BIR address-derivation endpoint for the same local-array producer.

`lir_producer_instruction_index` is still only a LIR producer-site coordinate.
It is not used as a prepared traversal index, BIR instruction index, or effect
scan endpoint.

## Files

- `src/backend/bir/bir.hpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`

## Proof

```sh
cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' && git diff --check
```

Result: passed. Output captured in `test_after.log`.
