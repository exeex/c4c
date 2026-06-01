Status: Active
Source Idea Path: ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consume Prepared Edge Publication Sources

# Current Packet

## Just Finished

Started Step 3 by mapping `dispatch_publication.cpp` source-selection decisions
to existing prepared publication facts. No implementation files were changed.

| `dispatch_publication.cpp` decision | Current local path | Existing prepared fact | Step 3 note |
| --- | --- | --- | --- |
| Current block entry publication register reuse | `collect_current_block_entry_publications`, `current_block_entry_publication_register`, and `record_current_block_entry_publication_registers` consume `PreparedBlockEntryPublication` and parse destination registers locally. | `PreparedBlockEntryPublication` from value-location block-entry publications. | Already consumes prepared block-entry publication availability; AArch64 register parsing/recording should stay local. |
| Generic prepared producer lookup for scalar publication helpers | `prepared_publication_source_producer_for_value` finds `PreparedEdgePublicationSourceProducer` from `prepared_lookups->edge_publication_source_producers`, or rebuilds local producer lookups when only `prepared` plus `control_flow` are present. | `PreparedEdgePublicationSourceProducerLookups` and `PreparedEdgePublicationSourceProducer`. | Prepared producer authority exists, but the local fallback rebuild is a candidate for tightening only when the caller can require canonical prepared lookups. |
| Conditional branch condition materialization | `lower_missing_conditional_branch_condition_publication` looks up the branch condition and requires a prepared source producer before calling `lower_scalar_control_value_instruction`. | `PreparedBranchCondition` plus `PreparedEdgePublicationSourceProducer`. | Already producer-gated; no edge-copy source fact needed here. |
| Fused compare operand publication | `lower_missing_fused_compare_operand_publication` first tries a prepared source producer, then falls back to `plan_prepared_scalar_publication` / value-home publication. `lower_missing_fused_compare_operand_publications` separately checks producer kind to prefer a scratch for select materialization. | `PreparedEdgePublicationSourceProducer`; related richer prepared helpers exist as `find_prepared_fused_compare_operand_producer` and `find_prepared_materialized_condition_producer`. | First narrow migration target: replace the local producer-kind/scratch probe for fused compare operands with a prepared fused-compare/materialized-condition helper, if its inputs cover the same same-block and before-instruction constraints. |
| Move-clobber guard for current-block join publication | `block_entry_move_clobbers_current_join_publication` scans current block entry publications and aliases parsed destination registers against the move destination. | `PreparedBlockEntryPublication`; current-block join source aggregation exists as `PreparedCurrentBlockJoinParallelCopySourceFacts`. | Possible later target, but it is a clobber guard over already prepared destinations rather than source selection. |
| Recursive register-read query for publication dependencies | `value_publication_may_read_register_index` checks current block entry publication, then follows `PreparedEdgePublicationSourceProducer`; if no prepared producer matches, it falls back to `mir::find_same_block_named_producer_record`. | `PreparedEdgePublicationSourceProducer`, `PreparedSameBlockScalarProducer`, and `PreparedCurrentBlockJoinParallelCopySourceFacts` for join parallel-copy sources. | Candidate blocker/target: the raw same-block fallback is remaining local source discovery. A narrow migration should first test whether `find_prepared_same_block_scalar_producer` can replace it without losing the current fallback behavior when prepared lookups are absent. |
| Edge-copy source facts | Not directly consumed by `dispatch_publication.cpp`; direct consumers are in `dispatch_edge_copies.cpp`. | `PreparedEdgeCopySourceFacts`, `prepare_edge_copy_source_facts`, and `prepare_block_entry_parallel_copy_edge_source_facts` validate/copy `PreparedEdgePublication` source value, source home, producer kind, producer pointer, and source memory access. | Existing fact surface is sufficient for edge-copy source authority, but migrating it belongs in a code packet that touches `dispatch_edge_copies.cpp`, not this read-only mapping packet. |
| Current-block join parallel-copy sources | Not directly consumed by `dispatch_publication.cpp`; prepared query is wrapped in `dispatch_producers.cpp`. | `PreparedCurrentBlockJoinParallelCopySourceFacts` via `prepare_current_block_join_parallel_copy_source_facts`. | Source facts exist for current-block join parallel-copy sources. Any Step 3 code packet should decide whether `dispatch_publication.cpp` needs this query directly or whether the current owner remains `dispatch_producers.cpp`. |

## Suggested Next

First narrow Step 3 migration target: in `dispatch_publication.cpp`, replace
the fused-compare operand source producer selection with existing prepared
fused-compare/materialized-condition producer helpers, or stop if those helpers
cannot preserve the current same-block and before-instruction constraints.

Suggested proof for that future code packet: focused AArch64 dispatch/publication
build or CTest subset that exercises fused compare operand publication and
materialized condition publication.

## Watchouts

- `PreparedEdgeCopySourceFacts` already copies and validates producer kind,
  source value/home, producer pointers, and source memory access from
  `PreparedEdgePublication`; do not recreate that validation locally.
- `dispatch_publication.cpp` does not currently call
  `prepare_edge_copy_source_facts`,
  `prepare_block_entry_parallel_copy_edge_source_facts`, or
  `prepare_current_block_join_parallel_copy_source_facts`.
- `value_publication_may_read_register_index` still has raw
  `mir::find_same_block_named_producer_record` fallback source discovery after
  prepared producer lookup misses. That is the clearest remaining local
  source-selection residue, but it may be blocked if prepared lookups are not
  guaranteed on every caller path.

## Proof

Command: `git diff --check -- todo.md`

Result: passed.
