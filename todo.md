Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consume Prepared Edge Publications in AArch64

# Current Packet

## Just Finished

Started Step 3 by moving the AArch64 predecessor edge-publication materializer
onto prepared edge-publication source producer facts.

`lower_predecessor_select_parallel_copy_sources` now looks up the unique
prepared edge publication for the predecessor/successor/destination value and
requires the prepared source value id/name to match the move source before
materializing a computed edge source. The materializer consumes
`PreparedEdgePublication::source_producer_kind`,
`source_producer_block_label`, `source_producer_instruction_index`, and the
typed producer pointers for load-local, cast, binary, and select-style roots;
missing or stale prepared producer facts fail closed instead of falling back to
the broad AArch64 successor scan for the root publication.

Focused AArch64 instruction-dispatch fixtures that exercise computed
predecessor publications now publish matching prepared join/edge transfer facts
and attach `make_prepared_function_lookups` before dispatch.

## Suggested Next

Continue Step 3 by applying the same prepared-publication consumption boundary
to the remaining edge-publication hazard/read paths, especially
`edge_value_publication_may_read_register_index`, so recursive dependency checks
stop rediscovering root edge producers through the legacy AArch64 scan.

## Watchouts

- Manual AArch64 dispatch tests that expect computed predecessor publications
  now need both prepared join/edge transfer facts and attached
  `PreparedFunctionLookups`; a move bundle alone is intentionally insufficient
  for the new root materialization path.
- Recursive operand materialization still uses the existing value-publication
  path for non-root operands. The next packet should avoid replacing that with
  another target-local producer search and should consume prepared facts where
  available.

## Proof

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepared_lookup_helper|backend_aarch64_prepared_branch_records|backend_aarch64_prepared_handoff_gate|backend_aarch64_instruction_dispatch)$"'`
passed. Proof log: `test_after.log`.
