Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_lookup_wrapper_fold_back.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Wrapper Use

# Current Packet

## Just Finished

Step 1 - Inventory Wrapper Use completed the wrapper inventory for
`prepared_named_value_id`, `prepared_value_id`, both `find_value_home` overloads,
`find_same_block_binary_producer`, `find_same_block_named_producer`,
`evaluate_same_block_integer_constant`, `instruction_result_value`, and
`instruction_result_value_ref`.

Method: used `c4c-clang-tool-ccdb list-symbols`, `function-callees`, and
`function-callers` on the in-scope AArch64 codegen translation units first, then
used targeted `rg`/`sed` reads for declarations and cross-TU call-site gaps.

Inventory:

- `prepared_named_value_id(const module::BlockLoweringContext&, const bir::Value&)`
  - Declared in `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp`; defined
    in `dispatch_lookup.cpp`.
  - Owner API: `prepare::resolve_prepared_value_name_id`.
  - Delegation shape: context/precondition adapter; checks prepared names exist
    and the BIR value is non-empty named, then resolves through
    `context.function.prepared->names`.
  - Caller set: `dispatch_producers.cpp`:
    `prepared_source_producer_for_value`,
    `is_current_block_join_parallel_copy_source`,
    `is_current_block_join_parallel_copy_incoming_expression`,
    `build_current_block_join_parallel_copy_cache`; `calls.cpp`:
    `find_prepared_scalar_call_argument_source_producer`,
    `materialize_scalar_call_argument_value`,
    `record_call_result_source_register`,
    `retarget_fpr_call_result_store_value_to_emitted_scalar`;
    `dispatch_lookup.cpp`: `make_named_prepared_result_register`,
    `emitted_scalar_value_available`; `dispatch_value_materialization.cpp`:
    `prepared_same_block_scalar_producer`,
    `emit_value_publication_to_register`; `cast_ops.cpp`:
    `lower_scalar_cast_publication_to_prepared_register`,
    `has_block_entry_stack_edge_consumer_preservation`,
    `lower_scalar_cast_publication_to_prepared_stack`;
    `fp_value_materialization.cpp`: `prepared_same_block_scalar_producer`,
    `lower_scalar_fp_binary_publication_to_prepared_register`; `dispatch.cpp`:
    `instruction_result_has_stack_home`; `dispatch_publication.cpp`:
    `emit_local_slot_address_publication_to_register_impl`,
    `prepared_value_home_for_value`, `current_block_entry_publication_home`,
    `prepared_publication_source_producer_for_value`,
    `lower_missing_conditional_branch_condition_publication`,
    `lower_missing_fused_compare_operand_publication`,
    `retarget_pointer_store_value_to_emitted_scalar`; `globals.cpp`:
    `make_load_global_got_materialization_instruction`; `comparison.cpp`:
    `emitted_register_name`, `prepared_current_block_entry_publication_register`,
    `lower_materialized_compare_condition_branch`,
    `lower_constant_rhs_fused_compare_branch`,
    `lower_conditional_branch_from_emitted_condition`; `memory.cpp`:
    `prepared_or_emitted_store_value_register`,
    `lower_stack_homed_pointer_value_load_publication`,
    `prepared_store_source_producer`,
    `lower_store_local_value_publication`,
    `lower_pointer_base_plus_offset_store_local_publication`,
    `lower_published_store_global_stack_value_publication`,
    `future_store_local_stack_value_publication_covers_instruction`;
    `dispatch_edge_copies.cpp`: `prepared_edge_named_source_producer_context`,
    `emit_edge_value_publication_to_register_impl`,
    `emit_select_chain_value_to_register`,
    `materialize_direct_global_select_chain_call_argument`; `alu.cpp`:
    `lower_scalar_mul_with_distinct_rhs_scratch`.
  - Mechanical status: mechanical context adapter; no AArch64 policy beyond null
    and named-value guards.
  - Intended replacement path: call
    `prepare::resolve_prepared_value_name_id(context.function.prepared->names,
    value.name)` at use sites with the existing prepared/named guards preserved.

- `prepared_value_id(const module::BlockLoweringContext&, ValueNameId)`
  - Declared in `dispatch_lookup.hpp`; defined in `dispatch_lookup.cpp`.
  - Owner API: `prepare::find_indexed_prepared_value_id`.
  - Caller set: none found by AST caller query or text search outside its own
    declaration/definition.
  - Mechanical status: mechanical and currently unused.
  - Intended replacement path: remove the wrapper when no callers remain; if a
    use appears, replace with `prepare::find_indexed_prepared_value_id(
    context.function.value_home_lookups, context.function.regalloc,
    context.function.value_locations, value_name)`.

- `find_value_home(const module::BlockLoweringContext&, PreparedValueId)`
  - Declared in `dispatch_lookup.hpp`; defined in `dispatch_lookup.cpp`.
  - Owner API: `prepare::find_indexed_prepared_value_home` by prepared value id.
  - Caller set: `calls.cpp`: `lower_before_call_move`,
    `lower_after_call_move`, `lower_before_return_moves`, `lower_value_moves`,
    `materialize_local_aggregate_address_call_argument`,
    `record_call_result_source_register`,
    `materialize_missing_frame_slot_call_arguments`,
    `publish_stack_preserved_call_values`; `dispatch_producers.cpp`:
    join parallel-copy source/incoming/cache helpers; `dispatch_publication.cpp`:
    `prepared_value_home_for_value`,
    `lower_missing_fused_compare_operand_publication`,
    `retarget_memory_result_to_prepared_home`; `memory.cpp`:
    `emit_prepared_pointer_value_load_to_register`,
    `lower_stack_homed_pointer_value_load_publication`,
    `plan_pointer_base_plus_offset_store_local_publication`,
    `lower_stack_homed_pointer_store_writeback`,
    `emit_pointer_base_plus_offset_to_register`,
    `lower_published_store_global_stack_value_publication`,
    `future_store_local_stack_value_publication_covers_instruction`;
    `dispatch_edge_copies.cpp`: `lower_predecessor_select_parallel_copy_sources`,
    `materialize_direct_global_select_chain_call_argument`; `comparison.cpp`:
    `prepared_current_block_entry_publication_register`,
    `lower_materialized_compare_condition_branch`,
    `lower_constant_rhs_fused_compare_branch`,
    `lower_conditional_branch_from_emitted_condition`; `alu.cpp`:
    `lower_scalar_mul_with_distinct_rhs_scratch`.
  - Mechanical status: mechanical context adapter.
  - Intended replacement path: direct call to
    `prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
    context.function.value_locations, value_id)`.

- `find_value_home(const module::BlockLoweringContext&, ValueNameId)`
  - Declared in `dispatch_lookup.hpp`; defined in `dispatch_lookup.cpp`.
  - Owner API: `prepare::find_indexed_prepared_value_home` by value name, via
    indexed value-id lookup.
  - Caller set: same symbol-level caller inventory as `find_value_home` above;
    text search shows both overloads are used across value-id and value-name
    call sites.
  - Mechanical status: mechanical context adapter.
  - Intended replacement path: direct call to
    `prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
    context.function.regalloc, context.function.value_locations, value_name)`.

- `find_same_block_binary_producer(const module::BlockLoweringContext&, const bir::Value&)`
  - Declared in `dispatch_producers.hpp`; defined in `dispatch_producers.cpp`.
  - Owner API: `mir::find_same_block_binary_producer`.
  - Delegation shape: returns only `.binary` from the owner record, dropping the
    instruction index.
  - Caller set: no external callers found; AST reports only the wrapper
    definition in `dispatch_producers.cpp`.
  - Mechanical status: mechanical and apparently unused.
  - Intended replacement path: remove if still unused; otherwise replace with
    `mir::find_same_block_binary_producer(context.bir_block, value).binary`
    where the caller only needs the pointer.

- `find_same_block_named_producer(const module::BlockLoweringContext&, string_view, size_t)`
  - Declared in `dispatch_producers.hpp`; defined in `dispatch_producers.cpp`.
  - Owner API: `mir::find_same_block_named_producer`.
  - Caller set: `dispatch_edge_copies.cpp`: `find_edge_named_producer`;
    `fp_value_materialization.cpp`: `emit_fp_value_to_register`;
    `dispatch.cpp`: assigned into the comparison callback table used through
    `comparison.hpp`'s function pointer.
  - Mechanical status: wrapper body is mechanical, but the comparison callback
    path is not a pure text substitution because the callback signature accepts
    `BlockLoweringContext`.
  - Intended replacement path: direct owner call for ordinary use sites; for the
    comparison callback path, either keep a tiny adapter until the callback
    contract changes or route the call through a caller-local adapter that calls
    `mir::find_same_block_named_producer(context.bir_block, ...)`.

- `evaluate_same_block_integer_constant(const module::BlockLoweringContext&, const bir::Value&, unsigned)`
  - Declared in `dispatch_producers.hpp`; defined in `dispatch_producers.cpp`.
  - Owner API: `mir::evaluate_same_block_integer_constant`.
  - Delegation shape: unwraps the owner `SameBlockIntegerConstant` record and
    returns only `.value`, dropping `.depth`.
  - Caller set: `dispatch_value_materialization.cpp`: `value_power_of_two_shift`,
    `value_publication_may_write_scratch_register`,
    `emit_value_publication_to_register`; `comparison.cpp`:
    `is_fused_compare_branch_support_instruction`.
  - Mechanical status: mechanical adapter where callers only need the integer.
  - Intended replacement path: call
    `mir::evaluate_same_block_integer_constant(context.bir_block, value, depth)`
    and map the optional record to `record->value`.

- `instruction_result_value(const bir::Inst&)`
  - Declared in `dispatch_publication.hpp`; defined in `dispatch_publication.cpp`.
  - Owner API: none suitable found. It delegates only to local
    `instruction_result_value_ref` and copies the referenced value into an
    optional.
  - Caller set: `dispatch_producers.cpp`:
    `is_current_block_join_parallel_copy_source`,
    `is_current_block_join_parallel_copy_incoming_expression`; `dispatch.cpp`:
    `instruction_result_has_stack_home`; `cast_ops.cpp`:
    `lower_scalar_cast_publication_to_prepared_register`,
    `lower_scalar_cast_publication_to_prepared_stack`; `memory.cpp`:
    `lower_pending_store_global_stack_value_publications`,
    `lower_published_store_global_stack_value_publication`,
    `future_store_local_stack_value_publication_covers_instruction`.
  - Mechanical status: excluded from mechanical fold-back for now because the
    result-kind policy is local to AArch64 publication code and includes
    optional `CallInst` results.
  - Intended replacement path: defer until there is a shared owner API for BIR
    instruction result extraction with the same call-result semantics.

- `instruction_result_value_ref(const bir::Inst&)`
  - Declared in `dispatch_publication.hpp`; defined in `dispatch_publication.cpp`.
  - Owner API: none suitable found. Similar local helper
    `prepare::prepared_instruction_result_value_ref` exists in
    `prealloc/prepared_lookups.cpp`, but it is private there and does not cover
    `bir::CallInst` optional results.
  - Caller set: `dispatch_publication.cpp`: `instruction_result_value`;
    `dispatch_producers.cpp`: `find_same_block_result_index`,
    `build_current_block_join_parallel_copy_cache`.
  - Mechanical status: excluded from mechanical fold-back because it still
    carries instruction-result policy that is not currently owned by a shared
    API.
  - Intended replacement path: defer; a later shared result-extraction owner
    would need to preserve the current Binary/Cast/Select/LoadLocal/LoadGlobal
    and optional Call result behavior.

## Suggested Next

Start Step 2 with the purely mechanical no-caller removals or direct owner-API
fold-backs, leaving `instruction_result_value` and `instruction_result_value_ref`
out of scope until shared result-extraction ownership exists.

## Watchouts

`instruction_result_value` and `instruction_result_value_ref` are intentionally
excluded from mechanical replacement because the only similar helper found is
private prealloc code and lacks `CallInst` result handling. The
`find_same_block_named_producer` body is mechanical, but the comparison
function-pointer use needs adapter/signature care instead of a blind
substitution.

## Proof

Inventory-only packet. No build or test was required and no `test_after.log`
was written. AST-backed inspection used `c4c-clang-tool-ccdb`; targeted text
search was used only after the AST queries to complete cross-translation-unit
caller inventory.
