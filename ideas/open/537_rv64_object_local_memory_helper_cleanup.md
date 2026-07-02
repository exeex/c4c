# RV64 Object Local Memory Helper Cleanup

## Goal

Extract RV64 object-route local frame-slot load/store, local pointer materialization, and pointer-value base-plus-offset helper families after the frame helper boundary is stable.

## Why This Exists

Local memory helpers in `object_emission.cpp` are mixed with global symbol materialization, prepared data objects, BIR dispatch, and frame offset logic. A local-memory-only cleanup should reduce coupling while preserving prepared access facts.

## In Scope

- Owned files:
  - `src/backend/mir/riscv/codegen/object_emission.cpp`
  - `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_local_memory_emit.hpp`
- Extract local frame-slot load/store, local pointer materialization, and pointer-value base-plus-offset helper families.
- Keep APIs structured around existing prepared access facts.
- Depend on, or account for, the frame/stack helper boundary from `ideas/open/535_rv64_object_frame_stack_helper_cleanup.md`.

## Out Of Scope

- Global symbol materialization, prepared data-object section emission, `.rodata`/`.data`/`.bss` assembly, and object symbol definition.
- Local-array semantic changes, pointer provenance changes, BIR inference, or prepared-memory fact repair.
- gcc_torture expectation changes, unsupported marker changes, target-side inference, or RV64 capability repair.

## Acceptance Criteria

- Local memory helper ownership is separated from global/address and object-data concerns.
- Prepared access facts and diagnostics are preserved.
- Validation includes:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|codegen_route_riscv64_prepared_local_array|obj_runtime_rv64_local_temp|obj_runtime_rv64_large_fixed_frame_slot_access|rv64_runtime_riscv64_pointer_to_pointer_local_address)'`

## Reviewer Reject Signals

- The slice rewrites local-array semantics, pointer provenance, or prepared memory facts.
- The diff blends local memory with global symbol materialization or object data sections.
- The implementation adds named-testcase memory shortcuts or new BIR-derived facts.
- Tests, unsupported markers, or expectations are weakened to claim progress.
- A new helper hides the same local/global/object-data coupling.
