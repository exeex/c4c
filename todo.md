Status: Active
Source Idea Path: ideas/open/50_aarch64_memory_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Memory Recovery Sites

# Current Packet

## Just Finished

Step 1 audit completed for
`src/backend/mir/aarch64/codegen/memory.cpp`. The duplicate recovery families
from idea 50 are grouped into bounded follow-up packets below.

### Audit Findings

- Value-home id recovery:
  `find_value_home_id`, `find_unique_value_home_id`,
  `apply_load_identity`, `apply_store_identity`, and the va-list field
  load/store record builders still recover value ids by scanning
  `value_locations.value_homes`.
  Replacement authority is available through
  `PreparedValueHomeLookups::value_ids`,
  `prepare::find_indexed_prepared_value_id`, and the existing AArch64
  `prepared_value_id` / `find_value_home` context helpers. Repair packet:
  thread the indexed value-home lookup/regalloc authority into the memory
  record helpers, then delete the local scan helpers.

- Prepared memory record consumers:
  `make_memory_record_from_prepared_access`,
  `make_prepared_*_memory_instruction_record`, `apply_load_identity`, and
  `apply_store_identity` are already anchored on `PreparedMemoryAccess`,
  `PreparedAddressingFunction`, `PreparedValueLocationFunction`, and
  `PreparedStoragePlanFunction`. The bounded repair is to keep these as
  consumers while replacing only their remaining value-id scan fallbacks with
  indexed value-home authority; no new memory-access query is currently
  missing.

- Before-return retargeting:
  `find_memory_return_abi_register` scans raw `move_bundles` for
  `PreparedMovePhase::BeforeReturn` and
  `PreparedMoveDestinationKind::FunctionReturnAbi`.
  Replacement authority is available through
  `prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank`
  and `context.function.move_bundle_lookups`, keyed by block index, source
  value id, and destination bank. Repair packet: derive the destination bank
  from the load result type, consume the prepared lookup, and preserve the ABI
  register conversion/spelling behavior. Watch the current raw path's
  `destination_register_name` fallback because the indexed lookup is populated
  from `destination_register_placement`.

- Local-address store stack-object recovery:
  `find_stack_object_by_value_name` and
  `rewrite_local_address_store_value` recover source stack objects through
  prepared value spelling and stack-object name equality before calling
  `prepare::find_prepared_frame_slot`.
  Exact replacement authority is not fully exposed as a shared query. Repair
  packet: add or consume a prepared stack-object/frame-slot lookup keyed by
  function plus value name/object id, then route local-address store value
  rewriting through that query and remove text-name matching.

- Stack-homed pointer value load publication:
  `emit_prepared_pointer_value_load_to_register` and
  `lower_stack_homed_pointer_value_load_publication` already consume
  `PreparedMemoryAccess` plus prepared value-home facts for pointer/result
  homes. No separate missing query was found here. Repair packet only if
  implementation of adjacent pointer-base work touches this route: preserve it
  as a prepared consumer and avoid introducing new value-spelling authority.

- Store-local source publication:
  `plan_store_local_source_publication`, `prepared_store_source_producer`, and
  `prepared_recovered_narrow_store_source` already feed
  `PreparedStoreSourcePublicationInputs` into
  `plan_prepared_store_source_publication`, using
  `PreparedEdgePublicationSourceProducerLookups`,
  `find_prepared_recovered_narrow_store_source_for_wide_local_load`,
  `prepared_store_source_load_local_is_byval_formal_pointer_source`, and
  `find_prepared_store_source_direct_global_select_chain_dependency`.
  Repair packet: keep this as the prepared store-local authority and limit
  changes to removing any obsolete local fallback introduced by the value-home,
  pointer-base, or store-global packets.

- Pointer-base-plus-offset/global symbol recovery:
  `prepared_global_symbol_from_value_name`,
  `emit_pointer_base_plus_offset_to_register`, and
  `lower_pointer_base_plus_offset_store_local_publication` still derive a
  global symbol from value-name spelling, including `@`-prefix handling.
  Existing prepared authority includes
  `PreparedValueHomeKind::PointerBasePlusOffset`,
  `pointer_base_value_name`, `pointer_byte_delta`, and the store-source
  publication plan's source pointer-base fields, but no shared query currently
  exposes "this pointer base is global symbol/link name X" without spelling
  recovery. Repair packet: add or consume a prepared pointer-base global-symbol
  field/query, then use it for pointer-base materialization and store-local
  publication.

- Store-global publication state:
  `lower_pending_store_global_stack_value_publications` scans forward through
  same-block select/load/binary/cast/store-global runs, and
  `lower_store_global_value_publication` carries duplicate state in the local
  `published_stack_values` set plus the `stack_homes_only` flag.
  `PreparedStoreSourcePublicationPlan` already has
  `StoreGlobalPublication`, `pending_publication`, `stack_homes_only`, and
  `duplicate_publication` fields, but memory lowering does not consume a
  shared query that identifies pending candidates or duplicate stack-home-only
  publications. Repair packet: add/consume a prepared store-global
  publication plan/query carrying those fields, then remove the local forward
  scan/set as semantic authority.

## Suggested Next

Delegate Step 2 value-home id recovery: replace the
`value_locations.value_homes` scans in memory record construction and va-list
field record paths with indexed prepared value-home id lookup authority.

## Watchouts

Keep the first implementation packet narrow: value-home id lookup only. The
return-retargeting packet can use the existing before-return prepared move
lookup, while local-address stack-object recovery, pointer-base global-symbol
recovery, and store-global pending/duplicate publication each need a small
shared prepared query before deleting local recovery. Do not expand into ALU,
publication, dispatch, comparison, calls, or value-materialization repair.

## Proof

Audit-only packet; no build/test proof required by the supervisor. Did not
modify `test_before.log` or `test_after.log`.
