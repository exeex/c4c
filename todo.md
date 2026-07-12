# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Remove the address-materialization reconstruction

## Just Finished

- Plan Step 3.1 removed the AArch64 block address-materialization index,
  builder, fallback overload, and dispatch lazy cache; consumers now query the
  traversal-attached common lookup with owner validation and fail closed when
  it is unavailable.

## Suggested Next

- Execute Plan Step 3.2 against the remaining named-handoff reconstruction
  identified by the active runbook.

## Watchouts

- Preserve the direct attached-lookup route established in Step 3.1; do not
  recreate an AArch64 block/instruction address-materialization cache.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_codegen_route_aarch64_(byval_global_payload_address_call_boundary|got_load_global_prepared_memory|global_function_pointer_table_selected_indirect_call|local_aggregate_address_pointer_copy_publishes_frame_address))$'`
  passed 4/4; combined output is in `test_after.log`.
