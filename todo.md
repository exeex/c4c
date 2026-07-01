Status: Active
Source Idea Path: ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Interval Effect Classifier Contract

# Current Packet

## Just Finished

Step 2 fail-closed interval effect classifier contract completed in
`build/agent_state/494_step2_interval_effect_contract/contract.md`.

The contract defines:

- interval key fields from populated selected proof-edge path records plus the
  dynamic local-array `lir_producer_lookup_key`;
- interval start as the selected proof edge/successor entry and interval end as
  the address-derivation event for the same producer key;
- the current endpoint blocker:
  `lir_producer_instruction_index` is only a LIR producer-site coordinate, not
  a prepared/BIR ordered endpoint;
- same-value criteria requiring structured dynamic-index identity, selected
  path coverage, resolved phi/alias transfers, and a bounded effect scan;
- effect classes for assignment/redefinition, phi/alias/join transfer,
  calls/helpers, inline asm, publications, move bundles, parallel copies, and
  unknown modeled effects;
- unavailable statuses for missing producer/key/path/index evidence,
  `missing_prepared_bir_endpoint_bridge`,
  `prepared_bir_coordinate_confusion`, `missing_same_block_ordering`,
  `selected_path_only_inference`, and effect-family unknown/clobber cases;
- same-block fail-closed policy unless a future truthful ordering bridge proves
  the proof source precedes the address derivation;
- the explicit availability rule: all otherwise promising rows remain
  unavailable until a real LIR-producer-to-prepared/BIR endpoint bridge exists,
  and the classifier must refuse coordinate confusion or selected-path-only
  inference.

## Suggested Next

Route Step 3 to the lower endpoint bridge owner, or add only fail-closed
status surface/tests that preserve the contract. Do not implement available
interval facts until a real prepared/BIR endpoint bridge exists for the
local-array `lir_producer_lookup_key`.

## Watchouts

- Do not infer same-value/no-clobber from selected path records alone.
- Do not populate idea 489 proof facts or idea 486 checker inputs directly.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate.
- Same-block proof/producer cases remain fail-closed unless a later truthful
  ordering bridge exists.
- The missing endpoint bridge also affects cross-block available records when
  the selected successor block is the producer block: effects before the
  address derivation cannot be separated from later block effects.
- `available` is contractually disallowed for this slice; use
  `missing_prepared_bir_endpoint_bridge`, `prepared_bir_coordinate_confusion`,
  `missing_same_block_ordering`, `selected_path_only_inference`, or the precise
  effect-family unavailable status.
- Existing untracked `review/*.md` files are transient and must remain
  untouched.

## Proof

Step 2 contract proof:

```sh
git diff --check
```

Result: passed. This delegated classification-only proof does not write
`test_after.log`; the packet kept canonical log files out of scope.
