Status: Active
Source Idea Path: ideas/open/631_direct_global_symbol_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace Direct Global-Symbol Authority Carriers

# Current Packet

## Just Finished

Step 2 traced the direct `global_symbol` local-memory carrier for
`src/pr46309.c`. The two Step 1 accesses remain:
`bar/block_1/inst_index=0 result=%t15 symbol=q offset=0 size=8 align=8` and
`main/entry/inst_index=7 stored=%lv.y symbol=q offset=0 size=8 align=8`, both
`base_plus_offset=yes layout_authority=scalar_layout range=proven_in_bounds`.

Producer functions and carrier fields:
- `build_direct_symbol_backed_access(..., const bir::LoadLocalInst&)` and
  `build_direct_symbol_backed_access(..., const bir::StoreLocalInst&)` in
  `src/backend/prealloc/stack_layout/coordinator.cpp` construct the
  `PreparedMemoryAccess` rows with `result_value_name` or `stored_value_name`,
  `address_space=prepared_memory_address_space(inst.address)`, `is_volatile`,
  and `.address = build_direct_symbol_backed_address(...)`.
- `build_direct_symbol_backed_address` resolves global identity through
  `resolve_prepared_global_symbol_address` and carries the link-name identity in
  `PreparedAddress.symbol_name` plus
  `PreparedAddress.provenance.base_identity.kind=GlobalSymbol` and
  `base_identity.link_name_id`.
- The same `PreparedAddress` carries `global_address_materialization_policy`,
  `byte_offset`, `size_bytes`, `align_bytes`, `can_use_base_plus_offset=true`,
  `provenance.object_extent` from the resolved global size, and
  `provenance.requested_range` from `prepared_memory_provenance`.
- `publish_scalar_global_layout_authority` is the scalar layout authority
  producer for `q`; it requires a resolved non-extern, non-TLS scalar global
  with complete extent, matching global link-name identity,
  `can_use_base_plus_offset`, known size/alignment, and
  `range_verdict=ProvenInBounds` before setting
  `provenance.layout_authority=ScalarLayout`.
- Addressing mode is present as
  `PreparedAddress.global_address_materialization_policy`; for this static
  direct-global route it resolves to direct materialization. Address space is
  present on `PreparedMemoryAccess.address_space` and is default for the
  `pr46309.c` rows.

Prepared authority check already exists:
`prepared_global_symbol_memory_has_publication_authority` in
`src/backend/prealloc/addressing.hpp` accepts exactly a `GlobalSymbol` address
with `symbol_name`, `can_use_base_plus_offset`, nonzero size/alignment, global
base identity, concrete non-opaque layout authority, complete known extent, and
a requested range matching `byte_offset`/`size_bytes` with
`range_verdict=ProvenInBounds`. The `pr46309.c` accesses satisfy the visible
scalar subset of this contract.

Current RV64 consumer checks/rejection points:
- `prepared_memory_access_for_local_instruction` in
  `src/backend/mir/riscv/codegen/object_emission.cpp` selects the prepared
  access by block/instruction or unique result/stored value.
- `diagnose_unsupported_prepared_instruction_fragment` then calls
  `local_memory_diagnostic` for `LoadLocalInst`/`StoreLocalInst`.
- `local_memory_diagnostic` currently admits only
  `prepared_frame_slot_absolute_byte_offset`,
  `prepared_byval_stack_slot_pointer_access_offset`,
  `prepared_pointer_value_base_offset`,
  `prepared_pointer_value_stack_home_base_offset`, and an 8-byte
  string-constant pointer-load special case. It never checks
  `PreparedAddressBaseKind::GlobalSymbol` or
  `prepared_global_symbol_memory_has_publication_authority`, so the first
  missing boundary is RV64 local-memory consumer admission for direct global
  symbol prepared accesses.
- `prepared_local_memory_emit.cpp` has direct-global address materialization
  support for pointer values and string/global address materializations, but no
  load/store helper that consumes a `PreparedMemoryAccess` with
  `address.base_kind=GlobalSymbol` as local memory.

## Suggested Next

Step 4 consumer packet: add a narrow RV64 direct-global local-memory admission
for the `pr46309.c` scalar family. Gate it on `access != nullptr`,
`address_space=Default`, `!is_volatile`,
`address.base_kind=PreparedAddressBaseKind::GlobalSymbol`,
`prepared_global_symbol_memory_has_publication_authority(access->address)`,
direct/default addressing policy, supported 1/2/4/8-byte width, alignment not
larger than width, and an encodable direct-global address+offset sequence. Use
the same packet to add fail-closed focused coverage for missing symbol identity,
missing base-plus-offset, non-default address space or unsupported addressing
policy, incomplete extent/range, wrong layout authority, volatile access, and
unsupported width.

## Watchouts

This trace found a consumer gap, not a missing prepared producer for the
`pr46309.c` scalar row. Keep the Step 4 packet limited to direct scalar
global-symbol local memory; do not admit aggregate byte-storage rows, large
offset rows, string constants, aggregate homes, move-bundle rows, runtime
mismatches, or unsupported-width cases. Do not infer authority from final symbol
spelling or assembly: consume only the prepared fields named above.

## Proof

Evidence-only trace; no build or backend tests were required and no root-level
logs were created. Used `c4c-clang-tool-ccdb` for symbol/caller/callee queries
before focused source reads, plus the existing Step 1 artifact
`build/agent_state/631_step1_pr46309.prepared.txt`.
