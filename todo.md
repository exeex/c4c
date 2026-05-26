Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Share Reusable Copy Planning Decisions

# Current Packet

## Just Finished

Step 4 bounded audit completed for redundant block-entry parallel-copy
suppression.

Moved the target-neutral "this exact prepared edge publication marks its
block-entry parallel-copy move redundant by assigned storage" decision into
`prepare::prepared_edge_publication_redundant_block_entry_parallel_copy_move`.
The helper requires an available publication, the exact prepared move pointer,
an out-of-SSA block-entry `Move`, a matching `parallel_copy_step_index`, and
`matching_move_redundant_by_assigned_storage`.

AArch64 now consults that shared helper when prepared edge-publication lookups
are attached, then keeps its existing target-local fallbacks for register alias
suppression, memory-source suppression, and current-join clobber checks.
Focused helper coverage proves redundant, non-redundant, and mismatched-step
classifications.

## Suggested Next

Continue Step 4 with a bounded audit of remaining AArch64-local edge-copy
suppression and source-publication predicates in `dispatch_producers.cpp` and
`dispatch_edge_copies.cpp`, looking only for decisions already expressible from
prepared homes/publications without moving aliasing, scratch, encoding, or
memory-source policy.

## Watchouts

- `prepared_value_homes_share_register_name` is exact prepared register-name
  equality only. It is not an AArch64 alias/hazard helper; `registers_alias`,
  scratch selection, instruction spelling, and encoding limits remain in
  AArch64.
- The redundant-copy helper is intentionally exact: copied move records without
  the original `source_move` pointer do not satisfy it.
- Memory-source suppression remains target-specific in AArch64. The prepared
  facts can prove assigned-storage redundancy for a move, but they do not encode
  AArch64's current policy of suppressing all block-entry memory-source edge
  moves.
- Current-join clobber checks and physical register aliasing remain AArch64
  decisions.

## Proof

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$"'`
passed. Proof log: `test_after.log`.
