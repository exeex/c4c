# Current Packet

Status: Active
Source Idea Path: ideas/open/81_aarch64_dispatch_edge_copy_owner_contraction.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Relocate narrow target-local emission helpers

## Just Finished

Completed `plan.md` Step 3 by moving only `prepared_memory_access` and
`prepared_memory_access_matches_instruction` from `dispatch_edge_copies.*` to
`memory.*`.

Concrete changes:

- Removed the two declarations from `dispatch_edge_copies.hpp`.
- Added the same declarations to `memory.hpp`.
- Removed the two definitions from `dispatch_edge_copies.cpp`.
- Added the same definitions to `memory.cpp`, preserving signatures and logic.
- Left `prepared_publication_source_memory_matches_access` and
  `prepared_publication_source_memory_access` in `dispatch_edge_copies.cpp`.

## Suggested Next

Supervisor should choose the next Step 3 contraction packet, with ownership
kept to one helper family and call-site include fixes included in the packet
ownership list when required.

## Watchouts

- `dispatch.cpp` and `dispatch_value_materialization.cpp` already include
  `memory.hpp`, so no caller file needed an include change in this packet.
- `dispatch_edge_copies.cpp` still includes `memory.hpp` for its remaining
  edge-publication memory-source helpers and edge load-local emission.
- `dispatch_value_materialization.cpp` was inspected as a caller but was not
  changed because it was outside the delegated owned-file list.

## Proof

Passed:

`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_prepared_memory_operand_records_test backend_aarch64_memory_operand_contract_test backend_aarch64_instruction_dispatch_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(prepared_memory_operand_records|memory_operand_contract|instruction_dispatch)|backend_codegen_route_aarch64_(got_load_global_prepared_memory|pointer_value_named_scalar_writeback_uses_computed_store_value)' --output-on-failure) 2>&1 | tee test_after.log"`

Proof log: `test_after.log`.
