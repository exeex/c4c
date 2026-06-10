Status: Active
Source Idea Path: ideas/open/161_bir_memory_access_identity_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Memory Access Surfaces

# Current Packet

## Just Finished

Completed Step 1 for
`ideas/open/161_bir_memory_access_identity_annotation_schema.md`.
Inventoried the Route 3 memory/access identity surfaces and recorded concrete
Step 2 schema targets.

Prepared query surfaces to preserve as migration oracles:

- `prepare::PreparedMemoryAccess` in `src/backend/prealloc/addressing.hpp`:
  `function_name`, `block_label`, `inst_index`, optional
  `result_value_name`, optional `stored_value_name`, `address_space`,
  `is_volatile`, and `PreparedAddress`.
- Prepared access lookup helpers:
  `find_prepared_memory_access(...)`,
  `find_prepared_memory_access_by_result_name(...)`,
  `find_prepared_memory_access_by_result_value_name(...)`,
  `find_prepared_memory_access_before_by_result_value_name(...)`,
  `make_prepared_memory_access_lookups(...)`,
  `find_indexed_prepared_memory_access(...)`,
  `find_unique_indexed_prepared_memory_access_by_result_value_name(...)`, and
  `find_unique_indexed_prepared_memory_access_by_result_value_id(...)`.
- Same-block global-load helpers:
  `find_prepared_global_load_access(...)` and
  `find_prepared_same_block_global_load_access(...)`.
- Same-block load-local helpers:
  `find_prepared_same_block_load_local_stored_value_source(...)` in
  `prepared_lookups.cpp` and
  `find_prepared_same_block_load_local_source_producer(...)` in
  `publication_plans.cpp`.

BIR/MIR query surfaces to mirror first:

- Existing BIR payload source: `bir::MemoryAddress`, `LoadLocalInst`,
  `LoadGlobalInst`, `StoreLocalInst`, and `StoreGlobalInst` in
  `src/backend/bir/bir.hpp`.
- Existing shared MIR answers in `src/backend/mir/query.*`:
  `BirMemoryAccessIdentityRequest`,
  `BirMemoryAccessIdentity`,
  `find_bir_memory_access_identity(...)`,
  `BirSameBlockGlobalLoadAccessRequest`,
  `BirSameBlockGlobalLoadAccessIdentity`,
  `find_bir_same_block_global_load_access_identity(...)`,
  `BirSameBlockLoadLocalSourceRequest`,
  `BirSameBlockLoadLocalSourceIdentity`, and
  `find_bir_same_block_load_local_source_identity(...)`.

Concrete BIR instruction annotation fields for Step 2:

- `Route3MemoryAccessNodeKind`: `Unknown`, `LoadLocal`, `LoadGlobal`,
  `StoreLocal`, `StoreGlobal`.
- `Route3MemoryAccessBaseKind`: `None`, `LocalSlot`, `GlobalSymbol`,
  `PointerValue`, `StringConstant`; keep `Label` out of the accepted Route 3
  memory schema unless later evidence justifies it.
- `Route3MemoryAccessRecord`: `available`, `instruction`,
  `instruction_index`, `node_kind`, `block_label`, `block_label_id`,
  optional result-value identity, optional stored-value identity,
  `address_space`, `is_volatile`, `base_kind`, local slot display/id,
  global display/link id, pointer base value identity, and string constant
  display/link id.
- Do not copy `PreparedAddress` or `PreparedMemoryAccess`, and do not add frame
  slot ids, byte offsets, size/align layout, target addressing legality,
  relocation spelling, TLS register details, or AArch64 memory operand fields.

Concrete BIR value annotation fields for Step 2:

- Result-value link for load accesses: value pointer/name/type plus owning
  access instruction index and `Route3MemoryAccessNodeKind`.
- Stored-value link for store accesses: value pointer/name/type plus owning
  access instruction index and `Route3MemoryAccessNodeKind`.
- Same-block load-local source link: loaded value identity, load-local
  instruction index, load-local access record, `source_available` flag,
  optional source store instruction index, optional stored value identity, and
  optional store-local access record. Present-negative no-source state must be
  distinguishable from absent annotation.

Optional lookup/index shape for Step 3, with Step 2 records designed to support
it:

- `Route3MemoryAccessIndex { const Block* block; vector<Route3MemoryAccessRecord> records; }`
  rebuilt from BIR instruction annotations.
- Same-block global-load lookup: query by root value name/type and
  before-instruction index, require matching `LoadGlobal`, `GlobalSymbol`
  base, result value identity, address-space, and volatile facts.
- Same-block load-local source lookup: query by root value name/type and
  before-instruction index, require matching `LoadLocal`, `LocalSlot` base,
  result value identity, and no same-slot invalidating store under the
  target-neutral Route 3 rules. Range/offset-sensitive overlap policy remains
  a prepared/prealloc oracle, not a BIR schema field.

Positive oracle cases to cover:

- Direct memory identity for `LoadLocal` frame-slot/local-slot,
  `LoadGlobal` global-symbol and string-constant bases, `StoreLocal`
  pointer-value and local-slot bases, and `StoreGlobal` global-symbol bases.
- Address-space and volatility preservation on direct memory identity.
- Same-block global-load success for a `LoadGlobal` with `GlobalSymbol` base.
- Same-block load-local source success for a `LoadLocal` with indexed memory
  access/source-producer authority and no intervening same-slot store.

Negative oracle cases to cover:

- Missing prepared memory access, missing structured BIR address, wrong node
  kind, wrong block label, before-producer query, root type mismatch, and
  missing root.
- Result-value mismatch, stored-value mismatch, global symbol mismatch,
  pointer value mismatch, string identity mismatch, address-space mismatch, and
  volatility mismatch.
- Same-block global-load fail-closed for non-global roots and string-constant
  loads.
- Same-block load-local source fail-closed for no indexed memory access,
  missing source-producer authority, root type mismatch, before-producer query,
  and same-slot intervening store. Preserve the existing prepared oracle for
  non-overlapping stores without encoding byte-range layout into BIR.

First code-changing Step 2 targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
- Use `tests/backend/mir/backend_store_source_publication_plan_test.cpp` as the
  secondary narrow target if Step 2 includes the load-local source record
  immediately.

## Suggested Next

Execute Step 2 by adding Route 3 BIR annotation records and construction
helpers in `src/backend/bir/bir.hpp` and `src/backend/bir/bir.cpp`, then extend
the narrow oracle tests without switching production consumers.

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

## Proof

Inventory/status-only packet. No build required.

Step 2 narrow proof command to delegate:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_store_source_publication_plan)$'
```

Additional local validation for this packet: `git diff --check`.
