Status: Active
Source Idea Path: ideas/open/161_bir_memory_access_identity_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate A Low-Risk Query Consumer

# Current Packet

## Just Finished

Completed Step 4 for
`ideas/open/161_bir_memory_access_identity_annotation_schema.md`.
Migrated the narrow shared MIR memory/access query consumers to read rebuilt
Route 3 BIR record/index helpers:

- `mir::find_bir_memory_access_identity(...)` now builds a Route 3 memory
  access index and converts `Route3MemoryAccessRecord` payloads into the public
  MIR identity result instead of reconstructing directly from raw BIR
  instruction/address fields.
- `mir::find_bir_same_block_global_load_access_identity(...)` now answers from
  Route 3 same-block global-load lookup records, preserving normalized request
  value name/type behavior and block-label fail-closed checks.
- `mir::find_bir_same_block_load_local_source_identity(...)` now answers from
  Route 3 same-block load-local source lookup records, including same-slot
  invalidation fail-closed behavior.
- Name-only same-block memory requests keep the previous no-explicit-type
  behavior through a Route 3 index-backed fallback scan rather than raw
  instruction reconstruction.

Updated the owned helper test expectations so prepared/prealloc oracle checks
continue to cover semantic memory identity while no longer requiring byte
offset, size, or align fields from Route 3-backed MIR memory identity.

No BIR schema file, AArch64 codegen production file, or prealloc production
helper was changed.

## Suggested Next

Execute Step 5 by broadening backend validation for the Route 3 memory/access
schema migration and preparing closure readiness.

## Watchouts

- Step 5 should run broad backend validation because Step 4 changed shared MIR
  query behavior used by downstream backend paths.
- Route 3-backed MIR memory identity intentionally does not provide byte
  offset, size, align, relocation/TLS details, addressing-mode legality, or
  AArch64 operand policy.
- Prepared/prealloc oracles remain responsible for target/layout-specific
  acceptance, including offset/range-sensitive overlap authority.

## Proof

Exact delegated proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_store_source_publication_plan|backend_aarch64_instruction_dispatch|backend_aarch64_prepared_scalar_alu_records)$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`4/4` matching backend tests passed).
