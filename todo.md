Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consume Prepared Edge Publications in AArch64

# Current Packet

## Just Finished

Continued Step 3 by tightening
`emit_edge_value_publication_to_register` recursive emission from prepared
edge-publication roots. Supported prepared roots now carry the prepared
producer context into cast, compare, arithmetic, and load-address dependency
materialization instead of letting those recursive operands start from the
original successor context.

Added an instruction-dispatch fixture that proves prepared-root emission ignores
a successor operand decoy producer and materializes the operand from the
prepared producer boundary plus prepared value home.

## Suggested Next

Continue Step 3 by reviewing unsupported prepared producer kinds that still
fail closed in AArch64 edge publication emission, especially non-binary
producer kinds that intentionally fall back only outside prepared-root mode.

## Watchouts

- The repaired `00183.c` path now emits predecessor-edge multiply
  materialization before entering the join, e.g. `mul w13, w13, w9` on both
  ternary incoming edges.
- Non-root operands under a prepared root now search from the prepared producer
  context and before-index; non-prepared calls still use the existing semantic
  producer/value-home lookup. Do not add named-case producer searches to extend
  this path.

## Proof

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepared_lookup_helper|backend_aarch64_prepared_branch_records|backend_aarch64_prepared_handoff_gate|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00183_c)$"'`
passed. Proof log: `test_after.log`.
