Status: Active
Source Idea Path: ideas/open/385_aarch64_dispatch_publication_mechanical_split.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Same-Block Producer Discovery

# Current Packet

## Just Finished

Step 2 mechanically extracted the same-block producer discovery cluster into
`dispatch_producers.cpp/hpp`: `find_same_block_binary_producer`,
`find_same_block_select_producer`, `evaluate_same_block_integer_constant`,
`select_chain_contains_direct_global_load`, `find_same_block_named_producer`,
`producer_instruction_index`, `find_load_global_target`,
`load_global_symbol_label`, and `is_current_block_join_parallel_copy_source`.
The moved `find_bir_block` definition remains in `dispatch_producers.cpp`, with
only `dispatch_publication.cpp` carrying the local declaration it needs.

## Suggested Next

Delegate the next mechanical publication split cluster after a fresh AST
caller/callee check, keeping value materialization and store-source helpers out
of the producer-discovery slice.

## Watchouts

- Keep the split mechanical and behavior-preserving.
- Do not fold code into long-term feature homes such as `calls.cpp`,
  `memory.cpp`, or `comparison.cpp`.
- `dispatch_publication.hpp` includes `dispatch_producers.hpp` so existing
  dispatch/call users continue to see producer declarations.
- Do not re-expose `find_bir_block` from `dispatch_producers.hpp`; `dispatch.cpp`
  has an existing anonymous-namespace helper with the same name, and
  publication keeps its own local forward declaration for the moved definition.

## Proof

Passed. Ran exactly:
`{ cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10 && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure; } > test_after.log 2>&1`

Proof log: `test_after.log`.
