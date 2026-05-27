# AArch64 Dispatch Publication Prepared Authority Repair Runbook

Status: Active
Source Idea: ideas/open/48_aarch64_dispatch_publication_prepared_authority_repair.md

## Purpose

Turn `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` into a
consumer of shared prepared value-home, block-entry-publication,
scalar-publication, branch-condition, move-bundle, store-source, and memory
facts instead of locally rediscovering the same publication authority.

## Goal

Remove or fail-close duplicate publication-authority recovery in AArch64
publication lowering while preserving emitted behavior.

## Core Rule

Prefer indexed prepared/shared authority over local rediscovery. Do not add new
value-home scans, same-block producer scans, raw move-bundle scans, lane-name
matching, or expectation downgrades as progress.

## Read First

- `ideas/open/48_aarch64_dispatch_publication_prepared_authority_repair.md`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- Prepared authority definitions and lookup helpers for:
  - `PreparedValueHomeLookups::value_ids`
  - `find_prepared_value_id`
  - `find_indexed_prepared_value_home`
  - `PreparedBlockEntryPublication`
  - `PreparedBlockEntryPublicationStatus`
  - `collect_prepared_block_entry_publications`
  - `prepared_block_entry_publication_available`
  - `PreparedBranchCondition`
  - `PreparedScalarPublicationPlan`
  - `PreparedEdgePublicationSourceProducerLookups`
  - `PreparedValueHome`
  - `PreparedMemoryAccess`
  - `PreparedMoveBundleLookups`
  - `PreparedStoreSourcePublicationPlan`
  - `PreparedStoreSourcePublicationInputs`
  - `plan_prepared_store_source_publication`

## Current Targets

- `current_block_entry_publication_register`
- `collect_current_block_entry_publications`
- `value_has_current_block_entry_publication`
- `record_current_block_entry_publication_registers`
- `block_entry_move_clobbers_current_join_publication`
- `lower_missing_conditional_branch_condition_publication`
- `lower_missing_fused_compare_operand_publication`
- `lower_missing_fused_compare_operand_publications`
- `memory_load_result_feeds_before_return_fpr_abi`
- `retarget_memory_result_to_prepared_home`
- `lower_local_slot_address_publication`
- `local_slot_address_frame_offset`
- `local_aggregate_address_frame_offset`
- `lower_fixed_formal_store_local_publication`
- `store_local_value_is_fixed_formal`

## Non-Goals

- Do not change register spelling, scratch choice, emitted move order,
  frame-address instruction spelling, or store/load mnemonics unless required
  to consume shared authority.
- Do not classify `value_publication_may_read_register_index` register-hazard
  behavior as duplicate authority unless it starts reselecting semantic source
  or home facts.
- Do not combine this repair with comparison, memory, ALU, calls, or other
  AArch64 follow-up scopes.
- Do not claim line-count reduction, helper renames, or expectation rewrites as
  capability progress.
- Do not downgrade supported-path tests to `unsupported`.

## Working Model

- Indexed value-id and value-home lookups own value-home identity.
- Prepared block-entry-publication records own current block-entry publication
  availability and register facts.
- Prepared branch-condition or scalar-publication authority should own missing
  branch-condition and fused-compare operand publication sources.
- Prepared move-bundle lookups should own before-return FPR ABI retargeting
  facts once a source-value/destination-bank query exists.
- Pointer-base-plus-offset homes, address materialization, or aggregate stack
  authority should own local-slot and aggregate-address frame offsets.
- Prepared store-source publication planning owns fixed-formal store
  publication classification.

## Execution Rules

- Keep each code-changing step behavior-preserving unless the source idea
  explicitly requires fail-closed behavior for unsupported local rediscovery.
- Before adding a new query, prove the existing prepared lookup surface cannot
  answer the required publication context.
- Add only narrowly scoped shared queries for real producer, move, or
  frame-offset gaps and make the owning authority visible in tests.
- Each implementation packet needs fresh build or compile proof. The supervisor
  chooses the exact proving subset.
- Narrow proof must cover the repaired publication route and any retained
  fallback path for that route.

## Steps

### Step 1: Map publication authority gaps

Goal: establish where `dispatch_publication.cpp` already has prepared authority
and where it still reconstructs publication facts locally.

Primary target: `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`

Actions:

- Inspect block-entry publication helpers and identify which manual value-home
  or current-publication scans can be replaced with indexed prepared lookups.
- Inspect missing branch-condition and fused-compare publication helpers and
  record whether existing scalar-publication or branch-condition plans cover
  each source.
- Inspect before-return FPR ABI retargeting and decide whether existing
  move-bundle lookups can answer source-value plus destination-bank queries.
- Inspect local-slot, aggregate-address, and fixed-formal store publication
  helpers for string, lane, or BIR-parameter identity recovery.

Completion check:

- `todo.md` names the exact first fallback route to repair and the proof subset
  the supervisor selected for that route.

### Step 2: Replace value-home and block-entry publication reconstruction

Goal: make current block-entry publication decisions consume prepared value
identity and block-entry-publication records.

Primary target: `current_block_entry_publication_register`,
`collect_current_block_entry_publications`,
`value_has_current_block_entry_publication`,
`record_current_block_entry_publication_registers`, and
`block_entry_move_clobbers_current_join_publication`

Actions:

- Prefer `PreparedValueHomeLookups::value_ids`, `find_prepared_value_id`, and
  `find_indexed_prepared_value_home` over manual value-home iteration by name.
- Consume `PreparedBlockEntryPublication`,
  `PreparedBlockEntryPublicationStatus`,
  `collect_prepared_block_entry_publications`, and
  `prepared_block_entry_publication_available` for current publication facts.
- Remove or fail-close local current-publication reconstruction that duplicates
  prepared block-entry authority.

Completion check:

- Block-entry publication decisions depend on indexed value-home and prepared
  block-entry publication facts, with narrow proof for the repaired route.

### Step 3: Route branch-condition and fused-compare publications through shared source authority

Goal: stop same-block producer and operand scans from owning missing condition
or fused-compare publication sources.

Primary target: `lower_missing_conditional_branch_condition_publication`,
`lower_missing_fused_compare_operand_publication`, and
`lower_missing_fused_compare_operand_publications`

Actions:

- Consume existing `PreparedBranchCondition`,
  `PreparedScalarPublicationPlan`, `PreparedEdgePublicationSourceProducerLookups`,
  `PreparedValueHome`, and `PreparedMemoryAccess` where they cover the route.
- If `plan_prepared_scalar_publication` cannot express the needed hook, add the
  minimal shared producer instruction index or scalar-publication query first.
- Do not add same-block condition producer scans, named branch-condition
  shortcuts, or fused-compare operand scans as durable authority.

Completion check:

- Missing branch-condition and fused-compare publication lowering use shared
  producer/scalar-publication authority, with proof covering both the repaired
  route and any retained fallback.

### Step 4: Replace before-return FPR ABI move-bundle scans

Goal: make memory-result retargeting consume shared move-bundle authority
instead of scanning raw move bundles.

Primary target: `memory_load_result_feeds_before_return_fpr_abi` inside
`retarget_memory_result_to_prepared_home`

Actions:

- Check whether existing `PreparedMoveBundleLookups` can answer the needed
  source value id plus destination register bank query.
- Add a narrow shared lookup only if the existing move-bundle index cannot
  answer the route.
- Replace raw `move_bundles` scans with the shared lookup once available.

Completion check:

- Before-return FPR ABI retargeting no longer scans raw move bundles and has
  proof for the retargeted memory-result route.

### Step 5: Route local-slot and aggregate-address publication through prepared address authority

Goal: remove frame-offset recovery based on stack object spelling, lane `.0`,
or frame-slot scans.

Primary target: `lower_local_slot_address_publication`,
`local_slot_address_frame_offset`, and `local_aggregate_address_frame_offset`

Actions:

- Prefer pointer-base-plus-offset homes, `PreparedAddressMaterialization`, or
  aggregate stack authority for address/frame-offset facts.
- Add a local-slot address/frame-offset query keyed by value or stack object
  only if existing prepared address authority cannot cover the path.
- Reject lane-name or stack-object string matching as local-slot address
  authority.

Completion check:

- Local-slot and aggregate-address publication consume prepared address or
  aggregate stack authority, with proof for the affected address publication
  route.

### Step 6: Consume store-source publication planning for fixed-formal stores

Goal: make fixed-formal store publication classification consume prepared
store-source publication planning.

Primary target: `lower_fixed_formal_store_local_publication` and
`store_local_value_is_fixed_formal`

Actions:

- Consume `PreparedStoreSourcePublicationPlan`,
  `PreparedStoreSourcePublicationInputs`, and
  `plan_prepared_store_source_publication`.
- Remove or fail-close BIR parameter identity checks that duplicate
  store-source publication planning.
- Keep final store publication behavior stable unless prepared authority proves
  the previous path unsupported.

Completion check:

- Fixed-formal store publication is driven by prepared store-source planning,
  with proof for the repaired fixed-formal store route.

### Step 7: Consolidate proof and close readiness

Goal: ensure the completed repair is not a testcase-overfit slice.

Primary target: lifecycle proof in `todo.md`, `test_before.log`, and
`test_after.log`

Actions:

- Confirm all remaining local fallback paths are removed, fail-closed, or
  documented as shared-query gaps with corresponding shared queries.
- Confirm no expectations were downgraded and no unsupported-path rewrites were
  used as capability proof.
- Run the supervisor-selected broader validation after the repaired publication
  routes have landed or the repair is ready to close.

Completion check:

- `todo.md` records fresh proof covering value-home/block-entry publication,
  branch-condition and fused-compare publication, before-return FPR ABI
  retargeting, local-slot/aggregate-address publication, and fixed-formal
  store-source publication as applicable.
