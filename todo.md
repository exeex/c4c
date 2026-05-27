Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Global And Select-Chain Materialization Authority

# Current Packet

## Just Finished

Step 4 - Repair Global And Select-Chain Materialization Authority completed.
Exposed `find_prepared_same_block_select_producer` from
`dispatch_producers.cpp` / `dispatch_producers.hpp`, tightened the prepared
select producer match to require pointer identity with the indexed BIR select
instruction, and changed `emit_select_chain_value_to_register` so the
direct-global select-chain dependency route recursively consumes the
prepared-only select producer lookup instead of falling through to the raw
same-block select walker.

## Suggested Next

Supervisor review/commit for the Step 4 slice, then choose the next active
packet from the remaining plan state.

## Watchouts

- `prepare::find_prepared_same_block_scalar_producer` validates producer
  pointer identity against the BIR instruction variant; callers should not
  bypass it with raw value-name scans.
- The legacy `find_same_block_select_producer` API still preserves its raw MIR
  fallback for existing non-direct-global consumers. The repaired path is the
  direct-global dependency route through `emit_select_chain_value_to_register`.
- This packet intentionally did not add dispatch-local fallback logic in
  `dispatch_value_materialization.cpp` and did not touch `globals.cpp` or
  `fp_value_materialization.cpp`.

## Proof

Ran fixed proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$') > test_after.log 2>&1`

Result: passed. `test_after.log` exists and records the build plus 5/5 passing
focused tests.
