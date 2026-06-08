Status: Active
Source Idea Path: ideas/open/127_aarch64_alu_post_contract_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Current ALU Helper Clusters

# Current Packet

## Just Finished

Step 1 from `plan.md` mapped the current helper clusters in
`src/backend/mir/aarch64/codegen/alu.cpp` without implementation edits.

Helper/cluster map by responsibility:

- Operand preparation:
  - Prepared value/storage lookup and conversion helpers:
    `prepared_named_value_id`, `find_prepared_scalar_value_home`,
    `find_prepared_scalar_storage`, `register_class_from_bank`,
    `make_prepared_scalar_register_operand`,
    `make_scalar_spill_scratch_operand`, `find_scalar_frame_slot_by_id`,
    `find_named_value_home`, `make_named_scalar_operand`,
    `make_resolved_scalar_operand`, `make_prepared_scalar_operand`,
    `make_prepared_scalar_result_register_operand`,
    `make_prepared_scalar_result_operand`, and
    `control_prepared_scalar_result_operand`.
  - These mostly consume prepared names, value homes, storage plans,
    prepared register placements, frame-slot offsets, and resolved MIR
    operands, then spell AArch64 operand records. They do compute local
    register-view conversion, scratch-for-spill selection, and frame-slot
    load operand shape.
  - Uncertain for Step 2: whether the fallback paths around
    `make_named_scalar_operand`, `find_return_abi_register`,
    `find_return_chain_register`, and stack-home load helpers are still pure
    prepared-fact consumers or are rebuilding authority already covered by
    ideas 51, 55, 71, and 74.

- Producer/publication:
  - Producer and publication lookup helpers:
    `value_publication_may_write_scratch_register`,
    `find_prepared_load_local_source_producer`,
    `load_local_home_needs_consumer_publication`,
    `make_unpublished_load_local_source_operand`,
    `find_prepared_control_same_block_scalar_producer`,
    `authoritative_immediate_storage`,
    `uses_unemitted_authoritative_immediate`,
    `find_emitted_scalar_register`, `record_emitted_scalar_register`,
    `clear_emitted_scalar_register`, and
    `clear_call_clobbered_emitted_scalar_registers`.
  - These consume prepared same-block producer/publication lookups, current
    block entry publication queries, load-local source producer facts,
    return-chain facts, and the local emitted-register cache. The local cache
    updates are AArch64 lowering state, but the same-block producer and
    materializability decisions need later audit against closed producer work.
  - Uncertain for Step 3: `value_publication_may_write_scratch_register`
    directly inspects producer instruction kinds and recurses through casts
    and binary producers; later steps should compare that with ideas 116,
    117, 122, and 123 before deciding whether it is a shared-boundary gap or
    target-local scratch-risk analysis.

- Immediate/materialization:
  - Immediate and constant helpers:
    `value_power_of_two_shift`, `scalar_immediate_name`,
    `is_plain_add_sub_immediate`, `is_plain_logical_immediate`,
    `is_plain_mov_immediate`, `scalar_alu_operation_accepts_immediate`,
    `integer_scalar_bit_width`, `scalar_type_size_bytes`,
    `unsigned_power_of_two_log2`,
    `unsigned_reduction_replacement_immediate`,
    `is_unsigned_power_of_two_reduction_opcode`,
    `unsigned_reduction_operation`,
    `unsigned_reduction_post_zero_extend_bits`,
    `make_immediate_scalar_operand`, `make_scalar_immediate_operand`, and
    in-printer lambdas that materialize integer immediates through
    `materialize_integer_constant_lines`.
  - These appear to decide AArch64 encodability, immediate spelling,
    power-of-two UDiv/URem reduction mechanics, and scratch materialization.
    They also carry source value id/name through immediate operands.
  - Uncertain for Step 4: whether unsigned reduction is purely AArch64
    lowering policy or should be represented as target-neutral semantic
    materializability somewhere shared.

- Control/fallback:
  - Fallback/control helpers:
    `ScalarFallbackOperandSelector`, `make_scalar_fallback_operand`,
    `make_control_publication_operand`, `MaterializedControlSource`,
    `materialize_control_register_source`,
    `materialize_control_compare_rhs`,
    `materialize_control_binary_result_source`,
    `append_control_value_to_register`,
    `append_move_control_value_to_register`,
    `append_control_binary_to_register`,
    `append_control_cast_to_register`,
    `lower_scalar_compare_publication`,
    `lower_scalar_select_publication`, and
    `lower_scalar_control_value_instruction`.
  - These combine prepared operands, local emitted-register state,
    prepared stack/load sources, same-block producer facts, select-chain
    materialization, and AArch64 compare/cset/csel/move/load spelling.
  - Uncertain for Step 5: fallback operand ordering, control-source producer
    use, return ABI retargeting, and select-chain materialization should be
    checked against closed scalar/control-flow prepared authority before
    classifying the cluster.

- Arithmetic emission:
  - Opcode/semantic and arithmetic record helpers:
    `is_scalar_compare_opcode`, `scalar_compare_condition`,
    `scalar_alu_opcode_semantic_facts`,
    `is_scalar_alu_publication_opcode`,
    `scalar_alu_publication_operation`, `scalar_alu_semantic_facts`,
    `scalar_alu_post_sign_extend_bits`, `scalar_registers_alias`,
    `scalar_gp_scratch_register`, `scalar_gp_scratch_name`,
    `is_scalar_alu_integer_opcode`, `is_scalar_unary_integer_operation`,
    `scalar_alu_operation_from_binary_opcode`, `scalar_register_view`,
    `scalar_storage_register_view`,
    `make_prepared_scalar_alu_record`,
    `make_prepared_scalar_unary_record`,
    `make_scalar_alu_instruction_record`,
    `make_scalar_unary_instruction_record`,
    `make_scalar_alu_print_lines`,
    `lower_scalar_mul_with_distinct_rhs_scratch`, and
    `lower_scalar_instruction`.
  - These select AArch64 opcode families, views, scratch registers, alias
    avoidance, post-extension spelling, immediate-vs-register materialization,
    and assembler/MIR lowering routes after prepared facts have been consumed.
  - Uncertain for later steps: `lower_scalar_instruction` owns several
    fallback paths after prepared-record construction fails; those are the
    main place to check for authority rebuilding rather than target-local
    emission.

- Machine-record emission:
  - Machine/record helpers:
    `scalar_alu_record_error`, `scalar_unary_record_error`,
    `scalar_instruction_record_error`,
    `scalar_alu_stack_publication_line`,
    `scalar_publication_width_bytes`,
    `stack_publication_direct_offset_is_encodable`,
    `scalar_result_uses_scratch`,
    `scalar_publication_address_scratch`,
    `scalar_alu_stack_publication_lines`,
    `append_scalar_alu_stack_publication`,
    `register_display_name`, `occupied_register_views`,
    `occupied_register_references`, `relocation_operand`,
    `make_control_publication_assembler`,
    `lower_scalar_instruction`, and
    `lower_scalar_mul_with_distinct_rhs_scratch`.
  - These build `ScalarInstructionRecord`, `InstructionRecord`,
    `AssemblerInstructionRecord`, `module::MachineInstruction`, origin data,
    publication stores, and diagnostic-bearing print results. They look
    target-local unless a later audit finds hidden shared publication
    authority in the inputs they consume.

## Suggested Next

Execute Step 2 from `plan.md`: audit prepared operand and result-home
boundaries, focusing on the operand-preparation and fallback result-register
paths identified above. Compare uncertain areas against closed ideas 51, 55,
71, and 74 before classifying.

## Watchouts

- This route is analysis-only; do not edit implementation files, tests, or
  build metadata.
- Do not treat `alu.cpp` line count as evidence of a boundary gap.
- Follow-up ideas must be concrete and must not duplicate ideas 51, 55, 71,
  74, 116, 117, 122, or 123.
- Step 1 did not classify ownership boundaries; it only mapped helper
  clusters and parked uncertain areas for Steps 2-5.
- The most likely boundary-sensitive areas are fallback paths in
  `lower_scalar_instruction`, same-block producer inspection in
  `value_publication_may_write_scratch_register`, and control publication
  source materialization.

## Proof

Analysis-only packet. No build or test command required by the supervisor.
Validation: confirm the only changed file is `todo.md`; no `test_after.log`
was produced because this packet had no delegated proof command.
