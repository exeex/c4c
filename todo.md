Status: Active
Source Idea Path: ideas/open/385_aarch64_dispatch_publication_mechanical_split.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Extract Edge Copy and Select-Chain Handling

# Current Packet

## Just Finished

Step 4 mechanically extracted the edge-copy and select-chain handling cluster
into `dispatch_edge_copies.cpp/hpp`: `EdgeProducerContext`,
`prepared_edge_select_source_is_destination_register`,
`unique_branch_predecessor_context`, `find_edge_named_producer`,
`prepared_memory_access`, `prepared_memory_access_matches_instruction`,
`edge_value_publication_may_read_register_index`,
`emit_edge_load_local_to_register`,
`emit_edge_value_publication_to_register`,
`lower_predecessor_join_source_publication`, `select_chain_label`,
`emit_select_chain_value_to_register`,
`make_select_chain_materialization_instruction`, and
`materialize_direct_global_select_chain_call_argument`. No Step 4-listed helper
was left in `dispatch_publication.cpp`; `dispatch_publication.hpp` re-exports
the new edge-copy header for existing users.

## Suggested Next

Delegate Step 5, the store source and writeback handling extraction, after a
fresh AST caller/callee check.

## Watchouts

- Keep the split mechanical and behavior-preserving.
- Do not fold code into long-term feature homes such as `calls.cpp`,
  `memory.cpp`, or `comparison.cpp`.
- `dispatch_publication.hpp` now includes `dispatch_edge_copies.hpp` so
  existing publication users continue to see edge/select declarations.
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
