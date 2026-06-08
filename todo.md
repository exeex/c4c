Status: Active
Source Idea Path: ideas/open/127_aarch64_alu_post_contract_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Decide Physical Split Readiness And Draft Outputs

# Current Packet

## Just Finished

Step 6 from `plan.md` synthesized the retained AArch64 ALU boundary audit
evidence into closure-ready output without implementation edits or new idea
files.

Final classification table:

| Audit standard | Classification | Closure decision |
| --- | --- | --- |
| Prepared scalar operand and result homes | `no-new-idea` | Covered by prepared value-home/storage/regalloc consumption in current ALU plus closed ideas 51, 55, 71, and 74. |
| Scalar producer and publication consumption | `no-new-idea` | Covered by prepared same-block producer, load-local producer, current-block publication, stack-publication, and select-chain dependency surfaces plus closed ideas 116, 117, 122, and 123. |
| Immediate and constant materialization policy | `no-new-idea` | Remaining decisions are AArch64 immediate encodability, UDiv/URem power-of-two instruction selection, scratch materialization, and assembler spelling. |
| Control-source and fallback operand materialization | `no-new-idea` | Fallback/control paths consume prepared homes, prepared producers, prepared return facts, prepared select-chain materialization, prepared load sources, and local emitted-register state. |
| Physical split readiness for local AArch64 ALU clarity | `no-new-idea` | A physical split is not required for this boundary audit. A future extraction would be purely mechanical and AArch64-local, not a shared-authority follow-up. |

Explicit `no new idea` entries:

- Prepared operand/result-home boundary: no new idea. Current ALU consumes
  prepared home, storage, regalloc, return ABI, return-chain, stack-home, and
  resolved-operand authority; remaining register views, scratch selection,
  stack-address encodability, and record spelling are target-local.
- Producer/publication boundary: no new idea. Current ALU consumes prepared
  same-block producer, load-local producer, current-block publication,
  stack-publication, and select-chain dependency surfaces; remaining logic is
  AArch64 scratch-risk, emitted-register cache, memory operand, store, and
  publication emission policy.
- Immediate/constant boundary: no new idea. Current ALU owns only target
  encodability, immediate spelling, power-of-two reduction instruction
  selection, scratch setup, and `movz`/`movk` line emission after source
  immediates or prepared rematerializable homes are known.
- Control-source/fallback boundary: no new idea. Current ALU consumes prepared
  homes, prepared load-source facts, prepared return facts, prepared
  same-block binary/cast producer facts, and prepared select-chain
  materialization; remaining operand preference, scratch avoidance, return
  operand retargeting metadata, and instruction emission are AArch64-local.
- Physical split readiness: no new idea. The helper/cluster map is retained
  below and supports review visibility. There is no concrete unresolved
  follow-up with a shared owner boundary, likely files, proof route, and
  testcase-overfit reject signals.

Physical split readiness decision:

- Do not open a shared-boundary, contract-visibility, or closure-blocking
  physical split idea from this audit. The five audited standards are clean
  against the source idea's owner-boundary criteria.
- A future local extraction may be reasonable only as a mechanical
  AArch64-local maintainability task if review cost becomes the first-bad
  evidence. Its likely scope would be to split existing clusters such as
  operand preparation, producer/publication consumption, immediate
  materialization, control/fallback materialization, arithmetic emission, and
  machine-record emission under AArch64 codegen while preserving behavior.
  That possible task is not needed for source-idea closure and does not
  warrant a new idea in this packet.
- No blocker or candidate follow-up was found. No new `ideas/open/` file was
  created.

The retained Step 1 helper/cluster map and full Step 2 through Step 5 evidence
remain in this file for plan-owner closure review.

## Suggested Next

Supervisor should hand the completed audit to the plan owner for lifecycle
closure review. No executor implementation packet is suggested from this
audit.

## Watchouts

- Do not reopen closed ideas 51, 55, 71, 74, 116, 117, 122, or 123 without new
  first-bad evidence.
- Do not convert the non-blocking AArch64-local mechanical extraction option
  into a shared-authority idea. It lacks a concrete unresolved owner-boundary
  defect in this audit.
- Any future split idea should reject testcase overfit by requiring unchanged
  generated behavior, unchanged test expectations, and proof that the split is
  mechanical rather than a narrow route to make one ALU case pass.

## Proof

Analysis-only packet. No build or tests were run by supervisor instruction, and
no `test_after.log` was produced because this packet had no delegated
build/test proof command. Validation: `git diff --check` passed, and
`git status --short` verified that only `todo.md` changed.

## Retained Step 5 Control-Source/Fallback Audit

Step 5 from `plan.md` audited control-source and fallback operand
materialization in
`src/backend/mir/aarch64/codegen/alu.cpp` without implementation edits. This
section is retained as stable audit evidence for Step 6 closure output.

Classification for audit standard 4, control-source and fallback operand
materialization: `no-new-idea`.

Control-source/fallback evidence:

- Fallback operand ordering is local lowering-state preference over prepared
  facts, not authority reconstruction. `ScalarFallbackOperandSelector::select`
  checks literal immediates first, then a local emitted-register cache keyed by
  the prepared value home, then prepared unpublished load-local source operands,
  then `make_named_scalar_operand`, then prepared scalar load sources. The
  direct callee evidence for `lower_scalar_instruction` shows this path is used
  only after structured prepared ALU record construction fails, while result
  homes, return ABI registers, authoritative immediates, return-chain lookups,
  prepared result operands, and publication opcode checks are still consumed
  before constructing the fallback `ScalarAluRecord`.
- Control publication operands consume prepared homes and prepared load-source
  surfaces. `make_control_publication_operand` accepts only literal immediates,
  emitted registers for a prepared home, `make_prepared_scalar_load_source`,
  and `make_prepared_stack_home_load_source`; it rejects reserved MIR scratch
  as the preferred emitted operand and falls back to prepared memory operands
  when allowed. Its direct callees are immediate conversion, prepared-home
  lookup, emitted-register lookup, prepared scalar/stack load-source helpers,
  register wrapping, memory wrapping, and scratch-reservation checks.
- Return ABI retargeting consumes prepared return facts. `find_return_abi_register`
  calls `find_prepared_before_return_abi_move_by_source_and_destination_bank`
  with prepared move-bundle/value-location state, then converts the prepared
  destination placement to the AArch64 view. `find_return_chain_register` calls
  `find_prepared_return_chain_terminal_value`, looks up the terminal prepared
  home, calls `find_return_abi_register`, and then `retarget_register_operand`
  only rewrites local register operand metadata to the current result value.
  The extra return-chain conflict branch in `lower_scalar_instruction` uses
  `find_prepared_return_chain_next_operand_value` plus indexed prepared homes
  to avoid clobbering a following operand, then chooses an AArch64 scratch
  register. It does not walk BIR return chains locally.
- Materialized control-source helpers consume operand records and prepared
  same-block producer facts, then emit AArch64 instructions. Direct callee
  evidence shows `materialize_control_register_source` only names registers,
  loads prepared memory operands, emits symbol/frame-slot address loads, or
  moves immediates into scratch registers. `materialize_control_compare_rhs`
  adds the AArch64 compare-immediate 0..4095 shortcut before delegating to the
  same register-source materializer.
- Binary/cast control-source reconstruction is bounded to prepared producers.
  `materialize_control_binary_result_source` obtains a scratch register, calls
  `find_prepared_control_same_block_scalar_producer`, and only emits a binary
  or cast when the prepared producer kind carries the corresponding BIR
  instruction pointer. `append_control_value_to_register` uses the same prepared
  producer query, falls back to prepared stack-home/load-local sources when
  materialization fails, and otherwise moves a control publication operand. The
  AST callee set contains the prepared producer query and prepared load-source
  helpers rather than raw producer scans.
- Compare/select publication lowerers consume prepared result and select-chain
  facts. `lower_scalar_compare_publication` calls
  `control_prepared_scalar_result_operand`, uses control publication operands
  for both sides, then emits `cmp`/`cset` and prepared stack publication lines.
  `lower_scalar_select_publication` calls
  `find_prepared_scalar_select_chain_materialization` before the direct-global
  select-chain path and `emit_select_chain_value_to_register` only with that
  prepared materialization/dependency record; its ordinary path uses control
  publication operands plus materialized binary-result sources for compare
  sides, then emits `cmp`/`csel` and stack publication.

Remaining ALU-local authority that is acceptable for this standard:

- Operand ordering prefers already emitted registers before prepared memory
  homes, and control publication avoids reserved scratch registers where
  possible. Those are AArch64 lowering-state and scratch-safety decisions, not
  durable value-home or producer authority.
- The materializers choose AArch64 scratch registers, register views,
  compare-immediate admissibility, frame-slot/symbol load spelling, and
  `cmp`/`cset`/`csel`/`mov`/binary/cast instruction text. These are target-local
  emission mechanics explicitly outside the source idea's shared-authority
  target.
- The control-source helpers recurse through prepared same-block binary/cast
  producers to avoid unnecessary stack loads, but the producer identity and
  source-routing authority come from prepared lookup surfaces.

No follow-up idea is warranted for Step 5. The audited control-source and
fallback paths consume prepared homes, prepared producers, prepared return
facts, prepared select-chain materialization, prepared load sources, and local
emitted-register state. The remaining logic is AArch64 operand ordering,
scratch avoidance, retargeted register operand metadata, and assembler emission.

## Retained Step 4 Immediate/Constant Audit

Step 4 from `plan.md` audited immediate and constant materialization policy in
`src/backend/mir/aarch64/codegen/alu.cpp` without implementation edits. This
section is retained as stable audit evidence for Step 6 closure output.

Classification for audit standard 3, immediate and constant materialization
policy: `no-new-idea`.

Immediate/materialization evidence:

- `value_power_of_two_shift` decides only whether an already known immediate or
  same-block integer constant is a nonnegative power of two. `c4c-clang-tool`
  caller evidence shows its only direct ALU caller is
  `value_publication_may_write_scratch_register`, where it recognizes
  multiply-by-power-of-two as a scratch-risk shortcut before recursively
  checking the nonconstant operand. It does not decide durable scalar producer,
  value-home, or target-neutral materializability authority.
- Scalar immediate admissibility helpers are AArch64 encodability and spelling
  checks. `scalar_immediate_name` spells `#<value>`,
  `is_plain_add_sub_immediate` accepts nonnegative 12-bit add/sub immediates,
  `is_plain_logical_immediate` currently returns false, `is_plain_mov_immediate`
  accepts the local single-`mov` range, and
  `scalar_alu_operation_accepts_immediate` maps those checks to AArch64 ALU
  operation families. Direct callees of `make_scalar_alu_print_lines` include
  these helpers plus `scalar_gp_scratch_register`, `abi::register_name`, and
  `materialize_integer_constant_lines`, confirming the policy is print-time
  operand choice and scratch mechanics.
- Immediate operand construction is prepared-fact consumption or literal BIR
  immediate conversion. `make_scalar_immediate_operand` only accepts BIR
  immediate values with scalar register views and preserves source prepared
  value id/name when supplied. `authoritative_immediate_storage` checks a
  prepared `RematerializableImmediate` home against prepared storage-plan
  immediate encoding before calling `make_scalar_immediate_operand`; it does
  not invent immediate authority locally.
- Unsigned div/rem reduction helpers are local AArch64 lowering mechanics for
  a supported algebraic instruction selection. `scalar_alu_semantic_facts`
  marks same-scalar-type `UDiv`/`URem` as reduction candidates,
  `unsigned_power_of_two_log2` validates a typed power-of-two immediate divisor,
  `unsigned_reduction_replacement_immediate` converts `UDiv` into an `lsr`
  shift amount and `URem` into an `and` mask while preserving source immediate
  identity, and `unsigned_reduction_operation` selects
  `LogicalShiftRight`/`And`. `make_prepared_scalar_alu_record` uses that only
  after prepared operands/results are already built, and rejects non-immediate
  or out-of-width divisors instead of deriving broader target-neutral
  divisibility facts.
- `materialize_integer_constant_lines` is an AArch64 utility shared by multiple
  AArch64 codegen files. It emits `movz`/`movk` halfword chunks for a requested
  destination register and width. ALU uses it when an immediate cannot be used
  directly in the selected AArch64 instruction, when stack/publication address
  offsets need scratch materialization, and in fallback immediate-to-register
  paths. Those uses decide scratch register setup and assembler lines, not
  whether the source program value is a constant.

Remaining ALU-local authority that is acceptable for this standard:

- The local conversion of `UDiv` by a power-of-two immediate into `lsr`, and
  `URem` by a power-of-two immediate into `and`, is target instruction
  selection. It is visible in the ALU record as `source_binary_opcode` plus
  chosen `ScalarAluOperationKind` and post-zero-extension bits, so it does not
  hide a missing shared producer/materializability contract.
- The immediate admissibility ranges are deliberately target-specific. Moving
  them into prepared/BIR authority would violate the source idea's non-goal of
  moving AArch64 opcode choice, immediate encodability, scratch policy, or
  instruction spelling into shared code.
- The materialization helper is already physically factored into
  `constant_materialization.cpp`, but remains under AArch64 codegen. That is
  the right owner boundary for `movz`/`movk` spelling.

No follow-up idea is warranted for Step 4. The audited helpers decide
AArch64-local immediate encodability, reduction, scratch materialization, and
assembler spelling after prepared operands or literal BIR immediates are known.
No target-neutral immediate semantics or constant-materializability authority
needs to move to shared code.

## Retained Step 3 Scalar Producer/Publication Audit

Step 3 from `plan.md` audited scalar producer and publication consumption in
`src/backend/mir/aarch64/codegen/alu.cpp` without implementation edits. This
section is retained as stable audit evidence for Step 6 closure output.

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
