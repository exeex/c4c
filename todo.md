Status: Active
Source Idea Path: ideas/open/39_aarch64_memory_foldback_after_store_source_planning.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Mechanical Store-Source Inventory

# Current Packet

## Just Finished

Step 1 refreshed the post-39a mechanical ownership inventory for
`memory_store_sources.*`.

Remaining top-level definitions in
`src/backend/mir/aarch64/codegen/memory_store_sources.cpp`:
`store_local_uses_pointer_value_address`,
`prepared_or_emitted_store_value_register`,
`emit_prepared_pointer_value_load_to_register`,
`lower_stack_homed_pointer_value_load_publication`,
`prepared_recovered_narrow_store_source`,
`prepared_store_source_producer`,
`prepared_store_source_producer_is_complete`,
`prepared_store_source_cast_producer_is_complete`,
`prepared_store_source_select_producer_is_complete`,
`prepared_store_source_scalar_fp_binary_producer_is_complete`,
`emit_scalar_conversion_cast_to_register`,
`store_local_destination_frame_slot`,
`store_local_destination_stack_object`,
`plan_store_local_source_publication`,
`plan_stack_homed_pointer_store_writeback`,
`plan_pointer_base_plus_offset_store_local_publication`,
`lower_store_local_value_publication`,
`lower_stack_homed_pointer_store_writeback`,
`prepared_global_symbol_from_value_name`,
`emit_global_symbol_address_to_register`,
`emit_pointer_base_plus_offset_to_register`,
`lower_pointer_base_plus_offset_store_local_publication`,
`is_store_global_select_snapshot_run_instruction`,
`lower_pending_store_global_stack_value_publications`, and
`lower_store_global_value_publication`.

Current `memory_store_sources.hpp` declarations additionally include a
declared-only `store_local_frame_slot_offset`; no definition or live caller was
found in source, tests, or build metadata.

External include sites for `memory_store_sources.hpp`:
`src/backend/mir/aarch64/codegen/dispatch.cpp`,
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp`,
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`, and
`tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`.

External call sites that must stay reachable after the move:
`dispatch.cpp` calls `lower_pointer_base_plus_offset_store_local_publication`,
`lower_store_local_value_publication`,
`lower_stack_homed_pointer_store_writeback`,
`lower_stack_homed_pointer_value_load_publication`,
`lower_pending_store_global_stack_value_publications`, and
`lower_store_global_value_publication`;
`dispatch_publication.cpp` calls `store_local_uses_pointer_value_address` and
`prepared_or_emitted_store_value_register`;
`dispatch_value_materialization.cpp` calls
`emit_prepared_pointer_value_load_to_register`; and
`backend_aarch64_prepared_memory_operand_records_test.cpp` directly calls
`lower_store_local_value_publication`.

Declarations that must remain public after folding into the memory owner:
`store_local_uses_pointer_value_address`,
`prepared_or_emitted_store_value_register`,
`emit_prepared_pointer_value_load_to_register`,
`lower_stack_homed_pointer_value_load_publication`,
`lower_store_local_value_publication`,
`lower_stack_homed_pointer_store_writeback`,
`lower_pointer_base_plus_offset_store_local_publication`,
`lower_pending_store_global_stack_value_publications`, and
`lower_store_global_value_publication`.
`emit_scalar_conversion_cast_to_register`,
`prepared_global_symbol_from_value_name`,
`emit_global_symbol_address_to_register`,
`emit_pointer_base_plus_offset_to_register`,
`is_store_global_select_snapshot_run_instruction`, and the private
prepared-plan helpers have no external caller and can become namespace-local
inside `memory.cpp`. The declared-only `store_local_frame_slot_offset` can be
removed unless a later compile proves an unseen dependency.

Build metadata still lists
`"${CMAKE_CURRENT_SOURCE_DIR}/mir/aarch64/codegen/memory_store_sources.cpp"` in
`src/backend/CMakeLists.txt`.

Search confirmation: no remaining `find_same_block`,
`producer_instruction_index`, `select_chain_contains_direct_global_load`,
`prepared_local_load_offset`, `same-block`, or `same block` source-choice
recovery matches were found in `memory_store_sources.*`. Remaining source
classification references consume prepared authority:
`find_prepared_recovered_narrow_store_source_for_wide_local_load`,
`prepared_store_source_load_local_is_byval_formal_pointer_source`,
`find_prepared_store_source_direct_global_select_chain_dependency`, prepared
source-producer completeness checks, and prepared value-home facts. No
AArch64-local semantic source-choice recovery remains in the folded helper
surface inventoried here.

## Suggested Next

Run the first mechanical fold-back packet: move all definitions from
`memory_store_sources.cpp` into `memory.cpp`, move the still-public declarations
above into `memory.hpp`, convert the no-external-caller helpers to
namespace-local implementation detail, replace external includes of
`memory_store_sources.hpp` with `memory.hpp` where needed, remove
`memory_store_sources.cpp` from `src/backend/CMakeLists.txt`, and delete the
obsolete `memory_store_sources.*` files.

Recommended focused proof for that code packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_store_source_publication_plan|backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_memory_operand_contract)$' | tee test_after.log`

## Watchouts

Keep the fold mechanical. Do not change prepared store-source behavior,
pointer-base value-home handling, memory diagnostics, or test expectations.
Do not reintroduce same-block producer scans under a new AArch64 helper name.
The only non-code fallout expected is include replacement and build metadata
removal.

## Proof

No build required for this inventory-only packet. Read-only proof commands run:

`c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/aarch64/codegen/memory_store_sources.cpp build/compile_commands.json`

`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/memory_store_sources.cpp build/compile_commands.json`

`rg -n "memory_store_sources" src/backend/mir/aarch64/codegen tests/backend/mir src/backend/CMakeLists.txt`

`rg -n "\b(store_local_uses_pointer_value_address|prepared_or_emitted_store_value_register|emit_prepared_pointer_value_load_to_register|lower_stack_homed_pointer_value_load_publication|emit_scalar_conversion_cast_to_register|lower_store_local_value_publication|lower_stack_homed_pointer_store_writeback|prepared_global_symbol_from_value_name|emit_global_symbol_address_to_register|emit_pointer_base_plus_offset_to_register|store_local_frame_slot_offset|lower_pointer_base_plus_offset_store_local_publication|is_store_global_select_snapshot_run_instruction|lower_pending_store_global_stack_value_publications|lower_store_global_value_publication)\b" src/backend/mir/aarch64/codegen tests/backend/mir src/backend/CMakeLists.txt`

`rg -n "find_same_block|producer_instruction_index|select_chain_contains_direct_global_load|prepared_local_load_offset|LoadLocalInst|StoreLocalInst|same-block|same block|find_prepared_store_source|prepared_store_source|recovered|byval|direct_global|select_chain" src/backend/mir/aarch64/codegen/memory_store_sources.cpp src/backend/mir/aarch64/codegen/memory_store_sources.hpp`

Result: inventory complete; no implementation files, tests, build files, plan,
or source idea files edited.
