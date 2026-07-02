# RV64 Object Global Address Helper Cleanup

## Goal

Extract RV64 object-route global load/store and direct symbol/address materialization helpers that produce per-instruction fragments while keeping final object data assembly late and central.

## Why This Exists

Global address helpers are entangled with local memory, AUIPC/LO12 fixups, object data sections, and public object assembly. A focused global-address cleanup should separate instruction-fragment materialization from final data-object ownership.

## In Scope

- Owned files:
  - `src/backend/mir/riscv/codegen/object_emission.cpp`
  - `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_global_memory_emit.hpp`
- Extract global load/store and direct symbol/address materialization helpers that remain per-instruction fragments.
- Preserve symbol spelling, fixup target contracts, visibility, and relocation target behavior.
- Coordinate with local memory cleanup so local and global address helpers remain distinct.

## Out Of Scope

- `append_rv64_prepared_data_objects`, final `.rodata`/`.data`/`.bss` section selection, global object symbol definition, data relocation assembly, text module assembly, and ELF writing.
- Capability repair, gcc_torture expectation changes, unsupported marker changes, target-side inference, or data-object contract changes.

## Acceptance Criteria

- Global/address helper ownership is separated without moving final data-object or module assembly.
- Symbol spelling, relocation targets, and emitted object behavior remain unchanged.
- Validation includes:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|codegen_route_riscv64_aggregate_global|codegen_route_riscv64_global_store|obj_runtime_rv64_global_store_prepared_value_preserves_source|rv64_runtime_global_load)'`

## Reviewer Reject Signals

- The slice moves `append_rv64_prepared_data_objects`, object section selection, text module assembly, or ELF writing.
- The diff changes symbol visibility, relocation targets, link-name spelling, or object bytes.
- Global helper movement is mixed with RV64 capability repair or unsupported expectation changes.
- The implementation routes through text assembly parsing or hides fixup contracts.
- The same global/data-object coupling remains behind new wrappers.
