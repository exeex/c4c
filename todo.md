Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Share Reusable Copy Planning Decisions

# Current Packet

## Just Finished

Step 4 narrow audit completed for same-register prepared-home copy planning.

Moved the target-neutral "source and destination prepared homes name the same
assigned register" decision from the local AArch64 predicate body into shared
`prepare::prepared_value_homes_share_register_name`. AArch64 still calls its
existing wrapper, so current producer and edge-copy consumers keep their local
shape, while the reusable decision now lives behind prepared lookup helpers.

Focused helper coverage now proves same-register prepared homes are recognized
and different-register, unnamed-register, and non-register homes are rejected.

## Suggested Next

Continue Step 4 with a second bounded audit around redundant block-entry
parallel-copy suppression, especially facts already exposed on
`PreparedEdgePublication` such as `matching_move_redundant_by_assigned_storage`
and `parallel_copy_step_index`, before touching any target emission logic.

## Watchouts

- `prepared_value_homes_share_register_name` is exact prepared register-name
  equality only. It is not an AArch64 alias/hazard helper; `registers_alias`,
  scratch selection, instruction spelling, and encoding limits remain in
  AArch64.
- The current migration is intentionally behavior-preserving: it does not change
  `dispatch_producers.cpp` call sites or move block-entry clobber checks.
- Do not treat source/destination same-register equality as enough to suppress
  memory-source moves; `should_emit_block_entry_edge_copy_move` still keeps
  AArch64 memory and current-join clobber checks local.

## Proof

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$"'`
passed. Proof log: `test_after.log`.
