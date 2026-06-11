Status: Active
Source Idea Path: ideas/open/195_cross_target_route_view_reuse_beyond_x86_route6.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory target wrapper consumers

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: inventoried x86 and riscv wrapper consumers of
prepared route facts, prepared compatibility helpers, and selected route-view
adapters.

### x86 Inventory

Route family / semantic fact: consumed function-level prepared banks.

- Boundary: `src/backend/mir/x86/x86.hpp` `ConsumedPlans` / `consume_plans`.
  Consumes `PreparedFramePlanFunction`, `PreparedDynamicStackPlanFunction`,
  `PreparedControlFlowFunction`, `PreparedCallPlansFunction`,
  `PreparedRegallocFunction`, `PreparedStoragePlanFunction`,
  `PreparedFunctionLookups`, and `Route6CallUseSourceIndex`.
  Ownership: prepared facts plus one selected route view.
- Boundary: `ConsumedPlans::shared_function_lookups` and
  `ConsumedPlans::shared_call_plan_lookups`.
  Consumes `PreparedFunctionLookups` and `PreparedCallPlanLookups` created by
  `make_prepared_function_lookups`.
  Ownership: compatibility adapter over prepared lookup indexes; route-view
  ownership unknown.

Route family / semantic fact: call wrapper plans and Route 6 call-use source.

- Boundary: `find_consumed_call_plan`,
  `find_consumed_call_argument_plan`, and
  `find_consumed_call_result_plan` in `x86.hpp`.
  Consumes `find_indexed_prepared_call_plan`,
  `PreparedCallPlanLookups`, `PreparedCallPlan`,
  `PreparedCallArgumentPlan`, and `PreparedCallResultPlan`.
  Ownership: prepared facts through an x86 compatibility adapter.
- Boundary: `find_consumed_scalar_i32_call_argument_source` in `x86.hpp`.
  Consumes `route6_find_call_argument_source` from
  `Route6CallUseSourceIndex`, cross-checked against
  `PreparedCallArgumentPlan::source_value_id`.
  Ownership: selected Route 6 route-view adapter guarded by prepared fact
  agreement.
- Boundary: `append_prepared_direct_extern_call_argument`,
  `append_prepared_direct_extern_call_return_function`, and the same-module
  call wrapper path in `src/backend/mir/x86/module/module.cpp`.
  Consumes `find_consumed_call_plan`, `find_consumed_scalar_i32_call_argument_source`,
  `PreparedCallWrapperKind`, `PreparedMoveBundle`, call argument/result ABI
  moves, and `PreparedValueHome` facts.
  Ownership: prepared facts plus Route 6 for scalar i32 argument-source
  substitution.
- Boundary: `src/backend/mir/x86/codegen/x86_codegen.hpp`
  `select_prepared_call_argument_abi_register_if_supported` and
  `select_prepared_call_argument_abi_stack_offset_if_supported`.
  Consumes `PreparedValueLocationFunction`, `find_prepared_move_bundle`,
  `PreparedMoveResolution`, and `PreparedAbiBinding`.
  Ownership: prepared facts; route-view ownership unknown.

Route family / semantic fact: edge/block publication and parallel-copy moves.

- Boundary: `src/backend/mir/x86/prepared/dispatch.cpp`
  `consume_edge_publication_move_intent` and
  `append_edge_publication_move_instruction`.
  Consumes `ConsumedPlans::shared_function_lookups`,
  `find_unique_indexed_prepared_edge_publication`,
  `PreparedEdgePublication`, `PreparedValueHome`, and
  `PreparedMoveResolution`.
  Ownership: prepared facts through the x86 `ConsumedPlans` adapter; no
  route-view consumer found.
- Boundary: `src/backend/mir/x86/module/module.cpp`
  `append_prepared_compare_join_parallel_copy` and related compare/join,
  countdown, and short-circuit helpers.
  Consumes `PreparedControlFlowFunction`,
  `find_prepared_parallel_copy_bundle`,
  `find_prepared_out_of_ssa_parallel_copy_move_bundle`,
  `find_prepared_parallel_copy_move_for_step`,
  `find_prepared_out_of_ssa_parallel_copy_move_for_step`,
  `find_prepared_branch_condition`, and compare/short-circuit control-flow
  helpers.
  Ownership: prepared facts; route-view ownership unknown.
- Boundary: `x86::prepared::Query` in
  `src/backend/mir/x86/prepared/prepared.hpp`.
  Consumes direct query helpers for addressing, value locations, call plans,
  regalloc, storage, BIR function lookup, decoded home storage, call-boundary
  classification, formal publication, and block-entry publication collection.
  Ownership: compatibility adapter over prepared facts; route-view ownership
  unknown.

Route family / semantic fact: value home, storage, frame, and local-slot
memory.

- Boundary: `append_grouped_authority_comments` in
  `src/backend/mir/x86/module/module.cpp`.
  Consumes `ConsumedPlans` frame, call, regalloc, and storage facts for grouped
  authority comments.
  Ownership: prepared facts only.
- Boundary: `require_prepared_value_location_function`,
  `require_prepared_i32_return_move`,
  `require_prepared_i32_instruction_move`,
  `require_prepared_i32_value_home`, and
  `require_prepared_i32_register_home` in `module.cpp`.
  Consumes `PreparedValueLocationFunction`, `find_prepared_move_bundle`, and
  `find_prepared_value_home`.
  Ownership: prepared facts; route-view ownership unknown.
- Boundary: local-slot and compare-load helpers in `module.cpp`, including
  `render_prepared_local_slot_statement_memory_operand`,
  `find_prepared_local_slot_compare_load`,
  and `find_prepared_local_slot_compare_instruction_index`.
  Consumes `PreparedAddressingFunction`, `find_prepared_memory_access`,
  `PreparedMemoryAccess`, `PreparedFrameSlot`, and value-home facts.
  Ownership: prepared facts; route-view ownership unknown.

### riscv Inventory

Route family / semantic fact: prepared edge publication.

- Boundary: `src/backend/mir/riscv/codegen/emit.cpp`
  `emit_prepared_module`.
  Consumes `PreparedBirModule::control_flow.functions`,
  `make_prepared_function_lookups`, `PreparedFunctionLookups`, and
  `PreparedEdgePublicationLookups`.
  Ownership: prepared facts; no route-view consumer found.
- Boundary: `consume_edge_publication_move_intent` and
  `append_edge_publication_move_instruction` in
  `src/backend/mir/riscv/codegen/emit.cpp` / `emit.hpp`.
  Consumes `PreparedFunctionLookups`,
  `find_unique_indexed_prepared_edge_publication`,
  `PreparedEdgePublication`, `PreparedValueHome`, and
  `PreparedMoveResolution`.
  Ownership: prepared facts through a riscv-local prepared lookup adapter; no
  route-view consumer found.
- Boundary: `render_edge_publication_source_operand` in `emit.cpp`.
  Consumes `PreparedValueHome` kinds for register, rematerializable immediate,
  stack slot, pointer-base-plus-offset, and load-local memory-source
  publication metadata; also consumes `lookups->value_homes.value_ids` and
  `lookups->value_homes.homes_by_id`.
  Ownership: prepared facts; route-view ownership unknown.
- Boundary: `prepare_same_width_i32_stack_source_publication` use inside
  riscv `consume_edge_publication_move_intent`.
  Consumes typed stack-source publication status, source/destination type,
  source stack slot/offset/size/alignment, extension policy, destination
  register bank, and destination register placement.
  Ownership: prepared compatibility helper; no route-view consumer found.

Route family / semantic fact: target-local register and stack policy.

- Boundary: `riscv_gpr_register_name_from_placement`.
  Consumes `PreparedRegisterPlacement` bank, pool, slot, and contiguous width.
  Ownership: prepared facts plus riscv target-local register mapping.
- Boundary: stack-destination policy helpers
  `has_direct_register_source_for_stack_destination`,
  `has_rematerializable_i32_source_for_stack_destination`,
  `has_non_aliasing_i32_stack_source_for_stack_destination`, and
  `has_existing_concrete_i64_stack_source_register_policy`.
  Consumes `EdgePublicationMoveIntent`, `PreparedValueHome`, and
  `PreparedEdgePublication` type/source metadata.
  Ownership: prepared facts plus riscv target-local scratch/offset policy;
  route-view ownership unknown.
- Boundary: `emit_prepared_lir_module`.
  Consumes direct `LirModule` shape, not prepared facts or route views.
  Ownership: outside Step 1 route/prepared wrapper inventory.

## Suggested Next

Start Step 2 by classifying which inventoried boundaries could reuse an
AArch64-proven semantic route view. Keep the initial candidates narrow:
compare x86 Route 6 call-argument source reuse against the riscv edge-publication
prepared lookup path, then decide whether riscv needs a new route view or must
remain target-local for now.

## Watchouts

- Do not treat x86 Route 6 `ConsumedPlans` as broad cross-target readiness.
- Do not migrate ABI, frame, register, wrapper, formatting, instruction
  selection, or emission policy into shared route facts.
- Do not weaken test or oracle expectations to make a boundary appear ready.
- x86 has both `ConsumedPlans` and direct prepared-query surfaces; Step 2 should
  avoid assuming `ConsumedPlans` covers every x86 wrapper boundary.
- riscv currently exposes prepared edge-publication consumers but no discovered
  Route 6 or other route-view adapter in the riscv wrapper code.
- Several x86 module-local control-flow, memory, and local-slot helpers consume
  prepared facts directly; their route-view ownership remains `unknown`.

## Proof

Lifecycle/docs-only packet. Used targeted `rg` plus `c4c-clang-tool` /
`c4c-clang-tool-ccdb` symbol and signature queries over x86/riscv wrapper
code; no build or test proof was required, and no `test_after.log` was
generated for this docs-only inventory.
