Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Load-Local Materialization Authority

# Current Packet

## Just Finished

Step 3 - Repair Load-Local Materialization Authority completed by audit; no
code repair was needed. In `emit_value_publication_to_register`, the load-local
branch is entered only from `prepared_same_block_scalar_producer`, then rejects
unmatched loads with `prepared_memory_access_matches_instruction` before any
materialization. The recovered narrow-store route delegates to
`find_prepared_recovered_narrow_store_source_for_wide_local_load`, whose load
and candidate store checks require prepared memory accesses and prepared frame
slot authority. The direct frame-slot load route goes through
`prepared_local_load_offset`, and the pointer-value fallback goes through
`emit_prepared_pointer_value_load_to_register`; both consume prepared addressing
facts instead of locally reconstructing load authority.

## Suggested Next

Supervisor review/commit for the Step 3 audit slice, then choose the next
active packet from the remaining plan state.

## Watchouts

- `find_prepared_recovered_narrow_store_source_for_wide_local_load` still scans
  same-block stores, but only returns after prepared load/store memory-access
  facts prove the frame slot byte relationship.
- `emit_prepared_pointer_value_load_to_register` lives in `memory.cpp`, outside
  this packet's owned edit set; this packet audited it as a callee and did not
  modify it.
- This packet intentionally did not touch `plan.md`, idea files,
  `globals.cpp`, `fp_value_materialization.cpp`,
  `dispatch_edge_copies.cpp`, `dispatch_producers.cpp`, or
  `test_before.log`.

## Proof

Ran fixed proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$') > test_after.log 2>&1`

Result: passed. `test_after.log` exists and records the build plus 6/6 passing
focused tests.
