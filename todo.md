Status: Active
Source Idea Path: ideas/open/162_bir_publication_availability_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Lookup/Index Helpers

# Current Packet

## Just Finished

Completed Step 3 for
`ideas/open/162_bir_publication_availability_annotation_schema.md`.
Added function-local Route 4 lookup/index helpers over the BIR publication
availability annotation records without switching production consumers:

- `Route4PublicationAvailabilityIndex` groups current-block records,
  block-entry records, and value-link records rebuilt from a `bir::Function`.
- `route4_build_publication_availability_index(...)` rebuilds the index from
  Route 4 current-block producer/value records and block-entry PHI/value
  records.
- `route4_find_current_block_publication(...)` looks up current-block
  availability by indexed block identity or stable block label/id, value
  name/type, and before-instruction boundary.
- `route4_find_block_entry_publication(...)` looks up successor block-entry
  PHI availability by indexed block identity or stable block label/id and
  destination value name/type.
- Lookup failures return explicit Route 4 statuses such as
  missing-publication, missing-value, missing-block, and no-match rather than
  relying on absent records.

Extended `backend_prepared_lookup_helper_test.cpp` oracle coverage:

- Current-block indexed lookup finds available `%sum` as a BIR binary
  producer and preserves the expected producer instruction identity/index.
- Current-block indexed lookup fails closed before the producer boundary with
  explicit missing-publication status.
- Current-block indexed lookup fails closed for value type mismatch with
  explicit no-match status.
- Block-entry indexed lookup finds an available PHI destination as a semantic
  BIR block-entry publication.
- Block-entry indexed lookup preserves explicit missing-publication status for
  a no-PHI successor block even when prepared destination-storage readiness is
  available.
- Block-entry indexed lookup fails closed for destination value type mismatch
  with explicit no-match status.

No MIR query consumer, target/codegen consumer, prealloc production helper, or
prepared publication record shape was switched or copied. The index remains a
target-neutral lookup over Route 4 BIR payloads, not a prepared publication
plan container.

## Suggested Next

Execute Step 4 by migrating the narrow shared MIR publication availability
query consumer to read Route 4 BIR record/index helpers while preserving
prepared and existing MIR oracle checks.

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
- Step 3 intentionally did not switch production MIR, target, or prealloc
  consumers.
- Step 4 should keep the same boundary: target/layout-specific publication
  readiness remains owned by prepared/prealloc or target code.

## Proof

Exact delegated Step 3 proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`1/1` matching backend test passed).
