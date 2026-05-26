Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Share Reusable Copy Planning Decisions

# Current Packet

## Just Finished

Step 4 bounded audit completed for the repeated block-entry edge-publication
lookup shape in `dispatch_edge_copies.cpp`.

The lookup is target-neutral and now lives behind
`prepare::find_unique_indexed_block_entry_parallel_copy_edge_publication`.
The helper takes prepared edge-publication lookups, an expected
predecessor/successor tuple, and a `PreparedMoveResolution`, then returns the
unique available block-entry publication for that edge and destination value.
It rejects invalid labels, non-value destinations, non-`Move` operations, and
optional per-move predecessor/successor labels that conflict with the expected
tuple.

Both AArch64 users now share the helper:
`should_emit_block_entry_edge_copy_move` still asks
`prepared_edge_publication_redundant_block_entry_parallel_copy_move` whether the
move is suppressible, and
`lower_predecessor_select_parallel_copy_sources` still asks
`prepared_edge_publication_matches_parallel_copy_move_source` whether the
publication exactly matches the source. The audit kept target policy in AArch64:
bundle/record authority validation, register aliasing, stack-source eligibility,
memory-source suppression, current-join clobber checks, scratch selection,
instruction emission, and diagnostics were not moved.

## Suggested Next

No further implementation packet is identified for this narrowed Step 4 lookup
audit. Supervisor should review the completed Step 4 helper slice and decide
whether to commit it, request independent review, or route the next lifecycle
packet.

## Watchouts

- `find_unique_indexed_block_entry_parallel_copy_edge_publication` intentionally
  does not require `move.authority_kind == OutOfSsaParallelCopy`: existing valid
  prepared fixtures carry out-of-SSA authority on the block-entry bundle/record
  while copied move facts may leave per-move authority unset.
- The helper also does not decide source eligibility, redundancy, or exact
  source-home identity. Those remain in the existing shared predicates and the
  AArch64 call-site policy.
- `clang-format` is not installed in this environment; formatting was checked
  with `git diff --check`.

## Proof

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$"'`
passed. Proof log: `test_after.log`.
