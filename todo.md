Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Share Reusable Copy Planning Decisions

# Current Packet

## Just Finished

Step 4 bounded audit continued for the source-publication match in
`lower_predecessor_select_parallel_copy_sources`.

Moved the target-neutral exact prepared publication/move/source-home identity
behind `prepare::prepared_edge_publication_matches_parallel_copy_move_source`.
The helper requires an available edge publication, exact linked
`PreparedMoveResolution` identity, block-entry publication phase, matching
destination value id, exact source-home pointer, matching source-home kind,
matching source value id/name, a value-destination `Move`, and a non-immediate
source. `dispatch_edge_copies.cpp` now uses that helper before asking AArch64 to
materialize the predecessor join-source publication.

The audit kept target policy in AArch64: source eligibility as shared-register
or stack source, register parsing/aliasing, scratch choice, publication
emission, instruction materialization, and fail-closed fallback behavior remain
local to `dispatch_edge_copies.cpp`. Optional parallel-copy step facts and
per-move authority fields were not moved into the helper because this site has
valid publication-linked prepared moves that do not populate those optional
fields; the stable shared fact is the publication's exact move pointer plus
source-home/source-id/name identity.

## Suggested Next

Continue Step 4 with a bounded audit for any remaining duplicated prepared-fact
predicates in `dispatch_edge_copies.cpp` that are exact lookup/source identity
checks. Do not move AArch64 register, stack-source, scratch, or emission policy
into shared helpers.

## Watchouts

- `prepared_value_homes_share_register_name` is exact prepared register-name
  equality only. It is not an AArch64 alias/hazard helper; `registers_alias`,
  scratch selection, instruction spelling, and encoding limits remain in
  AArch64.
- The parallel-copy source helpers are intentionally narrower than the
  AArch64 predicates: stack-source handling remains local target policy and is
  not classified as a shared prepared-register match.
- The redundant-copy helper remains intentionally exact: copied move records
  without the original `source_move` pointer do not satisfy it.
- `prepared_edge_publication_matches_parallel_copy_move_source` intentionally
  does not require optional parallel-copy step fields or per-move authority
  fields. The edge-publication lookup can link a valid move through
  block-entry out-of-SSA bundle facts even when those optional fields are not
  populated on the move itself.
- Memory-source suppression remains target-specific in AArch64. The prepared
  facts can prove assigned-storage redundancy for a move, but they do not encode
  AArch64's current policy of suppressing all block-entry memory-source edge
  moves.
- Current-join clobber checks and physical register aliasing remain AArch64
  decisions.

## Proof

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$"'`
passed. Proof log: `test_after.log`.
