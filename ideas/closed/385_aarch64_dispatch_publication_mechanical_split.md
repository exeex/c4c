# 385 AArch64 Dispatch Publication Mechanical Split

## Context

Idea 384 split the original `dispatch.cpp` enough that the main dispatcher is now
smaller, but `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` became the
new oversized bucket. It is currently over 4000 lines and contains around 99
function definitions.

The word `publication` is a temporary project-local label, not a standard backend
term. In this file it currently mixes several concepts:

- value materialization
- prepared home fulfillment
- same-block producer lookup
- select-chain materialization
- edge/join copy source handling
- store operand/source legalization
- pointer/store writeback helpers
- generic frame/register/mnemonic helpers

This idea is only about splitting that oversized file into smaller mechanical
pieces. Do not finalize terminology or move code into long-term homes such as
`calls.cpp`, `memory.cpp`, or `comparison.cpp` yet. After the file is split, review
the shape and decide which pieces should be renamed or merged.

## Goal

Split `dispatch_publication.cpp` into smaller files while preserving behavior.
The immediate target is to make the mixed responsibilities visible and keep every
implementation file under 4000 lines.

## Non-Goals

- Do not redesign the dispatch architecture.
- Do not rewrite the lowering behavior.
- Do not change generated assembly.
- Do not rename all `publication` concepts in this pass unless needed for the
  mechanical split.
- Do not fold code into `calls.cpp`, `memory.cpp`, `comparison.cpp`, or other
  existing feature files yet.
- Do not add testcase-shaped special cases or expectation rewrites.

## Required Tooling

Executor packets for this idea should use `.codex/skills/c4c-clang-tools`.
Before moving a cluster, inspect function signatures and at least one caller or
callee relationship for the cluster:

```bash
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch_publication.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch_publication.cpp <function-name> build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch_publication.cpp <function-name> build/compile_commands.json
```

Use these results to keep mechanical move boundaries small and explainable.

## Suggested First-Pass Split

Use provisional names. They are allowed to be renamed in a later cleanup idea.

### `dispatch_publication_common.cpp/hpp`

Low-level helpers that are shared by multiple clusters:

- `registers_alias`
- `integer_bit_width`
- `power_of_two_shift`
- `find_frame_slot`
- `find_stack_object`
- `prepared_frame_slot_load_address`
- `relocation_operand`
- `register_indirect_address`
- `fixed_slots_use_frame_pointer`
- `frame_slot_address`
- `scalar_view_for_type`
- `gp_register_name`
- `scalar_load_mnemonic`
- `dispatch_publication_scalar_type_size_bytes`
- `scalar_load_mnemonic_for_width`
- `scalar_store_mnemonic`
- `scalar_integer_width_bits`
- `scalar_gp_register_view`
- `scalar_fp_register_view`
- `immediate_integer_bits`

### `dispatch_producers.cpp/hpp`

Same-block producer and select-chain discovery:

- `find_same_block_binary_producer`
- `find_same_block_select_producer`
- `evaluate_same_block_integer_constant`
- `select_chain_contains_direct_global_load`
- `find_same_block_named_producer`
- `producer_instruction_index`
- `find_load_global_target`
- `load_global_symbol_label`
- `find_bir_block`
- `is_current_block_join_parallel_copy_source`

### `dispatch_value_materialization.cpp/hpp`

Prepared value/home materialization:

- `emit_fp_immediate_to_register`
- `emit_fp_value_to_register`
- `emit_prepared_global_symbol_load_to_register`
- `emit_prepared_va_list_field_load_to_register`
- `emit_prepared_pointer_value_load_to_register`
- `emit_prepared_value_home_to_register`
- `emit_value_publication_to_register`
- `lower_local_slot_address_publication`
- `lower_stack_homed_pointer_value_load_publication`
- `lower_scalar_cast_publication_to_prepared_register`
- `lower_scalar_fp_binary_publication_to_prepared_register`
- `lower_scalar_cast_publication_to_prepared_stack`

### `dispatch_edge_copies.cpp/hpp`

Edge/predecessor/join source handling:

- `prepared_edge_select_source_is_destination_register`
- `unique_branch_predecessor_context`
- `find_edge_named_producer`
- `prepared_memory_access`
- `prepared_memory_access_matches_instruction`
- `edge_value_publication_may_read_register_index`
- `emit_edge_load_local_to_register`
- `emit_edge_value_publication_to_register`
- `lower_predecessor_join_source_publication`
- `select_chain_label`
- `emit_select_chain_value_to_register`
- `make_select_chain_materialization_instruction`
- `materialize_direct_global_select_chain_call_argument`

### `dispatch_store_sources.cpp/hpp`

Store source legalization and writeback:

- `store_local_uses_pointer_value_address`
- `prepared_or_emitted_store_value_register`
- `local_slot_reference_name`
- `local_slot_reference_matches`
- `prepared_frame_slot_object_name`
- `prepared_load_local_frame_object_name`
- `value_name_has_slot_prefix`
- `parse_trailing_dot_offset`
- `store_local_targets_logical_slot`
- `find_latest_narrow_store_for_wide_local_load`
- `store_local_value_is_byval_frame_slot_load`
- `store_local_value_is_wide_load_from_narrow_local_store`
- `store_local_value_cast_producer`
- `store_local_value_has_select_producer`
- `store_local_value_has_scalar_fp_binary_producer`
- `emit_scalar_conversion_cast_to_register`
- `lower_store_local_value_publication`
- `lower_stack_homed_pointer_store_writeback`
- `prepared_global_symbol_from_value_name`
- `emit_global_symbol_address_to_register`
- `emit_pointer_base_plus_offset_to_register`
- `store_local_frame_slot_offset`
- `lower_pointer_base_plus_offset_store_local_publication`
- `is_store_global_select_snapshot_run_instruction`
- `lower_pending_store_global_stack_value_publications`
- `lower_store_global_value_publication`

### State Tracking

Keep these wherever dependency shape is least awkward during the first split:

- `prepared_value_home_for_value`
- `value_has_current_block_entry_publication`
- `current_block_entry_publication_register`
- `record_current_block_entry_publication_registers`
- `block_entry_move_clobbers_current_join_publication`
- `prepared_value_home_reads_register_index`
- `value_publication_may_read_register_index`
- `instruction_result_value`
- `instruction_result_value_ref`

If these remain in a central file for the first packet, record that in `todo.md`
as a deliberate follow-up.

## Proof

Each extraction packet must at minimum run:

```bash
cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure
```

Before closing this idea, run:

```bash
ctest --test-dir build -j10 --output-on-failure
```

## Completion Criteria

- `dispatch_publication.cpp` is below 4000 lines.
- No newly extracted publication-related file is over 4000 lines.
- Moves are mechanical and behavior-preserving.
- Any awkward temporary dependency or naming issue is recorded for a later
  reorganization idea.
- Full-suite baseline remains green.

## Closure Note

Closed after Step 5 commit `cc1ebeeb8dd21f95b79bea42272d62befe0895e3`
(`Extract AArch64 publication store sources`) and Step 6 verification.

Final implementation-file line counts confirmed every publication-related
implementation file is under 4000 lines:

- `dispatch_publication.cpp`: 559
- `dispatch_publication_common.cpp`: 303
- `dispatch_producers.cpp`: 390
- `dispatch_value_materialization.cpp`: 1528
- `dispatch_edge_copies.cpp`: 934
- `dispatch_store_sources.cpp`: 1172

Full-suite close proof is recorded in `test_baseline.log` for commit
`cc1ebeeb8dd21f95b79bea42272d62befe0895e3`: 3381/3381 tests passed.
The close-time regression guard was run against that accepted full-suite proof
with non-decreasing passes allowed and reported no new failures.

Follow-up cleanup should be handled as a separate idea if desired:

- Review whether provisional `publication` naming should be replaced now that
  the responsibilities are visible.
- Review whether the extracted clusters should later move to longer-term homes
  such as call, memory, comparison, or producer-specific files.
- Revisit temporary dependency seams noted during execution, including the
  local `find_bir_block` forward declaration in `dispatch_edge_copies.cpp` and
  central state/address helpers intentionally left in
  `dispatch_publication.cpp`.
