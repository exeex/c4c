Status: Active
Source Idea Path: ideas/open/498_dynamic_local_array_ordered_effect_source_stream_builder.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Ordered Effect-Source Population

# Current Packet

## Just Finished

Step 3 repaired production ordered effect-source population for dynamic
local-array bounded scans by closing the unordered-boundary gap identified in
review/498_step3_ordered_effect_source_population_review.md.

Production result:

- `populate_local_array_ordered_effect_source_streams` now creates one
  `LocalArrayOrderedEffectSourceStream` per selected proof-edge path after
  endpoint bridges are populated.
- Streams carry their selected path and endpoint bridge, build the
  `(proof_source, endpoint]` boundary from prepared/BIR coordinates, sort
  sources by the ordered coordinate contract, and fail closed on missing proof,
  endpoint, source, or unordered boundary coordinates.
- `compare(proof_source, endpoint) >= 0` now produces
  `UnorderedBoundaryCoordinate` in the production stream builder before source
  collection can classify a stream as available, and the interval classifier
  maps that to `UnorderedEffectSourceBoundary`.
- The production collector records concrete BIR instruction families for index
  redefinitions, phi/alias uncertainty, calls/helpers, inline asm, and
  publications, and records prepared move-bundle/parallel-copy events when
  prepared value-location metadata exposes them.
- `evaluate_local_array_interval_effect` no longer accepts a caller-supplied
  path-only coverage boolean; a clean builder-backed stream now returns
  `available`.
- Focused coverage proves clean, missing-coordinate, reversed/unordered
  boundary, and unknown/clobber behavior through the production population
  path.

Detailed notes are recorded in
`build/agent_state/498_step3_ordered_effect_source_population/summary.md`.

## Suggested Next

Execute the next packet by connecting downstream bounded local-array consumers
to the populated `local_array_ordered_effect_source_streams` records and
removing any remaining raw/path-only no-clobber inference from the acceptance
path.

## Watchouts

- The stream population is attached to prealloc publication-plan publication,
  but existing downstream bounded-scan consumers may still need to query the
  stored stream records explicitly.
- The boundary-ordering repair intentionally uses a precise unavailable status
  instead of treating existing-but-reversed coordinates as missing coordinates.
- Move-bundle and parallel-copy records are currently conservative
  `unknown_modeled_effect` sources because prepared move records do not carry a
  direct BIR value identity for the local-array dynamic index in this packet.
- Store/publication sources are conservative and may be refined later if a
  narrower publication authority proves the dynamic index is preserved.

## Proof

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output was preserved in `test_after.log`.
