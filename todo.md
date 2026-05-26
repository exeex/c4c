Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consume Prepared Edge Publications in AArch64

# Current Packet

## Just Finished

Continued Step 3 with a focused exhaustion audit of the remaining
`find_edge_named_producer` consumers and prepared-root fallback points in
`dispatch_edge_copies.cpp`.

Closed the remaining root-level prepared publication mismatch path:
`emit_edge_value_publication_to_register` and the public root call to
`edge_value_publication_may_read_register_index` now fail closed when a
prepared publication does not describe the requested root value, instead of
falling through to legacy producer rediscovery. Internal prepared operand
hazard checks still run as non-root checks so scratch preservation keeps using
prepared producer context.

Added focused instruction-dispatch coverage proving mismatched prepared root
emission and dependency checks fail closed.

## Suggested Next

Have the supervisor review the Step 3 prepared edge-publication slice for
acceptance/commit readiness, then decide whether Step 3 is exhausted or whether
another prepared edge-publication family needs a similarly focused audit.

## Watchouts

- The repaired `00183.c` path now emits predecessor-edge multiply
  materialization before entering the join, e.g. `mul w13, w13, w9` on both
  ternary incoming edges.
- Exhaustion audit finding: the only direct `find_edge_named_producer` consumers
  left in `dispatch_edge_copies.cpp` are the non-prepared fallback branches of
  `edge_value_publication_may_read_register_index` and
  `emit_edge_value_publication_to_register_impl`; prepared root calls now either
  consume `prepared_edge_publication_producer_context` or fail closed.
- Non-root operands under a prepared root now search from the prepared producer
  context and before-index; non-prepared calls still use the existing semantic
  producer/value-home lookup. Do not add named-case producer searches to extend
  this path.
- Prepared select-root labels are now rooted once per active select chain; the
  label counter remains shared across recursive edge-publication materialization
  so separate nested select regions do not reuse synthetic label indices.
- The prepared select-root path still intentionally handles the supported scalar
  compare/select shape directly and leaves other prepared non-binary roots
  fail-closed while prepared-root mode is active.

## Proof

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepared_lookup_helper|backend_aarch64_prepared_branch_records|backend_aarch64_prepared_handoff_gate|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00183_c)$"'`
passed. Proof log: `test_after.log`.
