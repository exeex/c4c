Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Separate Publication And Producer Lookup

# Current Packet

## Just Finished

Step 3 - Separate Publication And Producer Lookup inspected the remaining
publication and same-block producer lookup candidates after moving
memory-result publication retargeting.

Remaining Step 3 candidates in `dispatch.cpp`:

- Pointer-store retargeting after address materialization:
  `retarget_pointer_store_value_to_materialized_address` and
  `retarget_store_address_to_materialized_pointer`.
- Store-value retargeting after ordinary memory lowering:
  `retarget_pointer_store_value_to_emitted_scalar`,
  `retarget_store_local_value_to_emitted_scalar`, and
  `retarget_fpr_call_result_store_value_to_emitted_scalar`.
- Before-return publication recording:
  `before_return_publication_already_emitted` and
  `record_before_return_publication`.
- Branch/compare missing publication:
  `lower_missing_conditional_branch_condition_publication`,
  `lower_missing_fused_compare_operand_publication`, and
  `lower_missing_fused_compare_operand_publications`.
- Same-block producer lookup/cache:
  `named_operands_of_instruction`,
  `is_join_parallel_copy_expression_instruction`,
  `find_same_block_result_index`, `same_block_result_depends_on_value`,
  `is_current_block_join_parallel_copy_incoming_expression`,
  `CurrentBlockJoinParallelCopyCache`,
  `build_current_block_join_parallel_copy_cache`, and cached query wrappers.

## Suggested Next

Execute the next small non-call Step 3 publication target: move
`retarget_pointer_store_value_to_materialized_address` and
`retarget_store_address_to_materialized_pointer` from `dispatch.cpp` into
`dispatch_publication.*`.

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

Keep the packet limited to the two pointer-store retargeting helpers. Do not
move `lower_store_local_with_address_materialization`, address-materialization
lowering/index logic, or fallback memory-lowering orchestration.

Do not touch `calls*`, `calls_dispatch_bridge.*`, before/after-call source
retargeting, `lower_scalar_call_argument_producers`,
`materialize_call_boundary_source_to_destination`, or
`materialize_missing_frame_slot_call_arguments`.

Leave the store-value emitted-scalar retargeting helpers, before-return
publication recording, branch/compare missing-publication helpers, and the
current-block join producer/cache cluster for later packets.

## Proof

Mapping-only packet; no build/tests run and no proof logs touched.
