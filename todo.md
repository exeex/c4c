Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair Local-Slot Address Materialization Authority

# Current Packet

## Just Finished

Step 5 - Repair Local-Slot Address Materialization Authority completed by audit;
no code repair was needed. In `emit_value_publication_to_register`, the add/sub
pointer path first delegates to `emit_local_slot_address_publication_to_register`
before falling back to generic recursive binary materialization. The delegated
helper accepts only named-pointer plus immediate add/sub forms, resolves the
named base through `prepared_named_value_id`, and requires
`prepared_frame_address_offset_for_value` to return a prepared offset before it
emits the frame-address `add`. That prepared offset is backed by
`find_indexed_prepared_frame_address_offset_for_value`, which selects a
same-block prepared `FrameSlot` address materialization for the value, respects
the before-or-at instruction index, rejects ambiguous same-instruction frame
slot facts, and returns the stack offset from the prepared frame slot fact.
`local_slot_address_frame_offset` remains a null stub, so the audited
publication route is not using slot-name parsing or local offset reconstruction
as authority.

## Suggested Next

Supervisor review/commit for the Step 5 audit slice, then run the separately
planned broader backend validation that includes the intentionally excluded
dynamic-stack fixed-slot case.

## Watchouts

- `emit_local_slot_address_publication_to_register` and the backing helper live
  in `dispatch_publication.cpp`, outside this packet's owned edit set; this
  packet audited them as context and did not modify them.
- The known failing
  `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor` was
  intentionally excluded from this focused packet proof by supervisor
  instruction.
- This packet intentionally did not touch `plan.md`, idea files,
  `globals.cpp`, `fp_value_materialization.cpp`,
  `dispatch_edge_copies.cpp`, `dispatch_producers.cpp`, or
  `test_before.log`.

## Proof

Ran fixed proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_cli_dump_prepared_bir_local_arg_call_contract|backend_cli_dump_prepared_bir_vla_goto_stackrestore_cfg|backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value)$') > test_after.log 2>&1`

Result: passed. `test_after.log` exists and records the build plus 4/4 passing
focused tests.
