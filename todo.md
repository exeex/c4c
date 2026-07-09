Status: Active
Source Idea Path: ideas/open/630_string_constant_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace String/Data Authority Carriers

# Current Packet

## Just Finished

Completed Step 2: traced prepared string/data authority carriers for the
in-scope Step 1 string-constant local-memory accesses.

Producer and carrier trace:

- BIR string data is produced in `collect_lowered_string_constants(...)` and
  `materialize_string_constants(...)` in `src/backend/bir/lir_to_bir.cpp`.
  `bir::StringConstant` carries `name`, `name_id`, `bytes`, and `align_bytes`.
  This gives string identity, text symbol, byte payload, and alignment, but it
  is module data, not local-memory access authority.
- String pointer local-memory addresses are produced by
  `append_string_pointer_value_materialization(...)` in
  `src/backend/bir/lir_to_bir/memory/local_slots.cpp`. It emits a
  `bir::LoadLocalInst` whose `address` is a `bir::MemoryAddress` with
  `base_kind=StringConstant`, `base_name=<string symbol>`,
  `size_bytes=8`, `align_bytes=8`, and pointer result identity. It does not
  publish string extent, byte payload, or non-unknown layout authority into
  `MemoryAccessProvenance`.
- Prepared local-memory access production is
  `build_direct_symbol_backed_access(...)` in
  `src/backend/prealloc/stack_layout/coordinator.cpp`, through
  `build_direct_symbol_backed_address(...)`. For string constants it sets
  `PreparedAddress.base_kind=StringConstant`,
  `PreparedAddress.symbol_name=names.link_names.intern(symbol_name)`,
  `byte_offset=address->byte_offset + fallback_byte_offset`,
  `size_bytes=<access width>`, `align_bytes`, `can_use_base_plus_offset=true`,
  `PreparedMemoryAccess.address_space=prepared_memory_address_space(...)`, and
  `PreparedMemoryAccess.is_volatile`.
- Provenance identity is copied or synthesized by
  `prepared_memory_provenance(...)`. For string bases it can set
  `provenance.base_identity.kind=StringConstant`,
  `base_identity.spelling=address->base_name`, requested range from
  `make_memory_byte_range(byte_offset, size_bytes)`, and then
  `prove_memory_access_requested_range(...)`.
- The first missing authority boundary is still in prepared access production:
  `build_direct_symbol_backed_address(...)` handles string constants on a
  symbol-backed path, but it does not join the access to `bir::StringConstant`
  as the source of string bytes/extent, and it does not publish a string-specific
  layout/use authority. The existing global authority helpers
  `publish_scalar_global_layout_authority(...)`,
  `publish_integer_array_global_layout_authority(...)`, and
  `publish_byte_storage_global_layout_authority(...)` are global-object
  policies and are not the right authority for string constants. The resulting
  carrier has string identity, label spelling, offset, width, alignment,
  address space, and whatever range verdict provenance already supplied, but
  lacks explicit string bytes/extent authority tied to the carrier and selected
  string-local-memory layout/use authority.

Existing selected/local-memory authority carriers:

- `PreparedAddressMaterialization` can carry string address materialization
  authority: `kind=StringConstant`, `text_name`, `byte_offset`,
  `address_space`, and `has_tls_address_space`, produced by
  `append_string_constant_address_materialization(...)`.
- `PreparedMemoryAccess` currently carries selected local-memory facts for the
  failing accesses: function/block/instruction, result or stored value name,
  address space, volatility, and nested `PreparedAddress` with base kind,
  string symbol, byte offset, access size/align, base-plus-offset capability,
  and provenance.
- `PreparedEdgePublication` has source-memory mirrors for local-load
  publications through `copy_source_memory_access_fact(...)` in
  `src/backend/prealloc/prepared_lookups.cpp`: base kind, symbol name, byte
  offset, size, align, address space, volatility, base-plus-offset capability,
  `source_memory_layout_authority`, and `source_memory_range_verdict`. For
  strings this mirrors `layout_authority=unknown`; it is not the first missing
  boundary.

Current RV64 consumer checks and rejection points:

- `emit_riscv_string_constant_address_materialization(...)` in
  `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp` can materialize
  a `PreparedAddressMaterializationKind::StringConstant` into `lla` plus
  optional `addi` only when the destination register is nonempty, address space
  is default, TLS flags are clear, `text_name` exists, the byte offset is
  signed-12-bit encodable, and the prepared text label lookup is nonempty.
- `emit_riscv_string_constant_access_address_materialization(...)` can
  materialize a `PreparedMemoryAccess` with
  `address.base_kind=StringConstant`, default address space, nonvolatile access,
  present `address.symbol_name`, `can_use_base_plus_offset=true`, signed-12-bit
  offset, and nonempty prepared link-name spelling. It checks addressing shape
  only; it does not validate extent, bytes, range verdict, or layout/use
  authority.
- The object-route local-memory diagnostic in
  `src/backend/mir/riscv/codegen/object_emission.cpp` currently rejects the
  Step 1 rows before any string access admission. For load/store locals it calls
  `prepared_memory_access_for_local_instruction(...)`, then
  `local_memory_diagnostic(...)`, which accepts only
  `prepared_frame_slot_absolute_byte_offset(...)`,
  `prepared_byval_stack_slot_pointer_access_offset(...)`,
  `prepared_pointer_value_base_offset(...)`, and
  `prepared_pointer_value_stack_home_base_offset(...)` (plus f64's smaller set).
  A `PreparedAddressBaseKind::StringConstant` access matches none of these, so
  the route returns `unsupported_local_memory_access: RV64 object route requires
  prepared frame-slot or pointer-value base-plus-offset local memory addressing`.

Fail-closed conditions for Step 3/4:

- Reject if the prepared access is absent, ambiguous for the instruction/result,
  volatile, non-default address space, TLS-marked, or not
  `PreparedAddressBaseKind::StringConstant`.
- Reject if string identity is missing or ambiguous: no `symbol_name`/text
  identity, empty prepared spelling, no matching `bir::StringConstant.name_id`
  or text spelling, or multiple incompatible labels for one access.
- Reject if extent or byte authority is missing: no matching string constant
  bytes, unknown/zero emitted extent, alignment zero, range unavailable,
  overflowed, negative, mismatched with `byte_offset`/`size_bytes`, or not
  proven in bounds when the selected contract requires in-bounds.
- Reject if selected local-memory use authority remains only implicit:
  `layout_authority=unknown`, unsupported authority kind for strings,
  `can_use_base_plus_offset=false`, access width unsupported by RV64, or offset
  not encodable for the intended materialization/load path.
- Keep out-of-scope rows fail-closed: direct global-symbol accesses,
  aggregate homes, generic frame slots, large offsets, pointer-value byte
  accesses after the selected string pointer is stored, ABI/runtime/library
  gaps, and expectation/unsupported/allowlist changes.

## Suggested Next

Execute Step 3 as a prepared-layer coverage packet, not an RV64 consumer patch
yet: add the smallest producer/prepared evidence that proves
`build_direct_symbol_backed_address(...)` publishes complete string-constant
facts for selected local-memory accesses by joining the
`PreparedAddressBaseKind::StringConstant` access to `bir::StringConstant`
identity/bytes/extent and by recording an explicit string-local-memory layout or
use authority in the prepared carrier. Include positive coverage for one
in-bounds 8-byte access and fail-closed coverage for missing string identity,
missing bytes/extent, out-of-bounds range, and non-default address space.

## Watchouts

Do not route string constants through frame-slot, generic direct-global, or
testcase-shaped admission just because their current carrier uses
`symbol_name`. `policy=unspecified` is expected for string constants and should
not be patched by inventing a global materialization policy. The missing
boundary is explicit string data/extent/local-memory authority for the prepared
access. The existing RV64 string address materialization helpers are useful
shape emitters, but consumer admission should wait until the prepared carrier
can fail closed on identity, bytes, extent, range, address space, width, offset,
and selected string-local-memory authority.

## Proof

Evidence-only trace. No build or test was required by the delegated proof, and
no root-level logs or build artifacts were created or modified.

Read-only diagnostics used:

- `c4c-clang-tool-ccdb function-signatures` for
  `src/backend/prealloc/stack_layout/coordinator.cpp`,
  `src/backend/bir/lir_to_bir/memory/provenance.cpp`, and
  `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`.
- `c4c-clang-tool-ccdb list-symbols` for
  `src/backend/mir/riscv/codegen/object_emission.cpp`.
- Targeted source reads of the producer/carrier/consumer functions named above.
