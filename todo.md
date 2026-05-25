Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Dispatch Responsibilities

# Current Packet

## Just Finished

Step 1 - Map Dispatch Responsibilities completed the current AArch64 dispatch
responsibility map and selected the first small extraction target.

Responsibility map:

- Traversal/routing: `make_block_lowering_context` maps prepared blocks back to
  retained BIR blocks, and `dispatch_prepared_block` owns block setup, entry
  publication seeding, BIR instruction iteration, per-family routing, result
  accounting, terminator routing, and successor publication.
- Materialization currently embedded in dispatch: `make_load_global_got_materialization_instruction`
  constructs GOT address/value materialization for `LoadGlobalInst`;
  `lower_store_local_with_address_materialization` and
  `lower_scalar_with_address_materialization` orchestrate address
  materializations around memory/scalar lowering; `dispatch_value_materialization.*`
  already owns most value/home reload emission and prepared-register/stack
  publication materialization.
- Publication currently embedded in dispatch: `record_address_materialization_result`,
  `record_memory_result`, `before_return_publication_already_emitted`,
  `record_before_return_publication`, `retarget_memory_result_to_prepared_home`,
  `retarget_pointer_store_value_to_materialized_address`,
  `retarget_store_address_to_materialized_pointer`,
  `retarget_pointer_store_value_to_emitted_scalar`,
  `retarget_store_local_value_to_emitted_scalar`,
  `retarget_fpr_call_result_store_value_to_emitted_scalar`,
  and `lower_fixed_formal_store_local_publication`; `dispatch_publication.*`
  already owns value-home, frame-slot, entry-publication, and register-alias
  helpers.
- Edge copies / join values: `dispatch.cpp` still owns the current-block join
  parallel-copy cache (`named_operands_of_instruction`,
  `is_join_parallel_copy_expression_instruction`,
  `find_same_block_result_index`, `same_block_result_depends_on_value`,
  `is_current_block_join_parallel_copy_incoming_expression`,
  `CurrentBlockJoinParallelCopyCache`,
  `build_current_block_join_parallel_copy_cache`, and cached query wrappers)
  plus `lower_predecessor_select_parallel_copy_sources`; `dispatch_edge_copies.*`
  already owns predecessor/successor edge producer lookup, edge value
  publication, select-chain materialization, and direct-global select-chain
  call-argument materialization.
- Same-block producer lookup: `dispatch_lookup.*` owns prepared name/home lookup
  plus same-block scalar/load-local producer queries; `dispatch_producers.*`
  owns same-block binary/select/named producer lookup, producer instruction
  indexes, integer-constant evaluation, global-load labels, and a second
  `is_current_block_join_parallel_copy_source` surface. `dispatch.cpp` still
  has a local same-block result dependency walk for join-copy cache decisions.
- Diagnostics: `dispatch_diagnostics.*` owns block, unsupported-instruction,
  unsupported-terminator, and call diagnostics; `dispatch_prepared_block` still
  decides when to append missing block mapping and unsupported terminator or
  instruction diagnostics.
- Call-specific glue: `dispatch_prepared_block` still coordinates dynamic-stack,
  inline-asm, intrinsic, variadic-helper, prepared call-plan, before/after-call
  move, indirect-callee, call-result source, and call-clobber routing through
  `calls*`, `calls_dispatch_bridge.*`, `memory_dynamic_stack.*`, `inline_asm.*`,
  `intrinsics.*`, and `variadic.*`. Its local lambdas still retarget/record
  call-boundary move sources and destinations and order materialized addresses
  against before-call moves.

Blocked/deferred candidates:

- Do not extract or rewrite `lower_scalar_call_argument_producers`,
  `lower_before_call_moves`, `materialize_call_boundary_source_to_destination`,
  `materialize_missing_frame_slot_call_arguments`, or dispatch's local
  before-call source-retarget/defer logic yet. Those areas can depend on the
  missing shared prepared call-argument source fact from
  `ideas/open/02_aarch64_calls_emission_consolidation.md`: whether a selected
  `BeforeCall` argument move should source from prior preservation,
  local-frame address materialization, or byval register-lane materialization.

## Suggested Next

Execute Step 2 by extracting
`make_load_global_got_materialization_instruction` from `dispatch.cpp` into the
value-materialization owner as a narrowly named `LoadGlobal` GOT materializer.
Keep `dispatch_prepared_block` responsible only for recognizing `LoadGlobalInst`
and routing to the owner.

Suggested owned files for the code-changing packet:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `todo.md`

Suggested focused proof command:

```bash
cmake --build build --target c4c_tests && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call' --output-on-failure
```

## Watchouts

- The selected target is non-call materialization and should not change
  `calls*`, `calls_dispatch_bridge.*`, or any prepared call-argument source
  selection.
- Preserve `record_emitted_scalar_register` behavior for GOT loads that produce
  a register and the stack-home path that returns a select-chain
  materialization instruction.
- `make_load_global_got_materialization_instruction` currently depends on
  helpers from `dispatch_publication.*`, `dispatch_lookup.*`,
  `dispatch_producers.*`, `dispatch_edge_copies.*`, `globals.*`, `abi`, and
  `machine_printer`; expect the extraction to add the minimal includes or
  declarations needed by `dispatch_value_materialization.*`.

## Proof

Mapping-only packet; no build or tests run and no `test_after.log` produced.
Focused proof command selected for the first code-changing packet:

```bash
cmake --build build --target c4c_tests && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call' --output-on-failure
```
