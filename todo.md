Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Replace Same-Block Scalar Producer Recovery

# Current Packet

## Just Finished

Step 2 - Replace Same-Block Scalar Producer Recovery completed. Added the
shared prepared helper
`prepare::find_prepared_same_block_scalar_producer`, keyed by prepared named
value plus `before_instruction_index`, returning
`PreparedSameBlockScalarProducer` only when the prepared source producer matches
the requested block label, instruction index, instruction variant pointer, and
result value identity. `dispatch_value_materialization.cpp` now consumes that
helper through a narrow context adapter instead of owning local
`prepared_source_producer_for_value`,
`prepared_source_producer_matches_instruction`, and
`prepared_same_block_scalar_producer_context` logic.

## Suggested Next

Implement the next Step 2 packet by moving the remaining dispatch-specific
load-global address-policy or select-chain producer authority into prepared
lookups only if it can stay narrow and reusable. Keep the packet bounded to one
producer family and continue using the prepared same-block scalar producer
helper as the entry authority.

## Watchouts

- `prepare::find_prepared_same_block_scalar_producer` validates producer
  pointer identity against the BIR instruction variant; callers should not
  bypass it with raw value-name scans.
- The helper intentionally rejects immediate and unknown source-producer kinds.
- `find_prepared_same_block_load_local_stored_value_source` now routes through
  the same helper, so future changes to this authority affect recovered
  narrow-store source lookup as well as dispatch materialization.
- Do not widen into `globals.cpp`, `fp_value_materialization.cpp`, or edge-copy
  publication files without a new packet.

## Proof

Ran fixed proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$') > test_after.log 2>&1`

Result: passed. `test_after.log` exists and records the build plus 4/4 passing
focused tests.
