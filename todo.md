Status: Active
Source Idea Path: ideas/open/498_dynamic_local_array_ordered_effect_source_stream_builder.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Ordered Effect-Source Contract

# Current Packet

## Just Finished

Step 2 defined the ordered effect-source contract for dynamic local-array
bounded scans in `src/backend/bir/bir.hpp` and added focused coverage in
`tests/backend/bir/backend_lir_to_bir_notes_test.cpp`.

Contract result:

- Coordinates are ordered by prepared block index, instruction index, then
  tie-break index, with BIR block label required as coordinate identity.
- The bounded scan interval is `(proof_source, endpoint]`: the proof guard is
  excluded and the endpoint address-materialization coordinate is included.
- Builder-backed availability now comes through
  `LocalArrayOrderedEffectSourceStream`, not caller-supplied booleans.
- Missing builder output maps to `missing_ordered_effect_source_stream`, while
  explicit path-only inference remains `selected_path_only_inference`.
- Missing lower/endpoint/source coordinates or endpoint/stream mismatches fail
  closed as `missing_effect_source_coordinate`.
- Effect families map to existing fail-closed interval statuses for index
  redefinitions, phi/alias uncertainty, calls/helpers, inline asm,
  publications, move bundles, parallel copies, and unknown modeled effects.

Detailed notes are recorded in
`build/agent_state/498_step2_ordered_effect_source_contract/summary.md`.

## Suggested Next

Execute the next packet by building the production ordered effect-source stream
from the Step 1 audited prepared/BIR inputs and attaching it where
`evaluate_local_array_interval_effect` can consume the builder-backed stream.

## Watchouts

- The evaluator still deliberately returns `selected_path_only_inference`
  rather than `available` when the stream has no blocking source; publishing an
  available result should wait until the production builder is wired.
- The stream endpoint must match the `LocalArrayEndpointBridgeRecord`
  prepared/BIR endpoint; a LIR producer instruction index is still not accepted
  as effect-source ordering authority.
- Unsupported modeled effects are fail-closed through family-specific unknown
  statuses where possible, falling back to `unknown_effect` for unknown-family
  records.

## Proof

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output was preserved in `test_after.log`.
