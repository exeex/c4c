# 386 AArch64 Calls Mechanical Split

## Context

`src/backend/mir/aarch64/codegen/calls.cpp` is currently over 7000 lines. The
public header has a reasonable call-oriented surface, but the implementation file
contains many distinct private clusters:

- prepared call plan lookup
- argument/result binding lookup
- register/effect helpers
- aggregate/byval lane materialization
- frame-slot and local-frame call argument source construction
- prior preserved-value analysis
- before-call, after-call, and before-return move lowering
- prepared call instruction lowering
- call-boundary instruction construction
- target printer helpers for call records

This idea mirrors the dispatch split work: first make the implementation shape
visible through mechanical extraction, then decide which files should be renamed,
merged, or moved.

## Goal

Split `calls.cpp` into smaller implementation files while preserving behavior.
The immediate target is to keep every calls-related implementation file below
4000 lines and make the call lowering responsibilities reviewable.

## Non-Goals

- Do not redesign AArch64 call lowering.
- Do not change generated assembly or MIR records.
- Do not rename the public call API unless mechanically required.
- Do not merge call-boundary material back into dispatch files in this pass.
- Do not add backend capability or testcase-shaped special cases.

## Required Tooling

Executor packets for this idea should use `.codex/skills/c4c-clang-tools`.
`calls.cpp` is too large for line-range guessing.

Before moving each cluster, inspect function signatures plus at least one direct
caller or callee query:

```bash
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp <function-name> build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp <function-name> build/compile_commands.json
```

Record any awkward dependency that prevents a clean mechanical move.

## Suggested First-Pass Split

Use provisional names. A later cleanup idea can rename or merge these files.

### `calls_common.cpp/hpp`

Low-level helpers shared by the call lowering clusters:

- `align_to`
- `incoming_stack_argument_size_bytes`
- `incoming_stack_argument_alignment_bytes`
- `outgoing_stack_argument_size_bytes`
- `outgoing_stack_argument_bytes`
- `outgoing_stack_argument_base_register`
- `entry_param_uses_incoming_stack`
- `named_incoming_stack_bytes`
- `function_has_call`
- `fixed_frame_adjustment_bytes`
- `va_start_overflow_area_stack_offset`
- `register_class_from_bank`
- `register_display_name`
- `occupied_register_views`
- `prepared_clobber_expected_view`
- `append_call_diagnostic`

### `calls_argument_sources.cpp/hpp`

Argument source construction and prepared argument/result binding lookup:

- `find_prepared_argument_plan`
- `find_prepared_argument_binding`
- `find_prepared_result_binding`
- `make_register_operand_from_prepared_authority`
- `make_scalar_call_argument_immediate`
- `scalar_integer_register_view`
- `scalar_fp_register_view`
- `scalar_integer_register_view_from_size`
- `scalar_integer_type_from_size`
- `find_frame_slot_by_id`
- `make_frame_slot_call_argument_source`
- `make_sret_memory_return_address_source`
- `call_argument_call`
- `call_argument_is_pointer`
- `call_argument_is_byval_copy`
- `call_argument_allows_local_frame_address_publication`
- `make_frame_slot_call_argument_address_source`
- `local_frame_address_name_matches`
- `make_local_frame_address_call_argument_source`
- `make_stack_call_argument_destination`
- `make_aggregate_call_argument_source`
- `make_indirect_callee_register`
- `make_memory_return_storage`

### `calls_byval_aggregates.cpp/hpp`

Aggregate/byval lane copy and stack/register lane helpers:

- `stack_copy_address`
- `aggregate_stack_copy_load_mnemonic`
- `aggregate_stack_copy_store_mnemonic`
- `aggregate_stack_copy_scratch`
- `aggregate_stack_copy_chunks`
- `aggregate_register_lane_load_mnemonic`
- `aggregate_register_lane_load_register`
- `aggregate_register_lane_scratch`
- `aggregate_register_lane_memory`
- `aggregate_register_lane_memory_is_printable`
- `aggregate_register_lane_printable_chunk`
- `aggregate_register_lane_destination`
- `is_aggregate_register_lane_publication`
- `byval_register_lane_size_bytes`
- `aggregate_slot_source_byte_offset`
- `find_aggregate_lane_load_source_slot`
- `append_aggregate_lane_partial_source_stores`
- `collect_byval_register_lane_stores`
- `make_byval_register_lane_prepared_source`
- `aggregate_lane_store_memory`
- `aarch64_indirect_byval_argument_size_bytes`
- `aarch64_stack_byval_argument_size_bytes`
- `aarch64_register_byval_argument_size_bytes`
- `aarch64_indirect_register_byval_argument`
- `fragmented_aggregate_register_lane_publication_lines`
- `make_fragmented_aggregate_register_lane_publication_instruction`
- `make_aggregate_stack_copy_instruction`
- `make_byval_register_lane_stack_publication_instruction`
- `make_fragmented_byval_register_lane_stack_publication_instruction`

### `calls_preservation.cpp/hpp`

Preserved-value and move-bundle analysis:

- `prepared_block_index_by_label`
- `prepared_block_successors`
- `prepared_block_dominates`
- `move_bundle_position_key`
- `find_move_bundle`
- `find_value_home`
- `prepared_value_id`
- `prior_preserved_entry_position_less`
- `find_latest_prior_preserved_value_by_position`
- `find_prior_preserved_value_by_dominating_position`
- `find_prior_preserved_value_for_call_argument`
- `find_prior_preserved_value_for_value`
- `find_prior_stack_preserved_value_before_instruction`
- `value_spelling_matches`
- `non_call_instruction_uses_value`
- `terminator_uses_value`
- `branch_condition_uses_value`
- `preserved_value_has_later_non_call_use`
- `preserved_value_has_block_entry_non_call_use`
- `make_prior_preserved_call_argument_source`
- `make_callee_saved_preservation_home_republication_instruction`
- `make_callee_saved_preservation_home_republication`
- `make_callee_saved_preservation_home_population`

### `calls_moves.cpp/hpp`

Move lowering and value-move instruction construction:

- `make_call_boundary_machine_instruction`
- `make_outgoing_stack_base_instruction`
- `value_move_load_mnemonic`
- `value_move_store_mnemonic`
- `value_move_scratch_register`
- `value_move_frame_slot_address`
- `make_value_stack_move_instruction`
- `lower_before_call_move`
- `lower_before_call_immediate_binding`
- `lower_after_call_move`
- `lower_before_call_moves`
- `lower_after_call_moves`
- `lower_before_return_moves`
- `lower_value_moves`

### `calls_effects.cpp/hpp`

Machine effect resource derivation:

- `effect_from_prepared_call_clobber`
- `effects_from_prepared_call_clobbers`
- `effect_from_prepared_call_preserved_value`
- `effects_from_prepared_call_preserved_values`
- `effect_from_operand`
- `prepared_value_def`
- `effects_from_operands`

### `calls_printing.cpp/hpp`

Call record construction and printing helpers:

- `call_boundary_move_selection_status`
- `call_boundary_abi_binding_selection_status`
- `call_selection_status`
- `target_unsupported`
- `target_printed`
- `bad_header`
- `required_primary_mnemonic`
- `register_name`
- `same_gp_register_index`
- `call_boundary_load_width_bytes`
- `call_boundary_frame_slot_direct_offset_is_encodable`
- `call_boundary_address_scratch_register`
- `materialize_call_boundary_frame_slot_address_lines`
- `print_aggregate_register_lane_publication_lines`
- `print_call_boundary_frame_slot_load_lines`
- `f128_call_boundary_vector_register_name`
- `scalar_integer_width_bits`
- `scalar_integer_immediate_bits`
- `is_single_move_wide_immediate`
- `find_same_block_cast_producer`
- `integer_width_bits_for_type`
- `immediate_integer_bits`
- `scalar_fp_register_view`
- `scalar_gp_register_view`
- `append_materialize_fp_immediate`
- `make_immediate_cast_call_argument_publication_lines`
- `make_immediate_cast_call_argument_publication_instruction`
- `make_call_boundary_move_instruction`
- `make_call_boundary_abi_binding_instruction`
- `make_call_instruction`
- `print_call`
- `print_call_boundary_move`
- `print_call_boundary_abi_binding`

### Keep in `calls.cpp` Initially

Keep these high-level entrypoints in `calls.cpp` until lower-level clusters have
been extracted and proven:

- `publish_prepared_call_preserve_effects`
- `find_prepared_call_plan`
- `require_prepared_call_plan`
- `lower_prepared_call_instruction`
- `ScopedPreparedCallPreserveEffectPublication`

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

- `calls.cpp` is below 4000 lines.
- No newly extracted calls-related implementation file is over 4000 lines.
- Public `calls.hpp` remains coherent and call-oriented.
- Moves are mechanical and behavior-preserving.
- Any awkward temporary dependency or naming problem is recorded for a later
  cleanup/reorganization idea.
- Full-suite baseline remains green.

## Closure Note

Closed on 2026-05-23 after the mechanical split reached the size target:
`calls.cpp` is 200 lines and the largest extracted calls implementation file is
`calls_moves.cpp` at 3397 lines.

The repeated narrow proof for the split was green. The full close proof still
reported failures in `c_testsuite_aarch64_backend_src_00176_c`,
`c_testsuite_aarch64_backend_src_00182_c`, and
`c_testsuite_aarch64_backend_src_00205_c`, but supervisor baseline evidence
showed the same three runtime mismatches at pre-split commit `84af66ebf` using
the current c-testsuite contents. Those failures are therefore treated as
pre-existing baseline failures, not regressions from this behavior-preserving
mechanical split.
