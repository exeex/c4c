Status: Active
Source Idea Path: ideas/open/162_bir_publication_availability_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add BIR Annotation Records

# Current Packet

## Just Finished

Completed Step 2 for
`ideas/open/162_bir_publication_availability_annotation_schema.md`.
Added typed Route 4 BIR publication availability annotation records and
construction helpers without switching production consumers:

- `Route4PublicationAvailabilityStatus` with explicit available,
  unavailable, missing-block, missing-value, missing-publication,
  alternate-source, and no-match states.
- `Route4PublicationScope`, `Route4PublicationSourceKind`, and
  `Route4PublicationValueRole` vocabularies for target-neutral
  block/value-scoped annotation payloads.
- `Route4CurrentBlockPublicationRecord` for current-block value availability,
  source producer kind/reference, before-instruction boundary, and produced
  value identity.
- `Route4BlockEntryPublicationRecord` for successor block-entry PHI
  availability and destination value identity.
- `Route4PublicationValueRecord` for produced/current-block and
  consumed/block-entry value links back to the owning Route 4 record.
- Construction helpers:
  `route4_publication_source_kind(...)`,
  `route4_current_block_publication_record(...)`,
  `route4_block_entry_publication_record(...)`,
  `route4_current_block_publication_value_record(...)`, and
  `route4_block_entry_publication_value_record(...)`.

Extended `backend_prepared_lookup_helper_test.cpp` oracle coverage:

- Current-block available `%sum` publication now compares prepared,
  existing MIR, and Route 4 BIR record/value-link answers.
- Current-block missing/no-match and mismatched-type cases now check explicit
  Route 4 unavailable statuses instead of relying on absent records.
- Block-entry available PHI destination now compares prepared, existing MIR,
  and Route 4 BIR record/value-link answers.
- Block-entry missing-PHI and wrong-destination cases prove explicit Route 4
  missing-publication statuses.
- Block-entry storage-policy boundary remains covered: BIR Route 4 PHI
  availability stays semantic even when prepared destination-storage readiness
  is unavailable.

No MIR query consumer, target/codegen consumer, prealloc production helper, or
prepared publication record shape was switched or copied.

## Suggested Next

Execute Step 3 by adding function-local Route 4 lookup/index helpers over the
new BIR publication availability records, then prove current-block and
block-entry lookup answers are target-neutral indexes over BIR payloads.

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
- Step 2 intentionally did not switch production MIR, target, or prealloc
  consumers.
- Step 3 indexes should be rebuildable from Route 4 records and must not become
  prepared publication plan containers.

## Proof

Exact delegated Step 2 proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`1/1` matching backend test passed).
