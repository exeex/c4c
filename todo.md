Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Dispatch Materialization Fallbacks

# Current Packet

## Just Finished

Step 1 - Audit Dispatch Materialization Fallbacks completed. In
`dispatch_value_materialization.cpp`, the first bounded repair target is the
non-edge same-block scalar producer materialization path reached through
`prepared_same_block_scalar_producer_context` and then locally re-lowered in
`emit_value_publication_to_register` for cast, binary, select, load-local, and
load-global producers.

Classification notes:
- Current-block entry publication and value-home fallbacks are already prepared
  consumers: `current_block_entry_publication_register`,
  `prepared_value_home_for_value`, `value_has_current_block_entry_publication`,
  and prepared value-home materialization.
- Load-local materialization already has prepared anchors:
  `PreparedMemoryAccess`, `PreparedAddressingFunction`,
  `find_prepared_recovered_narrow_store_source_for_wide_local_load`,
  `prepared_local_load_offset`, and prepared pointer-value load.
- Load-global materialization already consumes `PreparedMemoryAccess` plus
  `prepared_global_symbol_address_policy`; the remaining concern is whether
  that policy should be exposed as a shared dispatch query rather than local
  instruction spelling.
- Select-chain dispatch delegates to `emit_select_chain_value_to_register`,
  but it is still reached from local same-block producer classification.
- Local-slot address materialization already routes through
  `emit_local_slot_address_publication_to_register`; it should stay downstream
  of the same shared local-slot/frame-offset authority.

## Suggested Next

Implement Step 2 as a focused packet in
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`: replace
the non-edge same-block scalar producer fallback with a shared prepared query
keyed by named value plus `before_instruction_index` that returns the prepared
producer context only when `PreparedEdgePublicationSourceProducer` matches the
current block, instruction index, and result value. Prefer moving the existing
`prepared_source_producer_for_value` /
`prepared_source_producer_matches_instruction` /
`prepared_same_block_scalar_producer_context` logic into a shared prepared
lookup helper, or consume an equivalent existing helper if one is promoted
first. The implementation packet should not add deeper recursion or
case-shaped producer matching; it should make dispatch consume the prepared
producer authority before any local semantic lowering.

## Watchouts

- Do not deepen same-block producer recursion or add named-case shortcuts.
- Do not replace prepared block-entry/value-home checks with raw move-bundle or
  value-name scans.
- Do not hard-code global-name spelling, GOT policy, or direct-global
  select-chain shortcuts.
- Keep `globals.cpp` and `fp_value_materialization.cpp` out of this plan; they
  belong to source idea `54`.
- Keep routine executor progress in this file rather than rewriting `plan.md`.
- `dispatch_producers.cpp` and `publication_plans.cpp` contain near-duplicate
  prepared source-producer validation logic; the repair should converge on one
  narrow shared query instead of cloning another local helper.
- The proof subset for the implementation packet should include the delegated
  dispatch tests from this audit packet, especially the function-pointer table
  indirect-call and pointer-value writeback cases.

## Proof

Ran fixed proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$') > test_after.log 2>&1`

Result: passed. `test_after.log` exists and records a no-op build plus 4/4
passing focused tests.
