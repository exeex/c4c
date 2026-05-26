Status: Active
Source Idea Path: ideas/open/39_aarch64_memory_foldback_after_store_source_planning.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Memory Store-Source Residue Ownership

# Current Packet

## Just Finished

Step 1 inventory completed for `memory_store_sources.*` without
implementation edits.

Helper groups:

- Prepared pointer/value publication emission helpers:
  `emit_prepared_pointer_value_load_to_register`,
  `lower_stack_homed_pointer_value_load_publication`,
  `lower_store_local_value_publication`,
  `lower_stack_homed_pointer_store_writeback`,
  `lower_pointer_base_plus_offset_store_local_publication`,
  `lower_pending_store_global_stack_value_publications`, and
  `lower_store_global_value_publication`.
- Store-source authority and retarget helpers:
  `store_local_uses_pointer_value_address` and
  `prepared_or_emitted_store_value_register`.
- Local slot/name/lane recovery helpers:
  `local_slot_reference_name`, `local_slot_reference_matches`,
  `prepared_frame_slot_object_name`, `prepared_load_local_frame_object_name`,
  `value_name_has_slot_prefix`, `parse_trailing_dot_offset`,
  `store_local_targets_logical_slot`, and
  `find_latest_narrow_store_for_wide_local_load`.
- Local producer/source classification helpers:
  `store_local_value_is_byval_frame_slot_load`,
  `store_local_value_is_wide_load_from_narrow_local_store`,
  `store_local_value_has_select_producer`,
  `store_local_value_has_scalar_fp_binary_producer`,
  `prepared_store_source_cast_producer`,
  `prepared_store_source_cast_producer_is_complete`, and
  `emit_scalar_conversion_cast_to_register`.
- Prepared store-source plan construction helpers:
  `store_local_destination_frame_slot`,
  `store_local_destination_stack_object`,
  `plan_store_local_source_publication`,
  `plan_stack_homed_pointer_store_writeback`, and
  `plan_pointer_base_plus_offset_store_local_publication`.
- Pointer/global symbol materialization helpers:
  `prepared_global_symbol_from_value_name`,
  `emit_global_symbol_address_to_register`, and
  `emit_pointer_base_plus_offset_to_register`.

Include sites:

- `src/backend/mir/aarch64/codegen/dispatch.cpp` includes
  `memory_store_sources.hpp`.
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` includes
  `memory_store_sources.hpp`.
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` includes
  `memory_store_sources.hpp`.
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
  includes `memory_store_sources.hpp`.
- `src/backend/mir/aarch64/codegen/memory_store_sources.cpp` includes its own
  header.

External call sites:

- `dispatch.cpp` calls
  `lower_pointer_base_plus_offset_store_local_publication`,
  `lower_store_local_value_publication`,
  `lower_stack_homed_pointer_store_writeback`,
  `lower_stack_homed_pointer_value_load_publication`,
  `lower_pending_store_global_stack_value_publications`, and
  `lower_store_global_value_publication`.
- `dispatch_publication.cpp` calls
  `store_local_uses_pointer_value_address` and
  `prepared_or_emitted_store_value_register`.
- `dispatch_value_materialization.cpp` calls
  `find_latest_narrow_store_for_wide_local_load` and
  `emit_prepared_pointer_value_load_to_register`.
- `backend_aarch64_prepared_memory_operand_records_test.cpp` calls
  `lower_store_local_value_publication` directly.

Build metadata:

- `src/backend/CMakeLists.txt` still builds
  `mir/aarch64/codegen/memory_store_sources.cpp`.

Declarations that must remain public during a mechanical fold:

- Keep declarations reachable from `memory.hpp` or another live owner header
  for all external call sites listed above.
- Helpers used only inside the folded implementation can become namespace-local
  in `memory.cpp`.
- The direct test include/call for `lower_store_local_value_publication` means
  that declaration must remain public unless the test is narrowly updated for
  deleted-header fallout.

Semantic blockers:

- A full mechanical fold of `memory_store_sources.cpp` into `memory.cpp` is
  blocked by helpers that still make or recover source choices locally:
  `find_latest_narrow_store_for_wide_local_load`,
  `store_local_recovered_narrow_store_source`,
  `store_local_value_is_byval_frame_slot_load`,
  `store_local_value_is_wide_load_from_narrow_local_store`,
  `store_local_value_has_select_producer`,
  `store_local_value_has_scalar_fp_binary_producer`, and
  `select_chain_contains_direct_global_load` use same-block producer scans,
  slot/object names, lane suffixes, or producer-class checks.
- `emit_pointer_base_plus_offset_to_register` also reaches back through
  `find_same_block_named_producer` for pointer-base load-local recovery before
  falling back to prepared value homes.
- These helpers are guarded by or feed
  `prepare::plan_prepared_store_source_publication`, but they are still
  AArch64-local semantic residue rather than pure prepared-fact consumption.
  Moving them wholesale would preserve the residue under a new owner and would
  not satisfy the source idea's "after store-source planning" constraint.

Focused memory/store-source test handles:

- `backend_store_source_publication_plan` covers target-neutral prepared
  store-source plan availability, recovered-source records, cast producers,
  pointer writeback, pointer-base-plus-offset, and missing-fact fail-closed
  behavior.
- `backend_aarch64_prepared_memory_operand_records` covers prepared pointer
  value store/load authority and directly calls
  `lower_store_local_value_publication`.
- `backend_aarch64_instruction_dispatch` covers local conversion store-source
  publication, prepared frame-slot/pointer-value stores, stack-homed
  pointer-value writeback/load chains, and missing store-source authority.
- `backend_aarch64_memory_operand_records` and
  `backend_aarch64_memory_operand_contract` are useful memory operand guards.

## Suggested Next

Do not start with a whole-file mechanical fold. First resolve or explicitly
split the semantic residue listed above so the fold-back does not merely move
AArch64-local store-source rediscovery into `memory.cpp`.

Recommended first mechanical fold-back packet after that blocker is handled:
move only the prepared-fact emission surface that is already driven by prepared
memory/value-home/store-source plans into `memory.cpp`, replace external
includes with `memory.hpp`, keep the externally called declarations public, and
leave any remaining semantic-source recovery helpers behind until the source
choice authority is fully target-neutral.

## Watchouts

Keep this route mechanical. Do not move target-neutral store-source facts,
rediscover semantic source choices in AArch64, weaken missing-fact fail-closed
behavior, change memory semantics, or mix in unrelated calls/dispatch/
comparison/prologue/module cleanup.

## Proof

No build required for inventory-only work.
