Status: Active
Source Idea Path: ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Interval Effect Classifier

# Current Packet

## Just Finished

Step 3 added a real fail-closed interval effect status surface and focused
tests without publishing available interval facts.

Implemented `LocalArrayIntervalEffectStatus`,
`LocalArrayIntervalEffectInputs`, `LocalArrayIntervalEffectRecord`, and
`evaluate_local_array_interval_effect` in `src/backend/bir/bir.hpp`. The
surface is keyed to an available selected proof-edge path and dynamic
local-array producer row, checks structured dynamic-index identity, preserves
the LIR-producer coordinate distinction, and exposes the Step 2 unavailable
statuses including `missing_prepared_bir_endpoint_bridge`,
`prepared_bir_coordinate_confusion`, `missing_same_block_ordering`,
`selected_path_only_inference`, and each effect-family unknown/clobber status.

Added synthetic coverage in
`tests/backend/bir/backend_lir_to_bir_notes_test.cpp` proving that a complete
selected proof-edge fixture remains fail-closed with
`missing_prepared_bir_endpoint_bridge`, same-block rows remain fail-closed
without truthful ordering, coordinate confusion and selected-path-only
inference are rejected, and bridged synthetic effect-family flags report their
precise unavailable status. The evaluator still returns
`selected_path_only_inference` instead of `available` for a bridged/scanned
synthetic row with no real lower-owned effect scan.

Recorded the Step 3 summary in
`build/agent_state/494_step3_interval_effect_status_surface/summary.md`.

## Suggested Next

Step 4 should perform residual disposition: route the remaining lower blocker
to the owner of the LIR-producer-to-prepared/BIR endpoint bridge, or decide
whether idea 494 should stop as blocked until that bridge exists.

## Watchouts

- Do not infer same-value/no-clobber from selected path records alone; Step 3
  still fails closed for selected-path-only inference.
- Do not populate idea 489 proof facts or idea 486 checker inputs directly.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate.
- The exact lower blocker is still the missing authoritative bridge from
  `lir_producer_lookup_key` / `lir_producer_instruction_index` to an ordered
  prepared/BIR address-derivation endpoint for the same local-array producer.
- Same-block proof/producer cases remain fail-closed unless a later truthful
  prepared/BIR ordering bridge proves source-before-address-derivation.
- `available` remains disallowed by this slice; the new surface intentionally
  does not make a no-flag synthetic bridged row available.
- Existing untracked `review/*.md` files are transient and must remain
  untouched.

## Proof

Step 3 code-changing proof:

```sh
cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' && git diff --check
```

Result: passed. Output captured in `test_after.log`.
