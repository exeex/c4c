# Current Packet

Status: Active
Source Idea Path: ideas/open/47_aarch64_dispatch_edge_copies_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Use prepared move-bundle authority for block-entry edge copies

## Just Finished

Step 4 routed block-entry edge-copy suppression through prepared
move-bundle/edge-publication authority. `should_emit_block_entry_edge_copy_move`
now uses `find_unique_indexed_block_entry_parallel_copy_edge_publication`,
`prepared_edge_publication_redundant_block_entry_parallel_copy_move`, and
`prepared_edge_publication_matches_parallel_copy_move_source` instead of local
same-register or source-memory suppression. `lower_predecessor_select_parallel_copy_sources`
continues to start from the prepared block-entry move bundle and now relies on
the edge-publication source-match helper before attempting source
materialization.

The AArch64 dispatch test now covers a redundant out-of-SSA block-entry
edge-copy route where prepared edge-publication authority suppresses the move,
and the same record emits when prepared lookup authority is absent.

## Suggested Next

Implement Step 5 by routing select-chain/direct-global late call-argument
materialization through the chosen prepared call-plan or shared select-chain
dependency authority.

## Watchouts

`find_edge_named_producer` and `unique_branch_predecessor_context` remain
defined/exported, but the repaired source-producer materialization and hazard
paths no longer call them. Load-local materialization still allows the
non-direct/null-publication prepared-memory-access route; the direct prepared
publication route now requires available source-memory authority. Block-entry
edge-copy suppression no longer drops same-register or source-memory moves
without prepared edge-publication authority.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log`

Result: passed, 2/2 focused tests green. Proof log: `test_after.log`.
