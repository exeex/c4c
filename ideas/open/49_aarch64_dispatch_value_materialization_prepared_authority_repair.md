# AArch64 Dispatch Value Materialization Prepared Authority Repair

## Goal

Repair duplicate value-materialization authority in
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` by routing
semantic source, producer, publication, memory, global, select-chain, and
local-slot address facts through shared prepared records or narrowly added
shared queries.

## Why This Exists

The audit found that value materialization still performs same-block producer
recursion and source reconstruction for named values, loads, globals, selects,
and local-slot addresses. Those paths should consume existing prepared source
and home facts or expose a missing shared query rather than becoming a second
semantic authority in AArch64.

## Owned File

- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`

## Duplicated Helpers And Fallback Paths

- `emit_value_publication_to_register` same-block producer path
  (`find_same_block_named_producer` plus recursive cast/binary/load/select
  lowering) rederives scalar source producer and operand chains.
- `current_block_entry_publication_register`,
  `value_has_current_block_entry_publication`, and
  `prepared_value_home_for_value` are valid only as consumers of prepared
  block-entry/value-home facts.
- The load-local branch using `prepared_local_load_offset`,
  `emit_prepared_pointer_value_load_to_register`, and
  `find_prepared_recovered_narrow_store_source_for_wide_local_load` must stay
  anchored in prepared memory and recovered store-source facts.
- The load-global branch (`find_load_global_target`,
  `load_global_symbol_label`, GOT/direct emission) may need a prepared
  global-load address/materialization query.
- The select branch calling `emit_select_chain_value_to_register` with
  `prepared_named_value_id` duplicates select-chain materialization for
  non-edge/non-store consumers.
- The add/sub address path through
  `emit_local_slot_address_publication_to_register` shares the local-slot
  address/frame-offset gap found in publication.

## Shared Facts To Consume Or Add

- Consume `PreparedEdgePublicationSourceProducerLookups`,
  `PreparedEdgePublicationSourceProducer`, `PreparedScalarPublicationPlan`,
  and `PreparedValueHome` for scalar materialization sources.
- Consume `PreparedBlockEntryPublication`, `PreparedValueHome`,
  `collect_prepared_block_entry_publications`,
  `prepared_block_entry_publication_available`, and
  `find_indexed_prepared_value_home`.
- Consume `PreparedMemoryAccess`, `PreparedAddressingFunction`,
  `PreparedStoreSourcePublicationPlan`, and
  `find_prepared_recovered_narrow_store_source_for_wide_local_load` for
  load-local source materialization.
- Add a prepared scalar-materialization/source-producer query by value and
  instruction index only if non-edge callers cannot consume existing prepared
  producers.
- Add or consume a prepared global-load address/materialization query keyed by
  value or instruction if `PreparedAddressMaterialization` cannot carry
  GOT/direct policy.
- Add or consume a prepared scalar select-chain materialization query for
  non-edge/non-store consumers if shared producer facts are insufficient.
- Reuse the prepared local-slot address/frame-offset query from the
  publication repair if pointer-base-plus-offset homes cannot cover it.

## Out Of Scope

- Do not change recursive operand emission, scratch/read-write hazard checks,
  load/global address spelling, compare/select instruction spelling, or
  AArch64-specific GOT/direct emission mechanics except to consume a shared
  semantic contract.
- Do not expand this idea into calls, ALU, comparison, or memory repairs.
- Do not count removal of lines as success without removing duplicate semantic
  recovery.

## Acceptance Criteria

- Non-edge callers of `emit_value_publication_to_register` are audited and
  either consume existing prepared producer/home facts or motivate one shared
  scalar materialization query.
- Same-block named producer recursion no longer grows as the source of truth
  for scalar materialization.
- Load-local source materialization consumes prepared memory/recovered-source
  facts without neighbor scans.
- Global-load and select-chain paths either consume shared authority or add a
  shared query before removing the local fallback.
- Local-slot address materialization uses the same shared authority selected
  for publication repair.

## Reviewer Reject Signals

- Reject deeper same-block producer recursion or named-case producer shortcuts
  in `emit_value_publication_to_register`.
- Reject replacing prepared block-entry/value-home checks with raw move-bundle
  or value-name scans.
- Reject hard-coded global-name spelling, GOT policy, or direct-global
  select-chain shortcuts as the repair.
- Reject classifying scratch-order checks alone as duplicate authority unless
  semantic source/home facts are also reselected locally.
- Reject expectation downgrades, unsupported-test rewrites, or helper renames
  claimed as semantic capability progress.
