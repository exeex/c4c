Status: Active
Source Idea Path: ideas/open/290_gate_quarantined_opaque_compatibility_memory_accesses.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Quarantined Compatibility Consumers

# Current Packet

## Just Finished

Step 1: Inventory Quarantined Compatibility Consumers completed as an
inventory-only packet.

Read-only inspection commands used:

- `git status --short`
- `sed -n '1,260p' AGENTS.md`
- `sed -n '1,260p' todo.md`
- `sed -n '1,220p' plan.md`
- `sed -n '1,260p' review/idea289_step6_route_review.md`
- `rg --files src/backend/prealloc tests/backend/bir`
- `command -v c4c-clang-tool && command -v c4c-clang-tool-ccdb`
- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/bir/lir_to_bir/memory/provenance.cpp build/compile_commands.json`
- `c4c-clang-tool function-signatures src/backend/bir/lir_to_bir/memory/memory_types.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/publication_plans.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/prepared_lookups.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/prealloc/prepared_lookups.cpp apply_source_memory_access_fact build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/prealloc/prepared_lookups.cpp copy_source_memory_access_fact build/compile_commands.json`
- targeted `rg` and `sed` reads of the allowed memory, prealloc, test, and
  review files for `layout_authority`, `OpaqueCompatibility`,
  `UnknownCompatible`, `can_use_base_plus_offset`, `source_memory_*`, and
  prepared memory-access matching.

Current quarantine publishers and carriers:

- `src/backend/bir/lir_to_bir/memory/provenance.cpp` classifies scalar
  subobject addressability as `Accepted`, `Rejected`, or
  `OpaqueCompatibility`. The opaque cases are the old
  `allow_opaque_ptr_base && stored_type == I8` admission and zero-offset
  empty/`ptr`/`i8` spelling bridge.
- `pointer_value_memory_provenance_with_layout_authority` is the explicit
  publisher for quarantine metadata. When addressability is
  `OpaqueCompatibility`, it sets
  `layout_authority = MemoryLayoutAuthorityKind::OpaqueCompatibility` and
  forces `range_verdict = MemoryRangeVerdict::UnknownCompatible`.
- `try_lower_addressed_pointer_store` and
  `try_lower_addressed_pointer_load` are target-independent lowerer consumers
  that currently accept `OpaqueCompatibility` because they only reject
  `Rejected`; they then emit prepared source-memory rows carrying the explicit
  quarantine metadata.
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp` carries provenance in
  `PointerAddress` and dynamic pointer-array state, but it does not itself
  gate quarantine metadata.

Prepared target-independent consumers found:

- `src/backend/prealloc/prepared_lookups.cpp::copy_source_memory_access_fact`
  flattens source-memory facts from `PreparedMemoryAccess` into
  `PreparedEdgePublication`, including
  `source_memory_can_use_base_plus_offset`,
  `source_memory_layout_authority`, `source_memory_range_verdict`,
  `source_memory_dynamic_array_verdict`, and
  `source_memory_requires_address_materialization`.
- `src/backend/prealloc/prepared_lookups.cpp::apply_source_memory_access_fact`
  publishes source-memory facts for `LoadLocal` source producers and marks the
  row `Available` if the result name, complete base, size, and alignment match.
  It does not currently reject `OpaqueCompatibility` or `UnknownCompatible`.
- `src/backend/prealloc/publication_plans.hpp` stores the same flattened
  source-memory fields on both `PreparedEdgePublication` and
  `PreparedEdgeCopySourceFacts`.
- `src/backend/prealloc/publication_plans.cpp::copy_prepared_edge_copy_source_fact_fields`
  copies the flattened source-memory fields into `PreparedEdgeCopySourceFacts`.
- `src/backend/prealloc/publication_plans.cpp::prepared_edge_publication_source_memory_matches_access`
  already strictly compares the source value, base identity fields, byte range,
  address space, volatility, `can_use_base_plus_offset`, `layout_authority`,
  `range_verdict`, and dynamic-array verdict. This is a usable
  target-independent policy/matching surface.
- `src/backend/prealloc/publication_plans.cpp::validate_prepared_edge_copy_publication_source_facts`
  only rejects missing or incomplete load-local source-memory facts. It does
  not currently reject quarantine metadata.
- `src/backend/prealloc/publication_plans.cpp::prepared_store_source_load_local_is_byval_formal_pointer_source`
  and `src/backend/prealloc/prepared_lookups.cpp::find_prepared_global_load_access`
  still rely on `can_use_base_plus_offset` for materializable-address decisions.
  They must not become quarantine gates unless paired with explicit
  `layout_authority == OpaqueCompatibility`.
- Prepared memory-access lookup helpers index rows by block/instruction
  position, result value name, and result value id. They preserve duplicate and
  stale rows for consumers to reject; they do not inspect quarantine metadata.

Target-specific or presentation consumers found:

- The RISC-V prepared edge-publication tests exercise a target consumer that
  accepts a shared source-memory publication when base-plus-offset facts are
  materializable and rejects stale public `memory_accesses` rows. The target
  source implementation was outside this packet's read-only file list, so this
  is classified from tests only and is not the preferred policy surface.
- `src/backend/prealloc/prepared_printer/addressing.cpp` prints
  `layout_authority`, `range_verdict`, and dynamic-array verdict. This is a
  presentation consumer, not an acceptance policy surface.
- `src/backend/prealloc/addressing.hpp` provides string names for memory
  verdicts and layout authority. It is diagnostic support only.

Candidate policy surfaces:

- Preferred Step 2 candidate: lowerer-time rejection in
  `try_lower_addressed_pointer_load` and `try_lower_addressed_pointer_store`
  when `classify_scalar_subobject_addressability(...) ==
  ScalarSubobjectAddressability::OpaqueCompatibility`. This is
  target-independent, uses the explicit opaque-compatibility fact before
  publication, and avoids conflating generic `UnknownCompatible` with opaque
  compatibility.
- Viable prepared/publication candidate:
  `apply_source_memory_access_fact` or
  `validate_prepared_edge_copy_publication_source_facts` can reject/gate a
  load-local source-memory publication when
  `source_memory_layout_authority == OpaqueCompatibility`. The current
  flattened fact already exists, and the strict matcher already compares it.
- Viable matcher/consumer candidate:
  `prepared_edge_publication_source_memory_matches_access` can continue to be
  the stale-row agreement surface because it already compares
  `source_memory_layout_authority`; a behavior gate could be layered by callers
  only if they use that explicit field.
- Avoid as primary policy: target-specific prepared consumers and helpers that
  only inspect `can_use_base_plus_offset` or `UnknownCompatible`.

Matching-field implications:

- No new flattened opaque-compatibility fact appears necessary if the next
  packet gates on existing `source_memory_layout_authority`.
- If Step 2 chooses a new boolean or enum fact anyway, it must be added to both
  `PreparedEdgePublication` and `PreparedEdgeCopySourceFacts`, copied in
  `copy_source_memory_access_fact` and
  `copy_prepared_edge_copy_source_fact_fields`, and compared in
  `prepared_edge_publication_source_memory_matches_access` before any consumer
  uses it for acceptance.
- Existing strict matching already covers stale-row drift for
  `layout_authority`, `range_verdict`, byte offset, size, alignment,
  base-kind identity, address space, volatility, and
  `can_use_base_plus_offset`.

Existing focused tests:

- `backend_prepared_lookup_helper`:
  `verify_edge_publication_source_producer_facts` verifies publication of
  `source_memory_layout_authority == OpaqueCompatibility`, range verdict, and
  dynamic-array verdict, and verifies
  `prepared_edge_publication_source_memory_matches_access` rejects mismatched
  layout authority, range verdict, offset, and unnamed source rows.
- `backend_riscv_prepared_edge_publication`:
  `check_load_local_dynamic_stack_source_exposes_shared_memory_access` covers
  shared source-memory acceptance, current-position row selection, stale public
  memory row preservation, and target-consumer rejection of stale public rows.
- `backend_prepared_printer` covers printing
  `layout_authority=opaque_compatibility`, range verdict, and dynamic-array
  verdict for prepared memory accesses.
- `backend_lir_to_bir_notes` covers generic requested-range verdicts,
  including `UnknownCompatible`, but not the addressed pointer opaque bridge
  gate itself.

Missing focused tests for Step 2/3:

- A lowerer-level supported-path test that an addressed pointer load/store row
  with `layout_authority == OpaqueCompatibility` is rejected, gated, or
  diagnosed at the selected surface.
- A paired accepted structured row test proving `StructuredLayout`,
  `ScalarLayout`, `ByteStorageAggregate`, or other non-opaque proven in-range
  rows are still accepted.
- If using the prepared/publication surface, a focused stale-row mismatch test
  that changes only `source_memory_layout_authority` or an equivalent new fact
  and proves the consumer rejects it.
- If using a target consumer, a focused target-specific test must prove it is
  driven by explicit `layout_authority == OpaqueCompatibility`, not by
  `UnknownCompatible` or `can_use_base_plus_offset` alone.

## Suggested Next

Step 2 should choose the first behavior-changing policy surface. Recommended
packet: specify lowerer-time rejection in
`try_lower_addressed_pointer_load`/`try_lower_addressed_pointer_store` for
`ScalarSubobjectAddressability::OpaqueCompatibility`, with tests that prove
structured proven in-range rows remain accepted and opaque-compatibility rows
fail closed.

## Watchouts

- Do not gate on `UnknownCompatible` or `can_use_base_plus_offset` without
  checking explicit `layout_authority == OpaqueCompatibility` or an equivalent
  flattened opaque-compatibility fact.
- Do not weaken stale-row, duplicate-row, or cross-block prepared
  `memory_accesses` rejection.
- Do not reject structured proven in-range rows by conflating quarantine
  metadata with general unknown extent.
- Do not use target-specific prepared/prealloc code as the main policy surface
  while target-independent provenance facts are available.
- The transient idea 289 review report says flattened `layout_authority` was
  missing at that time, but the current code now has
  `source_memory_layout_authority` on publication and copy-source facts and
  compares it in the strict source-memory matcher.

## Proof

No build or test run; proof was explicitly not required for this
inventory-only `todo.md` update. No root-level logs were created.

Proposed focused Step 2/3 proof subset after code/test edits:

- Build: `cmake --build build --target backend_lir_to_bir_notes_test backend_prepared_lookup_helper_test backend_riscv_prepared_edge_publication_test backend_prepared_printer_test`
- Focused tests: `ctest --test-dir build -R '^(backend_lir_to_bir_notes|backend_prepared_lookup_helper|backend_riscv_prepared_edge_publication|backend_prepared_printer)$' --output-on-failure`
- If Step 3 chooses the lowerer-time gate and adds coverage outside
  `backend_lir_to_bir_notes`, include the exact new owning CTest target in the
  same focused subset.
