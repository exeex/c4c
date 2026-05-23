Status: Active
Source Idea Path: ideas/open/385_aarch64_dispatch_publication_mechanical_split.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Extract Shared Publication Helpers

# Current Packet

## Just Finished

Step 1 repaired the shared-helper extraction boundary. `dispatch_publication_common.cpp/hpp`
now contains only the low-level Step 1 helpers: register aliasing, integer width
and power-of-two helpers, frame/stack/address helpers, scalar register/view and
load/store mnemonic helpers. Same-block producer/select-chain helpers and
later-cluster publication helpers were moved back to `dispatch_publication.cpp`.

## Suggested Next

Delegate Step 2 for the next mechanical publication split cluster after a
fresh AST caller/callee check.

## Watchouts

- Keep the split mechanical and behavior-preserving.
- Do not fold code into long-term feature homes such as `calls.cpp`,
  `memory.cpp`, or `comparison.cpp`.
- Left `immediate_integer_bits`, `emit_fp_immediate_to_register`, and
  `emit_fp_value_to_register` in `dispatch_publication.cpp` because the floating
  value path pulls in higher publication lowering dependencies including
  branch-fusion predicates, global-load symbol lookup, prepared value homes, and
  recursive value publication.
- Left same-block producer/select-chain helpers in `dispatch_publication.cpp`
  for Step 2: `find_same_block_binary_producer`,
  `find_same_block_select_producer`, `evaluate_same_block_integer_constant`,
  `select_chain_contains_direct_global_load`, and
  `find_same_block_named_producer`.
- Left later-cluster helpers such as local-slot address publication,
  prepared-global/va-list materialization, store-source register checks,
  `producer_instruction_index`, and `prepared_memory_access` in
  `dispatch_publication.cpp`; moving them now would broaden Step 1 beyond the
  low-level helper cluster.

## Proof

Passed. Ran exactly:
`{ cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10 && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure; } > test_after.log 2>&1`

Proof log: `test_after.log`.
