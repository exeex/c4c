Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Replace Same-Block Scalar Producer Recovery

# Current Packet

## Just Finished

Step 2 - Replace Same-Block Scalar Producer Recovery continued. Moved the
same-block load-global address-policy authority into prepared lookups with
`prepare::find_prepared_same_block_global_load_access`, which consumes the
shared `PreparedSameBlockScalarProducer`, verifies the prepared memory access by
result value name and producer position, and only exposes global-symbol
base-plus-offset accesses to dispatch. `dispatch_value_materialization.cpp` now
emits the prepared global load from that prepared result instead of owning local
memory-access matching and global-symbol eligibility checks.

## Suggested Next

Implement the next Step 2 packet by assessing the remaining select-chain
producer authority. Keep it bounded to prepared same-block producer entry, and
stop for plan-owner/reviewer input if moving select-chain recursion would
require touching edge-copy ownership or changing source intent.

## Watchouts

- `prepare::find_prepared_same_block_scalar_producer` validates producer
  pointer identity against the BIR instruction variant; callers should not
  bypass it with raw value-name scans.
- `find_prepared_same_block_global_load_access` intentionally does not emit
  code or choose target relocation syntax; it only selects a prepared global
  load access whose address is eligible for base-plus-offset materialization.
- The load-global packet did not touch `globals.cpp`,
  `fp_value_materialization.cpp`, or edge-copy publication files.
- Do not widen into `globals.cpp`, `fp_value_materialization.cpp`, or edge-copy
  publication files without a new packet.

## Proof

Ran fixed proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$') > test_after.log 2>&1`

Result: passed. `test_after.log` exists and records the build plus 4/4 passing
focused tests.
