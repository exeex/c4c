Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Share Reusable Copy Planning Decisions

# Current Packet

## Just Finished

Step 4 bounded audit continued for AArch64-local edge-copy suppression and
source-publication predicates in `dispatch_producers.cpp` and
`dispatch_edge_copies.cpp`.

Moved the target-neutral prepared-move/home decision behind shared helpers:
`prepare::prepared_out_of_ssa_parallel_copy_register_destination_matches_value`
classifies exact out-of-SSA parallel-copy `Move` records that write a value to
an assigned register destination, and
`prepare::prepared_out_of_ssa_parallel_copy_source_shares_destination_register`
classifies exact prepared source/destination home register-name equality for
that move. `dispatch_producers.cpp` now uses those helpers for current-block
join parallel-copy source suppression.

The audit kept AArch64-local policy in AArch64: stack-source suppression,
physical register aliasing, current-join clobber checks, scratch selection,
instruction spelling, and publication emission remain target code. The
`dispatch_edge_copies.cpp` block-entry suppression path already consumes the
shared redundant-copy helper; its remaining suppression predicates are the
target-local clobber, alias, and memory-source policies.

## Suggested Next

Continue Step 4 with a bounded audit of the source-publication match in
`lower_predecessor_select_parallel_copy_sources`, if the supervisor wants one
more shared helper. Keep the packet limited to exact prepared publication,
move, and source-home identity; leave emission, scratch choice, and failure
fallback behavior in AArch64.

## Watchouts

- `prepared_value_homes_share_register_name` is exact prepared register-name
  equality only. It is not an AArch64 alias/hazard helper; `registers_alias`,
  scratch selection, instruction spelling, and encoding limits remain in
  AArch64.
- The new parallel-copy source helper is intentionally narrower than the old
  AArch64 predicate: stack-source suppression remains local target policy and
  is not classified as a shared prepared-register match.
- The redundant-copy helper remains intentionally exact: copied move records
  without the original `source_move` pointer do not satisfy it.
- Memory-source suppression remains target-specific in AArch64. The prepared
  facts can prove assigned-storage redundancy for a move, but they do not encode
  AArch64's current policy of suppressing all block-entry memory-source edge
  moves.
- Current-join clobber checks and physical register aliasing remain AArch64
  decisions.

## Proof

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$"'`
passed. Proof log: `test_after.log`.
