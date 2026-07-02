Status: Active
Source Idea Path: ideas/open/538_rv64_object_global_address_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Close-readiness review

# Current Packet

## Just Finished

Step 3: Prune only genuinely dead global-address wrappers is complete.

Removed after direct-caller confirmation:
- `rv64_global_load_funct3_for_size` in `object_emission.cpp`; AST caller
  query and `rg` found no object-side callers after the Step 2 extraction. The
  live global-load funct3 helper remains private to
  `prepared_global_memory_emit.cpp`.
- `rv64_global_store_funct3_for_size` in `object_emission.cpp`; AST caller
  query and `rg` found no object-side callers after the Step 2 extraction. The
  live global-store funct3 helper remains private to
  `prepared_global_memory_emit.cpp`.
- `rv64_prepared_object_global_label`; AST caller query and `rg` found no
  direct callers.

Retained parked wrappers:
- `make_rv64_pcrel_address_fragment` stays declared in `object_emission.hpp`
  and defined in `object_emission.cpp`. Direct callers still include
  `fragment_for_prepared_call` in `object_emission.cpp` and the extracted
  `fragment_for_prepared_symbol_address_materialization`,
  `fragment_for_prepared_load_global`, and
  `fragment_for_prepared_store_global` in `prepared_global_memory_emit.cpp`,
  so it remains a shared call/global AUIPC/LO12 compatibility boundary.
- Object data, final section selection, symbol definition, relocation assembly,
  text module assembly, ELF writing, call symbol-address handling, local memory
  helpers, and scalar fragment helpers remain parked outside this cleanup.

## Suggested Next

Execute Step 4 from `plan.md`: perform close-readiness review against the source
idea and decide whether this runbook is ready for plan-owner closure evaluation.

## Watchouts

- `make_rv64_pcrel_address_fragment` is still shared by call emission and the
  extracted global fragment builders; do not delete or move it as a dead wrapper.
- The retained public prepared-global declarations are live dispatch API from
  `object_emission.cpp`, not dead wrappers.
- No tests, expectations, unsupported markers, symbol spelling, visibility,
  fixup target contracts, or data-object contracts were changed.

## Proof

Ran the supervisor-selected Step 3 proof command:
`bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|codegen_route_riscv64_aggregate_global|codegen_route_riscv64_global_store|obj_runtime_rv64_global_store_prepared_value_preserves_source|rv64_runtime_global_load)'" > test_after.log 2>&1`

Result: passed. `ctest` matched 5 backend/RV64 global/object tests, all passed.

Proof log: `test_after.log`
