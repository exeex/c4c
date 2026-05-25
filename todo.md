Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Separate Publication And Producer Lookup

# Current Packet

## Just Finished

Step 3 - Separate Publication And Producer Lookup mapped the remaining
publication and same-block producer lookup responsibilities embedded in
`dispatch.cpp` after Step 2 value-materialization extraction.

Publication responsibilities still embedded in `dispatch.cpp`:

- Scalar-state publication recording: `record_address_materialization_result`,
  `record_memory_result`, `before_return_publication_already_emitted`, and
  `record_before_return_publication`.
- Memory/store publication retargeting: `retarget_memory_result_to_prepared_home`,
  `retarget_pointer_store_value_to_materialized_address`,
  `retarget_store_address_to_materialized_pointer`,
  `retarget_pointer_store_value_to_emitted_scalar`,
  `retarget_store_local_value_to_emitted_scalar`, and
  `retarget_fpr_call_result_store_value_to_emitted_scalar`.
- Store/formal publication lowering:
  `lower_fixed_formal_store_local_publication` and the dispatch-side
  orchestration around `lower_store_local_value_publication`,
  `lower_store_global_value_publication`,
  `lower_pending_store_global_stack_value_publications`, and
  pointer-base-plus-offset store publication.
- Branch/compare publication:
  `lower_missing_conditional_branch_condition_publication`,
  `lower_missing_fused_compare_operand_publication`, and
  `lower_missing_fused_compare_operand_publications`.

Same-block producer lookup responsibilities still embedded in `dispatch.cpp`:

- Current-block join parallel-copy producer/cache logic:
  `named_operands_of_instruction`,
  `is_join_parallel_copy_expression_instruction`,
  `find_same_block_result_index`, `same_block_result_depends_on_value`,
  `is_current_block_join_parallel_copy_incoming_expression`,
  `CurrentBlockJoinParallelCopyCache`,
  `build_current_block_join_parallel_copy_cache`,
  `cached_current_block_join_parallel_copy_incoming_expression`, and
  `cached_current_block_join_parallel_copy_source`.
- Terminator publication producer lookup: the missing conditional-branch and
  fused-compare helpers still call `find_same_block_named_producer` and
  `producer_instruction_index` locally even though `dispatch_producers.*` and
  `dispatch_lookup.*` already own general same-block producer surfaces.

## Suggested Next

Execute the first small non-call Step 3 target: move
`record_address_materialization_result` and `record_memory_result` from
`dispatch.cpp` into `dispatch_publication.*` as scalar-state publication
recording helpers.

Suggested owned files for the code-changing packet:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `todo.md`
- `test_after.log`

Suggested focused proof command:

```bash
cmake --build --preset default && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records' --output-on-failure > test_after.log 2>&1
```

## Watchouts

- Keep the first code-changing Step 3 packet limited to scalar-state recording
  for `AddressMaterializationRecord` and `MemoryInstructionRecord`; do not
  move address-materialization orchestration itself.
- Do not touch `calls*`, `calls_dispatch_bridge.*`, before/after-call source
  retargeting, `lower_scalar_call_argument_producers`,
  `materialize_call_boundary_source_to_destination`, or
  `materialize_missing_frame_slot_call_arguments`.
- Leave current-block join producer/cache extraction for a later producer
  lookup packet; that cluster is larger than the first small publication
  recording target.
- Branch/compare missing-publication helpers mix publication with producer
  lookup and scalar control lowering, so they should not be the first small
  extraction.

## Proof

Mapping-only packet; no build/tests run and no proof logs touched.
