Status: Active
Source Idea Path: ideas/open/538_rv64_object_global_address_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract the global-address fragment helper family

# Current Packet

## Just Finished

Step 2: Extract the global-address fragment helper family is complete. The
per-instruction global/address fragment helpers now live in
`prepared_global_memory_emit.cpp` with the minimal public declarations in
`prepared_global_memory_emit.hpp`.

Moved out of `object_emission.cpp`:
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

Retained compatibility boundary:
- `make_rv64_pcrel_address_fragment` stays declared in `object_emission.hpp`
  and defined in `object_emission.cpp` because call fragments still use the
  same AUIPC/LO12 encoded-fragment helper.
- Object data, final section selection, symbol definition, relocation assembly,
  text module assembly, ELF writing, call symbol-address handling, and local
  memory helpers remain parked in `object_emission.cpp`.

## Suggested Next

Execute Step 3 from `plan.md`: inspect the remaining object-route
global/address wrappers and delete only genuinely dead wrappers. Start with the
post-extraction `rg`/clang caller set; do not broaden into object data,
module/ELF assembly, calls, local memory, scalar fragments, tests, expectations,
or unsupported markers.

## Watchouts

- The new global helper uses private local-memory-style wrappers for register
  lookup, stack-home offsets, move-value-to-register, and global load/store
  funct3 encoding; these are intentionally not public API.
- `make_rv64_pcrel_address_fragment` is deliberately retained for Step 3
  review because it is still shared by call emission and the extracted global
  fragment builders.
- No tests, expectations, unsupported markers, symbol spelling, visibility,
  fixup target contracts, or data-object contracts were changed.

## Proof

Ran the supervisor-selected Step 2 proof command:
`bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|codegen_route_riscv64_aggregate_global|codegen_route_riscv64_global_store|obj_runtime_rv64_global_store_prepared_value_preserves_source|rv64_runtime_global_load)'" > test_after.log 2>&1`

Result: passed. `ctest` matched 5 backend/RV64 global/object tests, all passed.

Proof log: `test_after.log`
