Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Share Reusable Copy Planning Decisions

# Current Packet

## Just Finished

Plan-owner lifecycle decision: Step 3 is exhausted after the prepared
edge-publication consumption/audit slice through commit `471b0539a`.

The Step 3 completion check is satisfied: prepared-root emission now consumes
`prepared_edge_publication_producer_context` or fails closed, focused AArch64
coverage still covers the prepared join/select/branch/block-entry paths, and
the only direct `find_edge_named_producer` consumers left in
`dispatch_edge_copies.cpp` are non-prepared fallback branches rather than broad
prepared-publication rediscovery.

Advanced canonical execution state to Step 4 without rewriting `plan.md`,
because the active runbook contract still matches the source idea.

## Suggested Next

Start Step 4 with a narrow copy-planning audit in
`dispatch_edge_copies.cpp` and shared prepared lookup helpers. Identify one
decision family that depends only on value homes, edge identity, or publication
phase, such as redundant-copy suppression or ordering facts, and move only that
target-neutral decision behind shared prepared helpers if the audit confirms it
does not depend on AArch64 register hazards or instruction encoding.

## Watchouts

- The repaired `00183.c` path now emits predecessor-edge multiply
  materialization before entering the join, e.g. `mul w13, w13, w9` on both
  ternary incoming edges.
- Step 4 must keep physical register hazards, scratch choice, instruction
  spelling, and target encoding limits in AArch64.
- Exhaustion audit finding from Step 3: the only direct
  `find_edge_named_producer` consumers left in `dispatch_edge_copies.cpp` are
  the non-prepared fallback branches of
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
