Status: Active
Source Idea Path: ideas/open/385_aarch64_dispatch_publication_mechanical_split.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract Value Materialization

# Current Packet

## Just Finished

Step 3 mechanically extracted the prepared value/home materialization cluster
into `dispatch_value_materialization.cpp/hpp`: `emit_fp_immediate_to_register`,
`emit_fp_value_to_register`, `emit_prepared_global_symbol_load_to_register`,
`emit_prepared_va_list_field_load_to_register`,
`emit_prepared_pointer_value_load_to_register`,
`emit_prepared_value_home_to_register`, `emit_value_publication_to_register`,
`lower_local_slot_address_publication`,
`lower_stack_homed_pointer_value_load_publication`,
`lower_scalar_cast_publication_to_prepared_register`,
`lower_scalar_fp_binary_publication_to_prepared_register`, and
`lower_scalar_cast_publication_to_prepared_stack`. No Step 3-listed helper was
left behind.

## Suggested Next

Delegate the next mechanical publication split cluster after a fresh AST
caller/callee check, keeping edge-copy and store-source helpers out of the
value-materialization slice.

## Watchouts

- Keep the split mechanical and behavior-preserving.
- Do not fold code into long-term feature homes such as `calls.cpp`,
  `memory.cpp`, or `comparison.cpp`.
- `dispatch_publication.hpp` includes `dispatch_value_materialization.hpp` so
  existing publication users continue to see materialization declarations.
- Unlisted state/address helpers stayed central in `dispatch_publication.cpp`
  because they are shared across materialization, edge-copy, and store-source
  paths: examples include `prepared_value_home_for_value`,
  `current_block_entry_publication_register`, `prepared_local_load_offset`,
  `prepared_va_list_field_address`, `value_publication_may_read_register_index`,
  and `instruction_result_value`.

## Proof

Passed. Ran exactly:
`{ cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10 && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure; } > test_after.log 2>&1`

Proof log: `test_after.log`.
