Status: Active
Source Idea Path: ideas/open/289_structured_opaque_pointer_byte_range_provenance.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Current Opaque Pointer Provenance Routes

# Current Packet

## Just Finished

Step 1: Inventory Current Opaque Pointer Provenance Routes completed.

Current route inventory:

- `can_address_scalar_subobject()` in
  `src/backend/bir/lir_to_bir/memory/provenance.cpp` is the current
  compatibility gate. It rejects negative offsets and zero-sized access types,
  accepts exact scalar type matches, accepts scalar `i8` inspection/update when
  the access is in the stored scalar object, accepts byte-storage aggregate
  coverage through rendered type text, and finally accepts scalar-layout facts
  resolved from rendered type text.
- The exact opaque bridge is the stored-scalar overflow case:
  `allow_opaque_ptr_base && stored_type == I8`. This only runs when
  `stored_type != Void`, the stored scalar size is unknown or the requested
  `byte_offset + access_size` exceeds that scalar size, and the caller passed
  `allow_opaque_ptr_base=true`.
- The only current callers passing `allow_opaque_ptr_base=true` are
  `try_lower_addressed_pointer_store()` and `try_lower_addressed_pointer_load()`
  for `PointerAddressMap` entries. Local-slot pointer provenance and linear
  addressed globals pass `false`, so they do not use the opaque `I8` bridge.
- A separate zero-offset opaque runtime pointer rule accepts
  `allow_opaque_ptr_base && byte_offset == 0 &&
  (type_text.empty() || type_text == "ptr" || type_text == "i8")`; this is
  distinct from the `stored_type == I8` overflow bridge but feeds the same
  pointer-value load/store lowering path.
- Pointer-address producers live mainly in
  `src/backend/bir/lir_to_bir/memory/addressing.cpp`,
  `src/backend/bir/lir_to_bir/memory/value_materialization.cpp`,
  `src/backend/bir/lir_to_bir/memory/provenance.cpp`, and pointer-param / phi
  handling in `src/backend/bir/lir_to_bir/module.cpp`. They populate route-local
  `PointerAddress`, `LocalSlotAddress`, global-address maps, and dynamic
  pointer-array side maps, then lower prepared-visible memory through ordinary
  BIR `MemoryAddress`.

Facts currently published for prepared `memory_accesses`:

- BIR Route 3 records publish instruction kind, block, instruction index,
  result/stored value identity, address space, volatility, base kind, byte
  offset, size, alignment, and base identity for local slots, globals,
  pointer values, or string constants.
- `src/backend/prealloc/stack_layout/coordinator.cpp` converts these into
  `PreparedMemoryAccess`: function name, block label, instruction index,
  optional result/stored value name, address space, volatility, and
  `PreparedAddress`.
- `PreparedAddress` currently carries `base_kind`, `frame_slot_id`,
  `symbol_name`, `pointer_value_name`, global materialization policy,
  `byte_offset`, `size_bytes`, `align_bytes`, and
  `can_use_base_plus_offset`.
- Pointer-value prepared accesses are admitted by
  `build_pointer_indirect_address()` when the BIR address is
  `MemoryAddress::BaseKind::PointerValue` with a named pointer base; it always
  sets `can_use_base_plus_offset=true`.
- Prepared consumers copy/compare only those existing fields. Examples:
  `copy_source_memory_access_fact()` publishes the source memory base, offset,
  size, alignment, address space, volatility, and
  `source_memory_can_use_base_plus_offset`; same-block and publication-plan
  helpers require `can_use_base_plus_offset` but have no structured provenance
  verdict to consult.

Structured facts already present upstream but not published as prepared
provenance:

- Base identity exists as rendered/route-local values: pointer SSA names via
  `PointerAddress::base_value`, local slot spellings in `LocalSlotAddress`,
  globals through `GlobalAddress` plus `LinkNameId` where available, pointer
  params seeded in `module.cpp`, and phi-merged runtime pointers when the
  address shape agrees.
- Some object extent/layout authority exists before prepared publication:
  `GlobalInfo::storage_size_bytes`, `known_global_address`,
  `pointer_initializer_offsets`, aggregate layouts from structured layouts or
  `TypeDeclMap`, scalar layout facts from
  `resolve_scalar_layout_facts_at_byte_offset()`, local aggregate leaf slots,
  and local/global array extents.
- Dynamic range ingredients exist in side maps:
  `DynamicPointerValueArrayAccess`, `DynamicLocalPointerArrayAccess`,
  `DynamicGlobalPointerArrayAccess`,
  `DynamicGlobalAggregateArrayAccess`, and
  `DynamicGlobalScalarArrayAccess` carry element count, stride, base offset,
  and dynamic index values.

Missing structured facts:

- No prepared memory-access field records base identity kind independently of
  the printable address base; unknown runtime bases are implicit rather than
  explicit.
- No prepared field records object extent, completeness, requested byte-range
  begin/end, overflow status, or whether the range is proven in-bounds,
  unknown-compatible, or fail-closed.
- No prepared field records layout authority: structured layout, scalar layout,
  byte-storage aggregate, rendered-type fallback, or explicit unknown.
- Dynamic array element count, stride, selected index, and bounded/unbounded
  verdict are consumed before publication and lost in prepared
  `memory_accesses`.
- `can_use_base_plus_offset` is only syntactic address usability. It is not
  object-range proof and is currently used by consumers as a gate because no
  separate provenance/range carrier exists.

Existing focused coverage and gaps:

- `backend_lir_to_bir_notes` is the natural lowering-level bucket for opaque
  pointer byte-offset behavior and BIR route notes.
- `backend_prepared_lookup_helper` covers prepared memory lookup/indexing
  behavior.
- `backend_riscv_prepared_edge_publication` already covers pointer-value memory
  source publication, stale prepared rows, duplicate/current-vs-stale lookup
  behavior, Route 3/Route 5 agreement, and fail-closed prepared-publication
  consumers.
- `backend_prepared_printer` exposes prepared addressing text, including
  `can_use_base_plus_offset`, but not provenance/range facts because they do
  not exist yet.
- `backend_aarch64_instruction_dispatch` has pointer-value prepared memory
  consumers, byte-offset dispatch checks, stale-source protections, dynamic
  pointer reload/GEP behavior, and fail-closed carrier-authority tests.
- Backend case tests such as `global_char_pointer_diff.c`,
  `variadic_double_bytes.c`, `aarch64_local_aggregate_address_pointer_copy.c`,
  byte-oriented builtin memset/memcpy cases, and dynamic member-array cases
  exercise adjacent byte/dynamic access behavior, but there is no focused test
  yet for a prepared structured provenance verdict, unknown extent verdict,
  overflowed requested range, or out-of-range opaque pointer byte access.

## Suggested Next

Begin Step 2 by adding neutral structured provenance/range carrier fields to
`memory_types.hpp` and the prepared memory-access route without changing
observable behavior: represent base identity, object extent, requested range,
layout authority, dynamic-array facts, and an initial explicit unknown or
compatibility verdict while leaving existing `can_use_base_plus_offset`
consumers unchanged.

## Watchouts

- Do not remove the `allow_opaque_ptr_base && stored_type == I8`
  compatibility rule until every required route has structured range facts or
  an explicit unknown/fail-closed verdict.
- Do not treat `can_use_base_plus_offset` as object-range proof.
- Do not add target-specific prepared/prealloc acceptance that bypasses
  target-independent BIR provenance facts.
- Do not weaken stale-row, duplicate-row, cross-block, dynamic-range, overflow,
  or out-of-range rejection to make a narrow testcase pass.
- Keep `can_use_base_plus_offset` separate from range proof; next-step code
  should publish a neutral carrier first, then populate/prove it route by
  route.
- Pointer-value dynamic-array facts currently disappear when selected element
  loads/stores are lowered to ordinary BIR memory operations. Preserve those
  facts before relying on prepared consumers for dynamic-range decisions.
- Existing prepared-publication stale-row tests are relevant regression guards;
  new provenance fields should participate in lookup/copy equality only when
  the route deliberately publishes them.

## Proof

No build required by packet; no `test_after.log` created.

Inspection commands run for confidence:

- `git status --short`
- `command -v c4c-clang-tool && command -v c4c-clang-tool-ccdb`
- `c4c-clang-tool-ccdb list-symbols
  /workspaces/c4c/src/backend/bir/lir_to_bir/memory/provenance.cpp
  build/compile_commands.json`
- `c4c-clang-tool function-signatures
  src/backend/bir/lir_to_bir/memory/memory_types.hpp -- --std=c++17
  -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir
  -I/workspaces/c4c/src/frontend/parser`
- Focused `sed`/`rg` inspection over
  `src/backend/bir/lir_to_bir/memory/*.cpp`,
  `src/backend/bir/lir_to_bir/memory/memory_types.hpp`,
  `src/backend/bir/lir_to_bir/module.cpp`,
  `src/backend/bir/bir.cpp`,
  `src/backend/prealloc/addressing.hpp`,
  `src/backend/prealloc/stack_layout/coordinator.cpp`,
  `src/backend/prealloc/prepared_lookups.cpp`,
  `src/backend/prealloc/publication_plans.cpp`, and focused backend tests.

Initial focused proof subset for later code-changing steps:

`cmake --build build --target backend_lir_to_bir_notes_test
backend_prepared_lookup_helper_test backend_riscv_prepared_edge_publication_test
backend_prepared_printer_test backend_aarch64_instruction_dispatch_test && ctest
--test-dir build -R
'backend_lir_to_bir_notes|backend_prepared_lookup_helper|backend_riscv_prepared_edge_publication|backend_prepared_printer|backend_aarch64_instruction_dispatch'
--output-on-failure`
