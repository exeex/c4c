Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Global And Select-Chain Materialization Authority

# Current Packet

## Just Finished

Step 2 - Replace Same-Block Scalar Producer Recovery continued. Assessed the
remaining select-chain branch reached after
`prepare::find_prepared_same_block_scalar_producer`: the dispatch root producer
is already prepared-authorized, but the recursive select-chain materializer
delegates to `emit_select_chain_value_to_register`, whose producer lookup lives
outside this packet and can still fall back through
`find_same_block_select_producer`. No narrow owned-file implementation is
justified because changing that authority boundary requires the shared
edge-copy/select-chain emitter or producer lookup ownership, not another
dispatch-local source-of-truth check.

Plan-owner decision: Step 2 is precise enough to close for the dispatch-root
prepared producer lookup. The recursive select-chain fallback remains in idea
49 but is Step 4 scope because it is owned by the shared select-chain
producer/emitter boundary in `dispatch_edge_copies.cpp` /
`dispatch_producers.cpp`, not by another local walker in
`dispatch_value_materialization.cpp`.

## Suggested Next

Execute Step 4 with a focused slice that explicitly owns the shared
select-chain producer lookup in `dispatch_edge_copies.cpp` /
`dispatch_producers.cpp`, so recursive select-chain materialization can require
prepared producer authority without dispatch-local same-block fallback.

## Watchouts

- `prepare::find_prepared_same_block_scalar_producer` validates producer
  pointer identity against the BIR instruction variant; callers should not
  bypass it with raw value-name scans.
- The root dispatch select branch is not the remaining authority gap; the gap
  is the recursive select-chain producer selection inside the shared emitter.
- Do not add a second dispatch-local select-chain walker in
  `dispatch_value_materialization.cpp`; that would extend the duplicate
  authority pattern rather than repair it.
- This packet intentionally did not touch `dispatch_edge_copies.cpp`,
  `dispatch_producers.cpp`, `globals.cpp`, `fp_value_materialization.cpp`, or
  edge-copy publication files.

## Proof

Ran fixed proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$') > test_after.log 2>&1`

Result: passed. `test_after.log` exists and records the build plus 4/4 passing
focused tests.
