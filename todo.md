Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consume Prepared Edge Publications in AArch64

# Current Packet

## Just Finished

Continued Step 3 by comparing prepared select-root lowering with the generic
select-chain path and threading prepared edge-publication select lowering
through shared chain state. Nested prepared selected values now share the root
label identity and monotonic label counter, and prepared edge select recursion
has the same active selected-value cycle guard shape as generic select-chain
materialization.

Added focused instruction-dispatch coverage for a prepared select root whose
selected value is another select, proving nested labels use the root select
instruction/value identity with label indices `0` then `1`.

## Suggested Next

Continue Step 3 by having the supervisor review this prepared select-root slice
for acceptance/commit readiness, then choose the next prepared edge-publication
family or decide whether Step 3 is exhausted.

## Watchouts

- The repaired `00183.c` path now emits predecessor-edge multiply
  materialization before entering the join, e.g. `mul w13, w13, w9` on both
  ternary incoming edges.
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
