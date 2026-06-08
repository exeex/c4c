Status: Active
Source Idea Path: ideas/open/127_aarch64_alu_post_contract_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Audit Scalar Producer And Publication Consumption

# Current Packet

## Just Finished

Step 3 from `plan.md` audited scalar producer and publication consumption in
`src/backend/mir/aarch64/codegen/alu.cpp` without implementation edits.

Classification for audit standard 2, scalar producer and publication
consumption: `no-new-idea`.

Producer/publication consumption evidence:

- Same-block scalar producers: `find_prepared_control_same_block_scalar_producer`
  does not scan BIR locally. It builds missing prepared lookups only from
  `make_prepared_function_lookups` when the context did not carry cached
  lookups, then delegates to `prepare::find_prepared_same_block_scalar_producer`
  with prepared names, edge-publication source producers, the current block
  label, the BIR block, the queried value, and the before-instruction index.
  Control materialization helpers consume that prepared producer result before
  emitting AArch64 register moves, compares, or binary operations.
- Load-local source producers:
  `find_prepared_load_local_source_producer` delegates to
  `prepare::find_prepared_same_block_load_local_source_producer` using prepared
  names, stack layout, prepared memory-access lookup, edge-publication source
  producers, the current block label, and instruction position.
  `make_unpublished_load_local_source_operand` then checks the prepared source
  access and prepared value home, converts it through
  `make_prepared_scalar_load_source`, and only adjusts AArch64 memory operand
  width/alignment for the load result type.
- Current-block entry publication:
  `value_publication_may_write_scratch_register` consumes the shared
  `current_block_entry_publication_register` query before consulting the
  injected same-block producer callback. If a current-block publication
  register already exists for the value/view, the helper returns false and does
  not rederive publication authority locally.
- Stack publication: `lower_scalar_select_publication`,
  `lower_scalar_compare_publication`, structured ALU print paths, and fallback
  publication paths all call `scalar_alu_stack_publication_lines` only after a
  prepared result operand has supplied `result_stack_offset_bytes`.
  `scalar_alu_stack_publication_lines` decides store width, direct-offset
  encodability, base register spelling, and address-scratch materialization;
  it does not decide whether a value needs stack publication or where the home
  lives.
- Select-chain dependency decisions: `lower_scalar_select_publication` calls
  `prepare::find_prepared_scalar_select_chain_materialization` with prepared
  names, prepared edge-publication source producers, the current block label,
  the BIR block, result value, and package index. The branch that handles
  `direct_global_dependency` only emits the prepared dependency through
  `emit_select_chain_value_to_register` and then performs normal AArch64 stack
  publication and emitted-register recording.

Closed-idea evidence:

- Idea 116 closed the dispatch producer contract by moving
  edge-publication source consistency, current-block join instruction routing,
  and direct-global select-chain dependency lookup into shared prepared facts,
  while leaving machine-register hazard policy and target emission glue local.
  ALU consumes the same prepared producer/select-chain surfaces instead of
  recreating dispatch-style routing.
- Idea 117 closed comparison current-block publication authority by exposing
  fused-compare producer facts, materialized compare join/publication facts,
  and current-block publication register facts through shared prepared
  authority. ALU's use of `current_block_entry_publication_register` is
  consumption of that family of current-block publication facts.
- Idea 122 closed call-argument producer materializability by requiring shared
  producer materializability, publication-source routing, direct-global
  select-chain dependency visibility, and missing frame-slot publication need
  queries. ALU's load-local/select-chain paths consume equivalent shared
  producer/publication facts and keep only target emission local.
- Idea 123 closed call-result late publication by adding producer-fact-backed
  `PreparedCurrentBlockPublicationConsumption` /
  `find_prepared_current_block_publication_consumption` surfaces and by keeping
  target materialization/recording local. ALU does not duplicate that call
  late-publication decision; its current-block check is a preexisting shared
  publication query and its result recording is local lowering state.

Remaining ALU-local authority that is acceptable for this standard:

- `value_publication_may_write_scratch_register` inspects prepared same-block
  producer instruction kinds and recursively follows casts/power-of-two
  multiply operands, but the decision it owns is whether AArch64 publication
  may need a scratch register. That is machine-register hazard analysis, not
  durable target-neutral producer materializability.
- `load_local_home_needs_consumer_publication` parses the prepared home
  register name only to decide whether the target memory operand path is useful
  for AArch64 GP publication. The home and source access still come from
  prepared facts.
- The emitted-register cache (`find_emitted_scalar_register` /
  `record_emitted_scalar_register`) remains block-local lowering state used to
  reuse already emitted machine values. It does not create durable value-home,
  producer, or publication authority.

No follow-up idea is warranted for Step 3. The apparent local logic is either
shared prepared producer/publication consumption or AArch64-local hazard,
operand, stack-store, and emission policy.

## Retained Step 1 Helper/Cluster Map

Step 1 from `plan.md` mapped the current helper clusters in
`src/backend/mir/aarch64/codegen/alu.cpp` without implementation edits. This
section is retained as stable audit evidence for Step 6 closure output.

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
  - Step 2 resolved the uncertainty around `make_named_scalar_operand`,
    `find_return_abi_register`, `find_return_chain_register`, and stack-home
    load helpers as prepared-fact consumption plus AArch64-local spelling.

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

## Retained Step 2 Prepared Operand/Result-Home Audit

Step 2 from `plan.md` audited prepared operand and result-home boundaries in
`src/backend/mir/aarch64/codegen/alu.cpp` without implementation edits. This
section is retained as stable audit evidence for Step 6 closure output.

Classification for audit standard 1, prepared scalar operand and result homes:
`no-new-idea`.

Prepared-fact consumption evidence:

- Structured ALU/unary record construction consumes prepared homes and storage:
  `make_prepared_scalar_operand`,
  `make_prepared_scalar_result_register_operand`,
  `make_prepared_scalar_result_operand`,
  `control_prepared_scalar_result_operand`,
  `make_prepared_scalar_alu_record`, and
  `make_prepared_scalar_unary_record` all flow through
  `find_prepared_scalar_value_home` / `find_named_value_home`,
  `find_prepared_scalar_storage`, and `make_prepared_scalar_register_operand`
  before constructing operand/result records.
- Register results and operands use prepared value ids/names, storage-plan
  bank/encoding, prepared register placements or matching prepared
  home/storage register names, then call `abi::convert_prepared_register`.
  The remaining register-view conversion and display-name spelling are
  AArch64-local emission policy, matching idea 74's closed boundary.
- Stack-slot operands/results use prepared storage slot id,
  `stack_offset_bytes`, `PreparedFrameSlot` offsets, prepared memory-access
  lookups, or prepared stack-home facts to produce `MemoryOperand` records and
  spill scratch registers. The scratch choice and address/offset encodability
  are target-local; the home/offset authority is prepared input.
- `make_named_scalar_operand` delegates to shared `resolve_value_operand`,
  which decodes home/storage through `decode_prepared_home_storage` using
  regalloc, storage plan, value locations, and value-home indexes. It records
  the resulting `OperandAuthority` rather than rebuilding storage authority in
  ALU.
- Return result retargeting no longer scans raw move bundles or forward BIR
  chains in ALU. `find_return_abi_register` consumes
  `find_prepared_before_return_abi_move_by_source_and_destination_bank`, and
  `find_return_chain_register` consumes prepared return-chain lookup facts plus
  indexed value-home lookup before converting the destination register.
- The publication fallback in `lower_scalar_instruction` reuses prepared
  result-home/storage helpers when it cannot build the structured record,
  then builds an AArch64 record only after it has a prepared result register,
  prepared stack offset, and prepared/resolved operands.

Remaining ALU-local authority that is acceptable for this standard:

- AArch64 register view selection, register-name parsing/retargeting for view
  compatibility, scratch-for-spill selection, occupied-register spelling, and
  stack-address encodability.
- Local emitted-register cache preference in fallback operand selection. This
  is lowering-state reuse, not durable home authority; Step 3/5 can still audit
  producer/publication ordering around it.
- Iterating `storage_plan.values` inside `find_prepared_scalar_storage` is a
  local lookup over the prepared storage plan, not raw storage recovery. It may
  be a future clarity/indexing opportunity only if repeated lookup cost or
  review visibility becomes a real problem.

Closed-idea evidence:

- Idea 51 directly required ALU structured records and operands/results to
  consume `PreparedValueHome`, storage-plan value facts, prepared memory-access
  lookup, before-return move lookup, return-chain lookup, and select-chain
  materialization authority. Its closure note says no prepared-access
  value-spelling scans, same-block alias scans, raw move-bundle scans, or
  forward BIR return-chain walks remained in ALU.
- Idea 55 closed the stale scalar cast/ALU publication home route by requiring
  cast and ALU publication lowering to consume prepared source/home authority
  for pointer-derived scalar publications instead of stale live registers or
  raw BIR producer scans.
- Idea 71 classified prepared scalar homes, storage plans, regalloc facts,
  decoded home storage, prepared producers, computed values, and prepared
  control-flow facts as `consume-shared`, while keeping AArch64 immediate
  admissibility, materialization, condition-code spelling, and record
  construction local.
- Idea 74 closed the repeated register-view helper route by keeping BIR facts
  and `abi::convert_prepared_register` as inputs and keeping AArch64
  register-view/record/printer spelling local.

No new prepared operand/result-home follow-up idea is warranted by Step 2.
The remaining uncertain areas from Step 1 either consume prepared facts now
or belong to later producer/publication and fallback/control-source audits.

## Suggested Next

Execute Step 4 from `plan.md`: audit immediate and constant materialization
policy. Separate target-neutral immediate semantics from AArch64 immediate
encodability, unsigned reduction, scratch materialization, and assembler
spelling.

## Watchouts

- Step 2 found `no-new-idea` for prepared scalar operand/result homes, and
  Step 3 found `no-new-idea` for scalar producer/publication consumption.
  Do not reopen ideas 51, 55, 71, 74, 116, 117, 122, or 123 without new
  first-bad evidence.
- Step 4 should focus on `value_power_of_two_shift`, scalar immediate
  admissibility helpers, unsigned div/rem reduction helpers, and
  `materialize_integer_constant_lines` use. The main question is whether ALU
  decides target-neutral semantic materializability or only AArch64
  encodability/scratch mechanics.
- This route is analysis-only; do not edit implementation files, tests, build
  metadata, `plan.md`, or source/closed ideas.

## Proof

Analysis-only correction. No build or tests were run by supervisor
instruction, and no `test_after.log` was produced because this packet had no
delegated build/test proof command. No clang-tools were needed for this
todo-only evidence preservation fix. Validation: `git status --short` shows
only `M todo.md`.
