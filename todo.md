Status: Active
Source Idea Path: ideas/open/163_bir_edge_join_source_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add BIR Annotation Records

# Current Packet

## Just Finished

Completed Step 2 for
`ideas/open/163_bir_edge_join_source_annotation_schema.md`.
Added typed Route 5 BIR edge and current-block join-source annotation records
and construction helpers without switching production consumers:

- `Route5PublicationStatus`, `Route5PublicationScope`,
  `Route5PublicationSourceKind`, and `Route5PublicationValueRole` provide
  target-neutral vocabulary for edge and join-source semantic facts.
- `Route5CfgEdgePublicationRecord` captures predecessor/successor labels,
  destination value identity, source value identity, source producer kind,
  source producer instruction/block identity, explicit no-source status, and
  optional Route 3 memory-source identity.
- `Route5CurrentBlockJoinSourceRecord` captures current-block join PHI
  incoming source identity, destination value identity, predecessor/successor
  labels, and same-block source producer identity.
- `Route5PublicationValueRecord` links edge/join records back to destination
  and source value roles.
- Construction helpers:
  `route5_publication_source_kind(...)`,
  `route5_cfg_edge_publication_record(...)`,
  `route5_current_block_join_source_records(...)`,
  `route5_edge_destination_value_record(...)`,
  `route5_edge_source_value_record(...)`,
  `route5_join_destination_value_record(...)`, and
  `route5_join_source_value_record(...)`.

Focused `backend_prepared_lookup_helper_test.cpp` coverage now proves:

- Edge-copy source positive records for load-local memory-source and named
  non-memory source cases.
- Route 5 memory-source records reuse Route 3 memory access identity and match
  the existing prepared/MIR load-local edge oracle.
- Edge value-link records expose destination and source value roles.
- Edge fail-closed records explicitly represent missing-source, wrong-edge
  no-source, and missing-destination cases.
- Current-block join-source records expose named, immediate, and stack-source
  PHI incoming identities.
- Join value-link records expose destination and source value roles.
- Join fail-closed records explicitly represent no-PHI missing-publication and
  missing source producer cases.

No MIR, target/codegen, or prealloc production consumer was switched.

## Suggested Next

Execute Step 3 by adding function-local Route 5 lookup/index helpers over the
new BIR edge and join-source annotation records.

## Watchouts

- Keep Route 5 annotations target-neutral and semantic: edge and block records
  own CFG/value/source identity, not parallel-copy execution policy.
- Do not import move bundle order, parallel-copy step order, cycle temporary
  routing, execution site/block placement, phase or carrier policy, coalescing
  or redundancy flags, destination registers, storage-sharing checks,
  `PreparedEdgePublication`, or prepared move records.
- Do not import target/prealloc storage details as schema: destination homes,
  destination storage kind, destination registers, source/destination storage
  sharing, register views, stack-source extension policy, and aggregate copy
  authority remain prepared/target policy.
- Explicit no-source and memory-source records must be representable as
  semantic identities; absence alone should not stand in for unavailable or
  no-source when an edge/join identity exists.
- Step 2 intentionally did not switch
  `mir::find_bir_cfg_edge_publication_source_identity(...)` or
  `mir::find_bir_current_block_join_source_identity(...)`.
- Step 3 indexes should be rebuildable from Route 5 BIR records and must not
  become prepared edge-publication lookup containers.

## Proof

Exact delegated Step 2 proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`1/1` matching backend test passed).
