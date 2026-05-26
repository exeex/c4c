Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consume Prepared Edge Publications in AArch64

# Current Packet

## Just Finished

Continued Step 3 by adding explicit prepared-root lowering for
`SelectMaterialization` in `emit_edge_value_publication_to_register`. Prepared
select roots now materialize compare operands and selected values through the
prepared producer context, and non-binary prepared roots that are not explicitly
supported fail closed instead of falling through to generic AArch64 value
publication rediscovery.

Added an instruction-dispatch fixture that proves prepared select-root emission
uses the prepared producer boundary for generated select-chain labels and
materializes operands from prepared homes.

## Suggested Next

Continue Step 3 by reviewing whether prepared select-root lowering needs the
same nested-select cycle guard and deeper selected-value coverage as the generic
select-chain path before treating this family as exhausted.

## Watchouts

- The repaired `00183.c` path now emits predecessor-edge multiply
  materialization before entering the join, e.g. `mul w13, w13, w9` on both
  ternary incoming edges.
- Non-root operands under a prepared root now search from the prepared producer
  context and before-index; non-prepared calls still use the existing semantic
  producer/value-home lookup. Do not add named-case producer searches to extend
  this path.
- The prepared select-root path intentionally handles the supported scalar
  compare/select shape directly and leaves other prepared non-binary roots
  fail-closed while prepared-root mode is active.

## Proof

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepared_lookup_helper|backend_aarch64_prepared_branch_records|backend_aarch64_prepared_handoff_gate|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00183_c)$"'`
passed. Proof log: `test_after.log`.
