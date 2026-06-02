Status: Active
Source Idea Path: ideas/open/92_aarch64_calls_owner_subresponsibility_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Build Calls Owner Inventory

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by building a function-level inventory of
`src/backend/mir/aarch64/codegen/calls.cpp` and `calls.hpp`.

### calls.hpp public surface

- `find_prepared_call_plan`: prepared plan lookup for call instruction index;
  inputs are `BlockLoweringContext` plus instruction index; emits no machine
  record. Used by `dispatch.cpp` call lowering and `globals.cpp`; constrained by
  idea 69 as prepared-call authority consumption.
- `lower_before_call_moves`, `lower_after_call_moves`, `lower_before_return_moves`,
  `lower_value_moves`: public move-bundle lowering entry points; inputs are
  context, phase/call plan/index, diagnostics; emit vectors of call-boundary,
  memory, f128 transport, address/materialization, or inline-asm machine
  instructions. Used by `dispatch.cpp` and targeted tests; constrained by ideas
  69, 84, 91.
- `call_boundary_move_reloads_materialized_address`,
  `order_before_call_moves_for_source_preservation`,
  `call_boundary_move_reloads_prepared_stack_source`,
  `source_register_conflicts_with_materialized_address`: ordering/conflict
  helpers for dispatch around materialized addresses and prepared stack reloads;
  emit no instructions. Used by `dispatch.cpp`; constrained by ideas 69 and 84.
- `lower_scalar_call_argument_producers`: pre-call scalar producer bridge; inputs
  are context, call plan, BIR arguments, scalar state, diagnostics; emits scalar,
  select-chain, and address materialization instructions. Used by `dispatch.cpp`;
  constrained by idea 69 publication-source authority.
- `lower_call_instruction`: main call record entry point; inputs are context,
  `bir::CallInst`, instruction index, diagnostics; emits one direct/indirect or
  variadic call machine instruction. Used by `dispatch.cpp`; constrained by
  ideas 69 and 84.
- `materialize_call_boundary_source_to_destination`,
  `retarget_call_boundary_source_to_emitted_scalar`,
  `record_call_boundary_destination`,
  `record_call_boundary_source_in_destination`,
  `record_call_result_source_register`,
  `retarget_fpr_call_result_store_value_to_emitted_scalar`: dispatch-state
  repair/recording helpers around emitted scalar registers; emit at most one
  select-chain materialization instruction. Used by `dispatch.cpp`; constrained
  by ideas 69 and 84.
- `materialize_indirect_call_callee_to_prepared_register`: indirect-callee
  materialization bridge; inputs are context, call, prepared call plan, index,
  scalar state; emits one select-chain materialization instruction. Used by
  `dispatch.cpp`; constrained by ideas 69 and 84.
- `materialize_missing_frame_slot_call_arguments`,
  `publish_stack_preserved_call_values`: late dispatch bridges for frame-slot
  call arguments and stack-preserved values; emit call-boundary moves or
  select-chain store materializations. Used by `dispatch.cpp`; constrained by
  ideas 69 and 84.

### calls.cpp function inventory

- Prepared lookup/diagnostics: `prepared_named_value_id`,
  `find_prepared_call_plan`, `append_call_diagnostic`,
  `require_prepared_call_plan`. Inputs are prepared name/lookup tables, block
  context, instruction index, diagnostics; emit no instructions, only diagnostic
  entries. These are prepared-authority consumers under idea 69.
- Incoming/variadic frame helpers: `align_to`,
  `incoming_stack_argument_size_bytes`,
  `incoming_stack_argument_alignment_bytes`,
  `entry_param_uses_incoming_stack`, `named_incoming_stack_bytes`,
  `function_has_call`, `fixed_frame_adjustment_bytes`,
  `va_start_overflow_area_stack_offset`. Inputs are ABI params/function/frame
  plans/variadic entry facts; emit no instructions. They feed variadic
  `CallInstructionRecord` fields and remain AArch64 ABI/local frame policy.
- Call-record construction: `make_indirect_callee_register`,
  `make_memory_return_storage`, `call_boundary_move_selection_status`,
  `call_boundary_abi_binding_selection_status`, `call_selection_status`,
  `outgoing_stack_argument_bytes`, `outgoing_stack_argument_base_register`,
  `scalar_integer_register_view_from_size`,
  `make_call_boundary_move_instruction`,
  `make_call_boundary_abi_binding_instruction`, `make_call_instruction`,
  `lower_prepared_call_instruction`. Inputs are prepared call plans, ABI
  bindings, memory return plans, registers, variadic facts; emit
  `InstructionRecord` payloads for `CallBoundaryMove`,
  `CallBoundaryAbiBinding`, `DirectCall`, `IndirectCall`, and variadic call
  opcodes, then wrap them in `MachineInstruction`. Ideas 84/87/91 constrain
  these as target-local ABI, selection-status, and machine-record construction;
  printer validation stays outside this file.
- Register/f128/immediate operand helpers: `register_class_from_bank`,
  `complete_full_width_f128_carrier`, `complete_f128_constant_carrier`,
  `find_prepared_f128_carrier_in_module`,
  `scalar_fp_view_from_register_name`,
  `scalar_view_from_register_name`, `scalar_size_from_register_view`,
  `register_name_with_expected_view`,
  `make_register_operand_from_prepared_authority`,
  `make_f128_q_register_operand_from_carrier`,
  `make_scalar_call_argument_immediate`, `scalar_integer_register_view`,
  `scalar_integer_type_from_size`. Inputs are prepared register placements,
  carriers, BIR types/literals, diagnostics; emit register/immediate operands
  and diagnostics only. Idea 84 explicitly retains this AArch64 ABI conversion
  and diagnostics locally.
- Aggregate byval lane/source helpers: `is_aarch64_byval_register_lane_move`,
  `validate_aggregate_register_lane_publication`,
  `make_byval_register_lane_prepared_source`,
  `selected_byval_lane_extent_bytes`,
  `has_byval_register_lane_transport`. Inputs are prepared moves, call argument
  plans, aggregate transport chunks/lanes, source homes; emit prepared
  `MemoryOperand` sources or diagnostics. Ideas 75/90/91 constrain these:
  shared aggregate transport is consumed here, while AArch64 byval
  classification, selected-source validation, and record construction stay in
  `calls.cpp`.
- Effect/memory/source helpers: `local_effect_from_operand`,
  `call_boundary_frame_slot_direct_offset_is_encodable`,
  `materialize_call_boundary_frame_slot_address_lines`, `stack_copy_address`,
  `make_call_boundary_machine_instruction`,
  `make_selected_frame_slot_source`,
  `make_sret_memory_return_address_source`,
  `make_selected_local_frame_address_source`,
  `make_stack_call_argument_destination`,
  `make_aggregate_call_argument_source`,
  `make_prior_preserved_call_argument_source`, `effect_value_id`,
  `effect_value_name`, `endpoint_has_stack_storage`,
  `make_frame_slot_memory_from_endpoint`,
  `find_prior_stack_preserved_value_before_instruction`,
  `make_prior_stack_preserved_value_source`. Inputs are prepared source
  selections, value homes, endpoints, preserved-value facts, context/index;
  emit memory/register operands, effects, or call-boundary machine wrappers.
  Ideas 69/84 keep prepared-source consumption here without local re-deriving.
- Stack/value/aggregate publication instruction builders:
  `make_outgoing_stack_base_instruction`, `value_move_load_mnemonic`,
  `value_move_store_mnemonic`, `value_move_scratch_register`,
  `value_move_frame_slot_address`, `make_value_stack_move_instruction`,
  `make_aggregate_stack_copy_instruction`,
  `make_byval_register_lane_stack_publication_instruction`. Inputs are prepared
  moves, frame homes, memory operands, chunk widths; emit address
  materialization, memory, or inline-asm machine instructions. Ideas 75/90
  leave stack-lane inline asm and chunk/scratch spelling target-local.
- Immediate cast publication helpers: `find_same_block_cast_producer`,
  `integer_width_bits_for_type`, `immediate_integer_bits`,
  `scalar_fp_register_view` overloads, `scalar_gp_register_view`,
  `append_materialize_fp_immediate`,
  `make_immediate_cast_call_argument_publication_lines`,
  `make_immediate_cast_call_argument_publication_instruction`. Inputs are BIR
  casts/immediates, destination registers, source homes; emit inline-asm
  materialization instructions for supported scalar cast argument publication.
  This is AArch64 concrete instruction spelling/scratch use under ideas 69/84.
- Before-call lowering: `lower_before_call_move`,
  `find_immediate_argument_in_call_plan`,
  `lower_before_call_immediate_binding`,
  `make_callee_saved_preservation_home_population`,
  `make_selected_call_argument_source`, `find_move_bundle`,
  `call_boundary_move_reloads_materialized_address`,
  `move_source_aliases_destination`,
  `order_before_call_moves_for_source_preservation`,
  `lower_before_call_moves`. Inputs are prepared call plan, move bundle,
  boundary effects, ABI bindings, source homes, diagnostics; emit outgoing stack
  base setup, preservation population, call-boundary moves, memory stores,
  f128 transport, aggregate copies, byval lane inline asm, and immediate
  bindings. Used externally by `dispatch.cpp` and tests. Ideas 69/75/84/90/91
  constrain this as the densest prepared-consumer plus AArch64 emission owner.
- After-call/value/return lowering: `lower_after_call_move`,
  `make_callee_saved_preservation_home_republication_instruction`,
  `lower_after_call_moves`, `lower_before_return_moves`, `lower_value_moves`.
  Inputs are prepared after-call/result/value bundles, call plans, homes,
  preservation effects, diagnostics; emit call-boundary moves, memory stores,
  stack moves, and preservation republication instructions. Used by
  `dispatch.cpp` and tests. Ideas 69/84 constrain result/preservation facts to
  prepared authority while keeping register conversion and records local.
- Scalar producer dispatch bridge: `PreparedFrameSlotCallArgumentMove`,
  `LocalAggregateAddressMaterializationResult`,
  `make_dispatch_bridge_machine_instruction`,
  `materialize_local_aggregate_address_lines`,
  `materialize_local_aggregate_address_call_argument`,
  `find_prepared_call_argument_plan`,
  `find_prepared_frame_slot_call_argument_move`,
  `find_prepared_scalar_call_argument_source_producer`,
  `materialize_scalar_call_argument_value`,
  `lower_scalar_call_argument_producers`. Inputs are BIR call arguments,
  prepared source-producer lookups, call argument plans, scalar state; emit
  select-chain/address materialization and scalar ALU instructions before call
  moves. Idea 69 constrains this to consume prepared publication producers.
- Call entry point: `lower_call_instruction`. Inputs are context, `CallInst`,
  instruction index, diagnostics; first delegates dynamic-stack and variadic
  helper paths, then requires a prepared call plan and lowers through
  `lower_prepared_call_instruction`. Emits one call-family machine instruction.
  Used by `dispatch.cpp`; constrained by idea 69 prepared plan authority and
  idea 84 wrapper-contraction limits.
- Call-boundary retarget/state helpers:
  `prepared_call_boundary_source_value`,
  `materialize_call_boundary_source_to_destination`,
  `retarget_call_boundary_source_to_emitted_scalar`,
  `record_call_boundary_destination`,
  `record_call_boundary_source_in_destination`,
  `call_boundary_move_reloads_prepared_stack_source`,
  `source_value_is_materialized_address`,
  `source_register_conflicts_with_materialized_address`. Inputs are existing
  call-boundary instructions, source-producer lookups, materialized-address
  instructions, scalar state; emit at most one select-chain materialization and
  otherwise mutate/record scalar-state facts. Used by `dispatch.cpp`; idea 69
  constrains this to prepared publication facts, not local re-derivation.
- Indirect callee materialization:
  `indirect_callee_materialization_scratch_indices`,
  `scratch_index_is_available`, `choose_scratch_index`,
  `prepared_indirect_callee_source_producer_result`,
  `find_prepared_indirect_callee_source_producer` overloads,
  `find_prepared_indirect_callee_direct_global_select_chain`,
  `find_prepared_indirect_callee_stored_value_source`,
  `emit_indirect_callee_value_to_register_with_csel`,
  `materialize_indirect_call_callee_to_prepared_register`. Inputs are prepared
  indirect callee plan, source-producer/direct-global/stored-value facts,
  scratch candidates, scalar state; emit select-chain materialization to the
  prepared callee register. Used by `dispatch.cpp`; ideas 69/84 keep scratch
  choice and concrete `csel`/materialization spelling local while source facts
  are prepared.
- Result and late publication helpers:
  `record_call_result_source_register`,
  `retarget_fpr_call_result_store_value_to_emitted_scalar`,
  `materialize_missing_frame_slot_call_arguments`,
  `publish_stack_preserved_call_values`. Inputs are call plan, BIR call/store
  insts, scalar state, value homes, preserved-value facts; emit no instruction
  for result recording, retarget FPR store values in-place, or emit
  call-boundary/select-chain materializations for missing frame-slot arguments
  and first stack-preserved values. Used by `dispatch.cpp`; ideas 69/84
  constrain after-call result and preservation republication to consume prepared
  facts.

### Cross-idea constraints captured

- Idea 69: do not re-derive prepared call plans, move bundles, after-call result
  facts, or publication source producers; `calls.cpp` should consume them before
  AArch64-local register rendering and machine-record construction.
- Idea 75: aggregate transport facts for byval register lanes are shared and
  consumed in `make_byval_register_lane_prepared_source`; large indirect byval
  selected-source materialization remains accepted target-local residue.
- Idea 84: helpers that own AArch64 ABI conversion, diagnostics, addressing,
  scratch selection, or machine-record construction are not redundant wrappers.
- Idea 87: call-boundary/printer cleanup requires traced record paths; no broad
  calls/printer rewrite follows from this inventory alone.
- Idea 90: aggregate-lane helper/table contraction preserved the boundary where
  `calls.cpp` constructs byval lane records and printer/instruction helpers own
  final assembly validation/spelling.
- Idea 91: `CallBoundaryMoveInstructionRecord` now has explicit aggregate-lane
  view/schema support, but ordinary moves, immediate publication, f128 facts,
  prepared frame-slot memory, and aggregate-lane publication still share the
  calls-side construction owner.

## Suggested Next

Execute Step 2 by classifying these inventory groups with the runbook labels
and deciding which clusters are intentionally target-local, candidate local
subowners, or need shared-authority evidence before any implementation idea.

## Watchouts

- This route is audit-only; do not edit implementation files.
- The obvious boundaries are not line-count boundaries: before-call lowering,
  aggregate byval lane publication, indirect callee materialization, and
  result/preservation republication all mix prepared-source consumption with
  AArch64 ABI/scratch/record construction.
- Any follow-up idea must avoid reopening ideas 69, 75, 84, 87, 90, or 91
  without new evidence.
- Candidate local subowners should be narrow enough to preserve prepared-source
  authority and not require moving call spelling, ABI register conversion,
  scratch choice, or printer validation out of their current owners.

## Proof

Audit-only packet; no build or tests required by the delegated proof because
only `todo.md` was edited. `git diff --name-only` was run before handoff to
confirm the changed-file set.
