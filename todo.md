Status: Active
Source Idea Path: ideas/open/538_rv64_object_global_address_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map the global-address boundary

# Current Packet

## Just Finished

Step 1: Map the global-address boundary is complete. The RV64 object-route
global-address helper boundary is classified for the Step 2 extraction.

Move set for `prepared_global_memory_emit.*`:
- `fragment_for_prepared_symbol_address_materialization`
- `prepared_address_materialization_symbol`
- `prepared_direct_global_materialization_symbol`
- `fragment_for_prepared_load_global`
- `fragment_for_prepared_store_global`
- `prepared_global_access_is_supported`
- `prepared_global_for_access_symbol`
- `append_rv64_load_global_base_to_register`
- `append_rv64_store_register_to_global_base`
- `rv64_global_scalar_memory_size_for_type`

Shared support that may move only if Step 2 keeps call/data ownership intact:
- `make_rv64_pcrel_address_fragment` overloads emit AUIPC/LO12 encoded
  fragments for calls, symbol address materialization, and global load/store;
  keep the public declaration in `object_emission.hpp` or move the definition
  behind a neutral encoded-fragment helper boundary without moving call logic.

Already owned by prepared global memory:
- `emit_riscv_direct_global_address_materialization`
- `emit_riscv_direct_function_address_materialization`
- `emit_riscv_simple_load_global`
- `emit_riscv_simple_store_global`
- `append_prepared_global_storage_asm`

Parked in `object_emission.cpp`:
- `prepared_call_argument_object_symbol` and
  `rv64_normalize_prepared_object_symbol`: call-argument symbol-address
  emission, not global memory ownership.
- `fragment_for_prepared_frame_address_materialization`: local/frame address
  ownership already guarded by the local-memory split.
- `rv64_elf_relocation_type`, `rv64_relocatable_elf_config`,
  `build_rv64_text_object_module`, `write_rv64_relocatable_elf_object`, and
  `write_rv64_prepared_relocatable_elf_object*`: relocation assembly, text
  module assembly, and ELF writing.
- `rv64_prepared_object_global_label`, `rv64_prepared_object_text_label`,
  `rv64_selected_object_data_contract_facts`,
  `rv64_prepared_object_data_contract_diagnostic`,
  `rv64_prepared_object_data_label`, `rv64_prepared_link_name_label`,
  `rv64_prepared_link_symbol_kind`,
  `rv64_prepared_object_data_has_emission_identity`,
  `rv64_prepared_object_data_is_selected_fallback_candidate`,
  `rv64_is_selected_zero_fill_object_data`,
  `rv64_selected_symbol_pointer_initializer_label`,
  `rv64_prepared_object_data_admission_diagnostic`,
  `rv64_find_or_declare_relocation_symbol`,
  `rv64_prepared_object_data_symbol_binding`, and
  `append_rv64_prepared_data_objects`: final object data, symbol definition,
  data relocation, and section-selection ownership.

## Suggested Next

Execute Step 2 from `plan.md`: extract the move-set helpers into
`prepared_global_memory_emit.cpp`/`.hpp`, leaving parked object-data, call,
module, relocation, ELF, and local-memory helpers in `object_emission.cpp`.

## Watchouts

- Do not move `append_rv64_prepared_data_objects`, final data section selection, global object symbol definition, data relocation assembly, text module assembly, ELF writing, or final object assembly.
- Do not change symbol spelling, visibility, relocation targets, object bytes, tests, unsupported markers, gcc_torture expectations, or data-object contracts.
- Keep local memory and global address helper ownership distinct.
- Keep routine execution notes in this file; do not edit the source idea unless durable source intent changes.
- Step 2 include/declaration prerequisites: add `object_emission.hpp` visibility
  to `prepared_global_memory_emit.hpp`/`.cpp` for `RiscvEncodedFragment` and
  `RiscvObjectFixupTargetKind`; expose only the extracted fragment builders
  needed by `object_emission.cpp`; include `prepared_frame_emit.hpp` for
  `rv64_prepared_stack_slot_home_absolute_offset`-style stack destination
  support if the global load helper follows the local-memory split pattern.
- Step 2 helper prerequisites: either duplicate the local-memory-style private
  wrappers in `prepared_global_memory_emit.cpp` for register lookup, scalar
  size, stack-home offset, move-value-to-register, and load/store funct3
  encoding, or first expose neutral prepared/object fragment utilities without
  pulling scalar, call, frame, or local-memory ownership into the global helper.
- Keep `make_rv64_pcrel_address_fragment` call-compatible. It has callers from
  `fragment_for_prepared_call`, `fragment_for_prepared_symbol_address_materialization`,
  `fragment_for_prepared_load_global`, and `fragment_for_prepared_store_global`.

## Proof

No validation run; Step 1 is mapping-only and the delegated packet did not
require a build.

Exact Step 2 proof command from `plan.md`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|codegen_route_riscv64_aggregate_global|codegen_route_riscv64_global_store|obj_runtime_rv64_global_store_prepared_value_preserves_source|rv64_runtime_global_load)'`

No `test_after.log` was written for this mapping-only packet.
