Status: Active
Source Idea Path: ideas/open/537_rv64_object_local_memory_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prune only genuinely dead local-memory wrappers

# Current Packet

## Just Finished

Step 3 from `plan.md` reviewed the remaining object-route local-memory wrapper state and pruned only the dead wrapper confirmed by direct caller review.

Removed:
- `prepared_frame_slot_absolute_offset` from `object_emission.cpp`; `c4c-clang-tool-ccdb function-callers` reported no direct callers in the translation unit, and `rg` found only the definition.

Retained with call-site reasons:
- `fragment_for_prepared_store_local` and `fragment_for_prepared_load_local` stay exported from `prepared_local_memory_emit.*`; direct callers are the object-route `fragment_for_prepared_instruction` store/load dispatch cases.
- `prepared_frame_slot_absolute_byte_offset`, `prepared_byval_stack_slot_pointer_access_offset`, and `prepared_sret_stack_slot_pointer_access` stay exported for `diagnose_unsupported_prepared_instruction_fragment`, with additional direct use inside local store/load emission.
- `prepared_pointer_value_base_offset`, `prepared_frame_slot_address_materialization_offset`, and `rv64_local_memory_size_for_type` stay live through direct local store/load helper callers; `rv64_local_memory_size_for_type` is also used by `diagnose_unsupported_prepared_param_homes`.
- `fragment_for_prepared_frame_address_materialization`, global symbol materialization, pcrel/object fixup helpers, data-object emission, relocation/ELF writing, and module assembly remain parked in `object_emission.cpp` because they are outside this local-memory wrapper cleanup packet.

## Suggested Next

Supervisor should review/commit the Step 3 cleanup slice, then decide whether the active plan is exhausted enough for lifecycle review.

## Watchouts

- Tests, expectation files, unsupported markers, local-array semantics, pointer provenance, and prepared memory facts remain unchanged.
- No additional parked local-memory wrapper was found with zero direct callers in the owned files.
- This packet did not move global/object fixup helpers; that remains outside the Step 3 boundary.

## Proof

Supervisor-selected Step 3 proof passed and wrote `test_after.log`:

```bash
bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|codegen_route_riscv64_prepared_local_array|obj_runtime_rv64_local_temp|obj_runtime_rv64_large_fixed_frame_slot_access|rv64_runtime_riscv64_pointer_to_pointer_local_address)'" > test_after.log 2>&1
```

Result: 8/8 matching tests passed.
