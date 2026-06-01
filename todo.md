Status: Active
Source Idea Path: ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consume Prepared Edge Publication Sources

# Current Packet

## Just Finished

Completed Step 3's in-scope `dispatch_publication.cpp` source-selection audit
after the fused-compare and same-block producer migrations. No remaining
`dispatch_publication.cpp` source-selection helper performs raw BIR producer
discovery; the former `mir::find_same_block_named_producer_record` path is gone
and the live same-block and fused-compare queries route through prepared edge
publication source-producer facts.

## Suggested Next

Advance to Step 4: consume prepared typed stack and store-source publication
facts in `dispatch_publication.cpp` before local AArch64 load/store or copy
emission.

## Watchouts

- Remaining `std::get_if<bir::...>` checks in `prepared_source_producer_instruction`,
  `lower_missing_*publication`, and `value_publication_may_read_register_index`
  are target-local register parsing or instruction validation over prepared
  records, not source-selection authority.
- The local construction of
  `make_prepared_edge_publication_source_producer_lookups` in
  `prepared_publication_source_producer_for_value`,
  `prepared_same_block_publication_source_producer`, and
  `find_prepared_fused_compare_operand_producer` is an absent-lookup fallback
  for the same prepared source-producer records.
- Edge-copy source facts are already consumed by the edge-copy owner path
  (`dispatch_edge_copies.cpp`) via `PreparedEdgeCopySourceFacts`; no remaining
  Step 3 `dispatch_publication.cpp` migration was found there.
- Current-block join source facts are already consumed by the current-block
  join owner path (`dispatch_producers.cpp`) via
  `PreparedCurrentBlockJoinParallelCopySourceFacts`; the register-clobber check
  in `dispatch_publication.cpp` is current-block join owner residue, not a
  concrete source-selection migration.
- No concrete remaining Step 3 packet was identified in
  `dispatch_publication.cpp`.

## Proof

Command: `git diff --check -- todo.md`

Result: passed; no `test_after.log` was written for this documentation-only
audit packet.
