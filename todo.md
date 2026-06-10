Status: Active
Source Idea Path: ideas/open/161_bir_memory_access_identity_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Lookup/Index Helpers

# Current Packet

## Just Finished

Completed Step 3 for
`ideas/open/161_bir_memory_access_identity_annotation_schema.md`.
Added function-local Route 3 BIR memory/access lookup/index helpers without
switching production consumers:

- `Route3MemoryAccessIndex` and `route3_build_memory_access_index(...)` rebuild
  a block-local memory/access index from Route 3 BIR annotation records.
- `Route3MemoryAccessQuery` carries the function-local index plus
  same-block-before boundary for lookup helpers.
- `route3_find_memory_access_record(...)` looks up direct memory access identity
  by instruction index and Route 3 node kind.
- `route3_find_same_block_global_load_access(...)` answers the same-block
  global-load source use case from indexed Route 3 memory/access records.
- `route3_find_same_block_load_local_source(...)` answers the same-block
  load-local source use case from indexed Route 3 memory/access records and
  preserves the existing BIR fail-closed stance for same-slot intervening
  stores without layout-overlap authority.

Extended narrow oracle coverage in the owned tests:

- Direct memory lookup/index answers now match prepared/MIR oracle facts and
  fail closed for wrong Route 3 node kind while preserving address-space and
  volatile fields in indexed records.
- Same-block global-load lookup answers now match the prepared/MIR/direct Route
  3 oracle path and fail closed for before-boundary, type mismatch, and
  non-global string-base loads.
- Same-block load-local source lookup answers now match the prepared/MIR/direct
  Route 3 oracle path and fail closed for before-boundary, type mismatch,
  same-slot invalidation, and no indexed memory/source authority.

No production MIR query consumer, AArch64 codegen production file, or prealloc
production helper was switched.

## Suggested Next

Execute Step 4 by migrating the narrow shared memory/access query consumer to
read Route 3 BIR lookup/index records while preserving prepared/prealloc oracle
checks in tests.

## Watchouts

- Keep Step 4 target-neutral: shared MIR query migration may consume Route 3
  BIR records, but must not import AArch64 addressing policy or prepared-only
  layout authority.
- Preserve the existing oracle comparison shape: prepared/prealloc answers
  remain the test oracle while Route 3 proves it can carry the BIR identity
  payload.
- Same-block load-local source BIR lookup intentionally fails closed for any
  same-slot intervening store; offset/range-sensitive acceptance remains
  prepared/prealloc authority until a separate semantic overlap schema exists.
- Route 3 records/index helpers remain semantic: no frame slot ids, byte
  offsets, size/align layout, relocation spelling, TLS register details,
  addressing-mode legality, or AArch64 operand fields were added.

## Proof

Exact delegated proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_store_source_publication_plan)$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`2/2` matching backend tests passed).
