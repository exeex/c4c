# AArch64 Dispatch Publication Prepared Authority Repair

## Goal

Repair duplicate publication-authority recovery in
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp` by consuming shared
value-home, block-entry-publication, scalar-publication, branch-condition,
move-bundle, store-source, and memory facts, and by adding narrowly scoped
shared queries for the remaining producer and frame-offset gaps.

## Why This Exists

The audit found several publication paths still recover semantic source,
home, producer, return ABI, or local-slot facts inside AArch64 even though the
shared prepared layer already owns or should own those decisions.

## Owned File

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`

## Duplicated Helpers And Fallback Paths

- `current_block_entry_publication_register` falls back to value-home name
  scans when `prepared_named_value_id` does not resolve a BIR value.
- `collect_current_block_entry_publications`,
  `value_has_current_block_entry_publication`,
  `record_current_block_entry_publication_registers`, and
  `block_entry_move_clobbers_current_join_publication` rebuild current
  block-entry publication state.
- `lower_missing_conditional_branch_condition_publication` recovers a branch
  condition producer through a same-block producer scan.
- `lower_missing_fused_compare_operand_publication` and
  `lower_missing_fused_compare_operand_publications` need a scalar-publication
  route for fused-compare operands instead of ad hoc source scans.
- `memory_load_result_feeds_before_return_fpr_abi` inside
  `retarget_memory_result_to_prepared_home` scans raw move bundles for
  before-return FPR ABI consumers.
- `lower_local_slot_address_publication`, `local_slot_address_frame_offset`,
  and `local_aggregate_address_frame_offset` recover frame offsets from stack
  object source kind, object names, lane `.0`, and frame-slot scans.
- `lower_fixed_formal_store_local_publication` and
  `store_local_value_is_fixed_formal` should consume store-source publication
  planning rather than BIR parameter identity.

## Shared Facts To Consume Or Add

- Consume `PreparedValueHomeLookups::value_ids`,
  `find_prepared_value_id`, `find_indexed_prepared_value_home`, and
  `PreparedBlockEntryPublication`.
- Consume `PreparedBlockEntryPublication`,
  `PreparedBlockEntryPublicationStatus`,
  `collect_prepared_block_entry_publications`, and
  `prepared_block_entry_publication_available`.
- Consume `PreparedBranchCondition`, `PreparedScalarPublicationPlan`,
  `PreparedEdgePublicationSourceProducerLookups`, `PreparedValueHome`, and
  `PreparedMemoryAccess` where they already cover publication sources.
- Add or consume a prepared branch-condition producer instruction index or a
  scalar-publication query for materialized branch conditions and
  fused-compare operands if `plan_prepared_scalar_publication` cannot express
  the hook.
- Add a before-return move lookup by source value id and destination register
  bank if existing `PreparedMoveBundleLookups` cannot answer it.
- Add or consume a local-slot address/frame-offset query keyed by value or
  stack object only if pointer-base-plus-offset homes and
  `PreparedAddressMaterialization` cannot cover the path.
- Consume `PreparedStoreSourcePublicationPlan`,
  `PreparedStoreSourcePublicationInputs`, and
  `plan_prepared_store_source_publication` for fixed-formal store publication.

## Out Of Scope

- Do not change register spelling, scratch choice, emitted move order,
  frame-address instruction spelling, or store/load mnemonics except as a
  consequence of consuming shared facts.
- Do not classify `value_publication_may_read_register_index` register-hazard
  behavior as duplicate authority unless it starts reselecting semantic source
  or home facts.
- Do not combine this repair with comparison or memory follow-up scopes.

## Acceptance Criteria

- Manual value-home iteration and block-entry publication reconstruction are
  replaced by indexed prepared lookup consumption.
- Branch-condition and fused-compare publication paths use a shared producer
  or scalar-publication contract, or add the missing shared field/query first.
- Before-return FPR ABI retargeting no longer scans raw move bundles once a
  shared source-value/destination-bank lookup exists.
- Local-slot/aggregate address publication consumes pointer-base-plus-offset,
  address-materialization, or aggregate stack authority instead of string/lane
  scans.
- Fixed-formal store publication consumes store-source publication planning.

## Reviewer Reject Signals

- Reject manual iteration over `value_locations->value_homes` by spelling when
  indexed value-id/home lookups are available.
- Reject same-block condition producer scans, named branch-condition
  shortcuts, or fused-compare operand scans as durable authority.
- Reject raw `move_bundles` scans for before-return publication facts after a
  shared lookup is available.
- Reject lane-name or stack-object string matching such as `.0` as local-slot
  address authority.
- Reject expectation downgrades, unsupported-test rewrites, broad rewrites
  outside this file, or helper renames claimed as capability progress.
