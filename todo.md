Status: Active
Source Idea Path: ideas/open/161_bir_memory_access_identity_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add BIR Annotation Records

# Current Packet

## Just Finished

Completed Step 2 for
`ideas/open/161_bir_memory_access_identity_annotation_schema.md`.
Added typed Route 3 BIR memory/access annotation records and construction
helpers without switching production consumers:

- `Route3MemoryAccessNodeKind` for `LoadLocal`, `LoadGlobal`, `StoreLocal`,
  and `StoreGlobal` instruction identity.
- `Route3MemoryAccessBaseKind` for semantic BIR base identity:
  `LocalSlot`, `GlobalSymbol`, `PointerValue`, and `StringConstant`.
- `Route3MemoryAccessRecord` for instruction identity, block identity,
  result/stored value links, address-space, volatile flag, and semantic
  local/global/pointer/string base identity.
- `Route3MemoryAccessValueRecord` for result-value and stored-value links back
  to the owning memory access record.
- `Route3SameBlockGlobalLoadAccessRecord` and
  `Route3SameBlockLoadLocalSourceRecord` for Step 2 same-block oracle checks,
  including present-negative states where a named root has no accepted source.
- Construction helpers:
  `route3_memory_access_node_kind(...)`,
  `route3_memory_access_base_kind(...)`,
  `route3_memory_access_record(...)`,
  `route3_memory_access_result_value_record(...)`,
  `route3_memory_access_stored_value_record(...)`,
  `route3_same_block_global_load_access_record(...)`, and
  `route3_same_block_load_local_source_record(...)`.

Extended narrow oracle coverage:

- `backend_aarch64_prepared_memory_operand_records_test.cpp` now compares direct
  memory identity and same-block global-load answers against Route 3 BIR
  records in addition to the existing prepared/MIR oracle checks.
- `backend_store_source_publication_plan_test.cpp` now compares same-block
  load-local source answers against Route 3 BIR records in addition to the
  existing prepared/MIR oracle checks.

No production MIR query consumer, AArch64 codegen production file, or prealloc
production helper was switched.

## Suggested Next

Execute Step 3 by adding function-local Route 3 lookup/index helpers over the
new BIR annotation records, then prove same-block global-load and load-local
source lookups are target-neutral indexes over BIR payloads.

## Watchouts

- Keep Route 3 annotations target-neutral and semantic.
- Do not import frame slot ids, byte offsets, size/align layout, relocation
  spelling, TLS register details, addressing-mode legality, or AArch64 memory
  operand formation.
- Do not copy `PreparedAddress` or `PreparedMemoryAccess` wholesale as the BIR
  schema shape.
- Preserve explicit negative cases for volatile, address-space, local-source,
  and no-source behavior where applicable.
- Keep Route 3 semantic identity separate from target addressing policy:
  frame slot ids, byte offsets, size/align, relocation spelling, TLS register
  details, and AArch64 memory operand formation remain out of scope.
- The Step 2 BIR schema may reuse `bir::MemoryAddress` as construction input,
  but the durable annotation should be a Route 3 field-level semantic record,
  not the whole `MemoryAddress` or prepared carrier copied through.
- Step 2 intentionally keeps byte offsets, size/align, frame slot ids,
  relocation spelling, TLS register details, addressing-mode legality, and
  AArch64 operand fields out of the Route 3 records.
- Step 2 same-block load-local source records use same-slot invalidation only;
  range/offset-sensitive overlap authority remains with prepared/prealloc
  oracles and must not be smuggled into the BIR schema.

## Proof

Exact delegated proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_store_source_publication_plan)$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log`.
