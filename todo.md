Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consume Prepared Edge Publications in AArch64

# Current Packet

## Just Finished

Continued Step 3 by teaching
`edge_value_publication_may_read_register_index` to consume a supplied
`PreparedEdgePublication` when the checked value is the prepared edge source.
Prepared-root dependency checks now resolve the producer from the prepared
publication and recurse from that producer block, bypassing the legacy
successor/edge/predecessor scan for supported prepared roots.

Added an instruction-dispatch fixture that proves a prepared edge dependency
check ignores a same-name successor decoy producer once a prepared root
producer fact is supplied.

## Suggested Next

Continue Step 3 by reviewing the remaining AArch64 edge-publication emission
recursions for unsupported producer kinds and non-root operands, then decide
whether another focused prepared-fact handoff is needed or whether the current
prepared edge-publication boundary is ready for supervisor review.

## Watchouts

- The repaired `00183.c` path now emits predecessor-edge multiply
  materialization before entering the join, e.g. `mul w13, w13, w9` on both
  ternary incoming edges.
- Non-root operands still use the existing semantic producer/value-home lookup
  because the current prepared edge-publication fact only identifies the edge
  source root. Do not add named-case producer searches to extend this path.

## Proof

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepared_lookup_helper|backend_aarch64_prepared_branch_records|backend_aarch64_prepared_handoff_gate|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00183_c)$"'`
passed. Proof log: `test_after.log`.
