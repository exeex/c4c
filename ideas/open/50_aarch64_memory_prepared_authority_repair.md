# AArch64 Memory Prepared Authority Repair

## Goal

Repair duplicate memory-authority recovery in
`src/backend/mir/aarch64/codegen/memory.cpp` by making memory lowering consume
shared prepared memory, value-home, move, store-source, address-materialization,
and aggregate stack facts, and by adding missing shared lookups for the
remaining return, stack-object, pointer-base, and store-global gaps.

## Why This Exists

The audit found that memory lowering already has many prepared records
available, but still carries value-name scans, move-bundle scans, stack-object
name matching, pointer-base spelling recovery, and pending store-global
publication tracking that duplicate or should feed shared prepared authority.

## Owned File

- `src/backend/mir/aarch64/codegen/memory.cpp`

## Duplicated Helpers And Fallback Paths

- `find_value_home_id` and `find_unique_value_home_id` loop over
  `value_locations.value_homes` for value-name to prepared value-id recovery.
- `make_memory_record_from_prepared_access`,
  `make_prepared_*_memory_instruction_record`, `apply_load_identity`, and
  `apply_store_identity` must remain consumers of prepared memory identity,
  not raw BIR reconstruction paths.
- `find_memory_return_abi_register` and
  `retarget_load_result_to_return_abi` scan raw move bundles for before-return
  ABI destinations.
- `find_stack_object_by_value_name` and `rewrite_local_address_store_value`
  recover stack object/frame slot facts through value and object spelling.
- `emit_prepared_pointer_value_load_to_register` and
  `lower_stack_homed_pointer_value_load_publication` should consume prepared
  pointer-value and value-home facts.
- `plan_store_local_source_publication`, `prepared_store_source_producer`, and
  `prepared_recovered_narrow_store_source` should feed or consume store-source
  publication planning.
- `prepared_global_symbol_from_value_name`,
  `emit_pointer_base_plus_offset_to_register`, and
  `lower_pointer_base_plus_offset_store_local_publication` recover global
  pointer-base identity from value spelling.
- `lower_pending_store_global_stack_value_publications` and
  `lower_store_global_value_publication` track pending/duplicate store-global
  publication state locally.

## Shared Facts To Consume Or Add

- Consume `PreparedValueHomeLookups::value_ids`,
  `find_prepared_value_id`, `find_indexed_prepared_value_id`, and
  `PreparedFunctionLookups::value_homes`.
- Consume `PreparedMemoryAccess`, `PreparedAddressingFunction`,
  `PreparedValueHome`, `PreparedStoragePlanFunction`,
  `find_prepared_memory_access`, and `find_prepared_value_home`.
- Add a before-return move lookup by source value id and destination register
  bank if `PreparedMoveBundleLookups` cannot answer the return retarget query.
- Add or consume a prepared stack-object/frame-slot lookup keyed by value name
  or object id for local-address store values.
- Consume `PreparedStoreSourcePublicationPlan`,
  `PreparedStoreSourcePublicationInputs`,
  `PreparedStoreSourcePublicationIntent::PointerStoreWriteback`,
  `PreparedMemoryAccess`, `PreparedValueHome`,
  `PreparedEdgePublicationSourceProducerLookups`,
  `find_prepared_recovered_narrow_store_source_for_wide_local_load`,
  `prepared_store_source_load_local_is_byval_formal_pointer_source`, and
  `find_prepared_store_source_direct_global_select_chain_dependency`.
- Add or consume a prepared global-symbol pointer-base field/query for
  `PreparedValueHomeKind::PointerBasePlusOffset` if value spelling is the only
  current source.
- Add or consume a prepared store-global publication plan/query carrying
  pending and duplicate stack-homes-only state.

## Out Of Scope

- Do not change load/store opcode choice, offset folding, pointer-register
  materialization, duplicate-emission avoidance, source reload sequencing, or
  stack/global store instruction spelling except as needed to consume a shared
  semantic contract.
- Do not fold ALU, publication, or value-materialization repair into this
  idea.
- Do not treat target-local source reloads or scratch choices as duplicate
  authority by themselves.

## Acceptance Criteria

- Value-home id recovery uses prepared indexed lookup helpers instead of
  manual `value_homes` scans.
- Memory records consume `PreparedMemoryAccess` and related storage/value-home
  facts without raw BIR address/value identity reconstruction.
- Before-return load retargeting uses a shared source-value/destination-bank
  move query or adds that query before removing raw bundle scans.
- Local-address store, pointer-base-plus-offset, and global-symbol paths no
  longer depend on value spelling or stack-object name matching.
- Store-local, pointer-store writeback, and store-global publication behavior
  is represented through `PreparedStoreSourcePublicationPlan` or a new shared
  query with explicit pending/duplicate state.

## Reviewer Reject Signals

- Reject new manual scans over `value_locations.value_homes` or raw BIR memory
  operands where `PreparedMemoryAccess` exists.
- Reject raw `move_bundles` scans for before-return retargeting after a shared
  lookup is available.
- Reject text-name matching between values, stack objects, or `@`-prefixed
  globals as durable authority.
- Reject forward same-block store-global scans or ad hoc published-stack-value
  sets as the long-term publication state.
- Reject expectation downgrades, unsupported-test rewrites, broad memory
  rewrites outside this file's authority repair, or helper renames claimed as
  progress.
