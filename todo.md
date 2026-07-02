Status: Active
Source Idea Path: ideas/open/537_rv64_object_local_memory_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract the encoded local-memory helper family

# Current Packet

## Just Finished

Step 2 from `plan.md` extracted the encoded RV64 local-memory helper family into `prepared_local_memory_emit.*` without changing tests, expectations, unsupported markers, or prepared facts.

Moved into `prepared_local_memory_emit.*`:
- Encoded local load/store entry points: `fragment_for_prepared_store_local` and `fragment_for_prepared_load_local`.
- Direct local addressability helpers and diagnostics support: `prepared_frame_slot_absolute_byte_offset`, `prepared_pointer_value_base_offset`, `prepared_byval_stack_slot_pointer_access_offset`, `prepared_sret_stack_slot_pointer_access`, and `PreparedSretStackPointerAccess`.
- Local frame-slot address materialization offset support for storing frame-slot pointer values.
- Local type-size helper `rv64_local_memory_size_for_type`.

Retained wrapper/park decisions for Step 3 review:
- `object_emission.cpp` now includes `prepared_local_memory_emit.hpp` and calls the moved declarations directly from dispatch and diagnostics.
- `prepared_frame_slot_absolute_offset` remains parked in `object_emission.cpp`; it is still not part of the extracted encoded local-memory entry path and should be pruned or reclassified in Step 3 only after actual call-site review.
- `fragment_for_prepared_frame_address_materialization`, global symbol materialization, pcrel/object fixup helpers, data-object emission, relocation/ELF writing, and module assembly remain parked in `object_emission.cpp`.
- Generic encoded helpers stayed parked; `prepared_local_memory_emit.cpp` uses existing prepared-frame APIs plus local private support routines for the moved encoded local-memory implementation.

## Suggested Next

Execute Step 3 from `plan.md`: review remaining object-route local-memory wrappers and prune only wrappers that are genuinely dead and not preserving a compatibility boundary.

## Watchouts

- Step 3 should not infer that helper duplication inside `prepared_local_memory_emit.cpp` authorizes moving global/object fixup helpers; those remain outside the local-memory boundary.
- Do not prune `prepared_frame_slot_absolute_offset` unless direct caller review confirms it is genuinely dead or redundant.
- Keep tests, expectation files, unsupported markers, local-array semantics, pointer provenance, and prepared memory facts unchanged.

## Proof

Supervisor-selected Step 2 proof passed and wrote `test_after.log`:

```bash
bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|codegen_route_riscv64_prepared_local_array|obj_runtime_rv64_local_temp|obj_runtime_rv64_large_fixed_frame_slot_access|rv64_runtime_riscv64_pointer_to_pointer_local_address)'" > test_after.log 2>&1
```

Result: 8/8 matching tests passed.
