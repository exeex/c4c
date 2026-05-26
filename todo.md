Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consume Prepared Edge Publications in AArch64

# Current Packet

## Just Finished

Repaired Step 3's AArch64 prepared edge-publication producer regression for
binary multiply roots. A predecessor edge publication whose prepared root is a
`PreparedEdgePublicationSourceProducerKind::Binary` `Mul` now materializes the
prepared producer through `emit_edge_value_publication_to_register` instead of
falling through to successor value-home rediscovery.

Unsupported prepared binary roots now fail closed once the prepared publication
matches the requested root value, so the root producer path does not silently
restore the broad successor scan. Added an instruction-dispatch fixture that
publishes a same-register predecessor multiply from prepared edge facts before
the branch.

## Suggested Next

Continue Step 3 by applying the prepared-publication consumption boundary to
the remaining edge-publication hazard/read paths, especially
`edge_value_publication_may_read_register_index`, so recursive dependency checks
stop rediscovering root edge producers through the legacy AArch64 scan.

## Watchouts

- The repaired `00183.c` path now emits predecessor-edge multiply
  materialization before entering the join, e.g. `mul w13, w13, w9` on both
  ternary incoming edges.
- Recursive operand materialization still uses the existing value-publication
  path for non-root operands. The next packet should consume prepared facts
  where available without adding testcase-shaped producer searches.

## Proof

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepared_lookup_helper|backend_aarch64_prepared_branch_records|backend_aarch64_prepared_handoff_gate|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00183_c)$"'`
passed. Proof log: `test_after.log`.
