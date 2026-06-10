Status: Active
Source Idea Path: ideas/open/162_bir_publication_availability_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate A Low-Risk Query Consumer

# Current Packet

## Just Finished

Completed Step 4 for
`ideas/open/162_bir_publication_availability_annotation_schema.md`.
Migrated the narrow shared MIR current-block publication identity query to use
Route 4 BIR publication records/index helpers as its semantic source:

- `mir::find_bir_current_block_publication_identity(...)` now builds a
  function-local Route 4 publication availability index for the requested BIR
  block and resolves the current-block publication through
  `route4_find_current_block_publication(...)`.
- The MIR result shape remains unchanged: available answers still expose the
  same producer instruction, value identity/name/type, instruction index, and
  `SameBlockProducerKind`.
- The helper preserves existing public fail-closed behavior for missing values,
  block-label mismatch, before-boundary misses, and value type mismatches.
- Route 4 records remain target-neutral BIR payloads. The migration did not
  switch target/codegen, prealloc production helpers, prepared publication
  production, or broad publication consumers.

Extended `backend_prepared_lookup_helper_test.cpp` oracle coverage:

- Current-block `%sum` still compares against prepared publication consumption
  and existing BIR/MIR semantic behavior.
- A focused assertion now checks that the migrated MIR current-block
  publication identity matches the Route 4 indexed BIR record for producer
  pointer, instruction index, value name/type, and producer kind.
- Existing missing, wrong-block, before-boundary, and mismatched-type checks
  remain in place.
- Block-entry Route 4 index coverage remains as an oracle/boundary fixture, but
  the block-entry MIR consumer was intentionally not migrated in this packet.

## Suggested Next

Execute Step 5 by running the broader backend acceptance validation for Route 4
publication availability schema migration and record closure readiness in
canonical `todo.md`.

## Watchouts

- Keep Route 4 annotations block/value scoped and target-neutral.
- Reuse Route 1 producer/value identity records where possible; do not create
  another producer schema.
- Do not import publication hooks, destination homes, storage encodings,
  stack-source extension policy, register views, immediate payload spelling,
  emitted storage availability, scalar publication emission policy, prepared
  move records, move-bundle order, or whole prepared publication records.
- Keep block-entry PHI identity separate from prepared destination-storage
  readiness. Prepared storage policy remains an oracle/boundary, not schema.
- Only the narrow current-block MIR query consumer was migrated in Step 4.
- Block-entry PHI identity remains semantic and distinct from prepared
  destination-storage readiness.
- Do not import publication hooks, destination homes, storage encodings,
  stack-source extension policy, register views, immediate payload spelling,
  emitted storage availability, scalar publication emission policy, prepared
  move records, move-bundle order, or whole prepared publication records in any
  follow-up.
- Target/layout-specific publication readiness remains owned by
  prepared/prealloc or target code.

## Proof

Exact delegated Step 4 proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`1/1` matching backend test passed).
