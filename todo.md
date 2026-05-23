Status: Active
Source Idea Path: ideas/open/385_aarch64_dispatch_publication_mechanical_split.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Extract Store Source and Writeback Handling

# Current Packet

## Just Finished

Step 5 mechanically extracted the store-source and writeback handling cluster
into `dispatch_store_sources.cpp/hpp`: `NarrowLocalStorePublication`,
`store_local_uses_pointer_value_address`,
`prepared_or_emitted_store_value_register`, `local_slot_reference_name`,
`local_slot_reference_matches`, `prepared_frame_slot_object_name`,
`prepared_load_local_frame_object_name`, `value_name_has_slot_prefix`,
`parse_trailing_dot_offset`, `store_local_targets_logical_slot`,
`find_latest_narrow_store_for_wide_local_load`,
`store_local_value_is_byval_frame_slot_load`,
`store_local_value_is_wide_load_from_narrow_local_store`,
`store_local_value_cast_producer`, `store_local_value_has_select_producer`,
`store_local_value_has_scalar_fp_binary_producer`,
`emit_scalar_conversion_cast_to_register`,
`lower_store_local_value_publication`,
`lower_stack_homed_pointer_store_writeback`,
`prepared_global_symbol_from_value_name`,
`emit_global_symbol_address_to_register`,
`emit_pointer_base_plus_offset_to_register`,
`store_local_frame_slot_offset`,
`lower_pointer_base_plus_offset_store_local_publication`,
`is_store_global_select_snapshot_run_instruction`,
`lower_pending_store_global_stack_value_publications`, and
`lower_store_global_value_publication`. No Step 5-listed helper was left in
`dispatch_publication.cpp`; `dispatch_publication.hpp` re-exports the new
store-source header for existing users.

## Suggested Next

Delegate Step 6, the final size and dependency pass.

## Watchouts

- Keep the split mechanical and behavior-preserving.
- Do not fold code into long-term feature homes such as `calls.cpp`,
  `memory.cpp`, or `comparison.cpp`.
- `dispatch_publication.hpp` now includes `dispatch_edge_copies.hpp` and
  `dispatch_store_sources.hpp` so existing publication users continue to see
  extracted declarations.
- `dispatch_edge_copies.cpp` keeps a local forward declaration for
  `find_bir_block`, which is still defined in `dispatch_producers.cpp`; this
  avoided widening another header during the mechanical move.
- Unlisted state/address helpers stayed central in `dispatch_publication.cpp`
  because they are shared across value-materialization, edge-copy, and
  store-source paths: examples include `prepared_value_home_for_value`,
  `prepared_value_home_reads_register_index`,
  `value_publication_may_read_register_index`, `instruction_result_value`,
  `prepared_local_load_offset`, and `prepared_va_list_field_address`.

## Proof

Passed. Ran exactly:
`{ cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10 && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure; } > test_after.log 2>&1`

Proof log: `test_after.log`.
