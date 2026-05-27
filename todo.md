# Current Packet

Status: Active
Source Idea Path: ideas/open/48_aarch64_dispatch_publication_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map publication authority gaps

## Just Finished

Step 1 mapping completed for
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp`.

Mapped prepared-authority and fallback gaps:

- Block-entry publication: `collect_current_block_entry_publications` already
  consumes `collect_prepared_block_entry_publications`, and
  `record_current_block_entry_publication_registers` already requires
  `prepared_block_entry_publication_available`. The remaining fallback is in
  `current_block_entry_publication_register`, which falls back to iterating
  `value_locations->value_homes` and comparing prepared value spelling when
  `prepared_named_value_id` misses. `value_has_current_block_entry_publication`
  also treats a raw move/destination match as authority without checking
  `PreparedBlockEntryPublicationStatus`, and
  `block_entry_move_clobbers_current_join_publication` hand-filters unsupported
  statuses instead of using `prepared_block_entry_publication_available`.
- Branch and fused-compare publication:
  `lower_missing_conditional_branch_condition_publication` still recovers the
  condition source with `find_same_block_named_producer`. Fused-compare
  lowering consumes `PreparedBranchCondition` for the operand list, but
  `lower_missing_fused_compare_operand_publication` still locally resolves the
  destination home, scratch target, and value publication route instead of
  consuming a `PreparedScalarPublicationPlan` or shared producer query for the
  operand publication.
- Before-return FPR ABI retargeting:
  `memory_load_result_feeds_before_return_fpr_abi` scans raw
  `value_locations->move_bundles` for a `BeforeReturn` move with matching
  source value and FPR return ABI destination. Existing
  `PreparedMoveBundleLookups` only indexes bundles by phase/block/instruction
  position, so this route needs a shared source-value plus destination-bank
  query before the scan can be removed.
- Local-slot and aggregate-address publication:
  `local_slot_address_frame_offset` scans prepared stack objects by
  `source_kind == "local_slot"`, compares object spelling, and then scans frame
  slots for the lowest offset. `local_aggregate_address_frame_offset` derives
  aggregate authority by appending the lane suffix `.0`. This should move to
  pointer-base-plus-offset homes, `PreparedAddressMaterialization`, or a narrow
  shared frame-offset query keyed by prepared value/stack object.
- Fixed-formal store publication:
  `store_local_value_is_fixed_formal` reclassifies fixed formals by iterating
  BIR params and matching prepared names/types. `lower_fixed_formal_store_local_publication`
  already has the source value, emitted scalar, and prepared destination memory
  access via `prepared_frame_slot_load_address`; classification should consume
  `PreparedStoreSourcePublicationPlan` with `StoreLocalPublication` intent
  instead of BIR parameter identity.

Selected first repair route: Step 2 should repair the block-entry publication
fallback first, starting with the `current_block_entry_publication_register`
value-home spelling scan. The route should use
`PreparedValueHomeLookups::value_ids`, `find_prepared_value_id`, and
`find_indexed_prepared_value_home`, and should fail closed when indexed
prepared value identity is absent. The same slice should switch
`value_has_current_block_entry_publication` and
`block_entry_move_clobbers_current_join_publication` to prepared block-entry
publication availability/status authority where needed.

## Suggested Next

Implement Step 2 for the block-entry publication route described above. Keep
the behavior change limited to indexed value-home lookup and prepared
block-entry publication availability/status consumption.

## Watchouts

Keep the source idea stable during routine execution. Do not combine this
publication repair with the open comparison, memory, ALU, calls, or other
AArch64 follow-up scopes. Reject value-home spelling scans, same-block producer
scans, raw move-bundle scans, lane-name matching, and expectation downgrades as
progress.

For later steps, branch-condition publication likely needs shared
producer/scalar-publication authority before removing the same-block producer
scan; before-return FPR retargeting needs a source-value/destination-bank
move-bundle query; local-slot address publication needs prepared address or
frame-offset authority instead of object/lane spelling; fixed-formal stores
should be classified by store-source publication planning.

## Proof

Mapping-only packet; no build required and no log files were touched.

Supervisor-selected likely implementation proof subset for the first repair
route:

`cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_branch_control_lowering|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log`
