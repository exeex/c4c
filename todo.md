Status: Active
Source Idea Path: ideas/open/46_aarch64_duplicate_prepared_authority_audit.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Audit Value Materialization And Memory

# Current Packet

## Just Finished

Step 3 - Audit Value Materialization And Memory is complete for the AArch64
duplicate-authority audit. The Step 1 authority baseline is retained below
because later audit packets still depend on the same shared authority and
column contract.

Shared prepared authority baseline to use during the seven-file audit:

- Value homes: `PreparedValueHome`, `PreparedValueHomeKind`,
  `PreparedValueLocationFunction::value_homes`,
  `PreparedValueHomeLookups::homes_by_id`, and
  `PreparedValueHomeLookups::value_ids` own value-id, value-name, register,
  stack-slot, rematerializable-immediate, and pointer-base-plus-offset facts.
  Use `find_prepared_value_home`, `find_prepared_value_id`,
  `find_indexed_prepared_value_home`, `find_indexed_prepared_value_id`, and
  `make_prepared_value_home_lookups` as the named lookup baseline.
- Move and edge-copy route authority: `PreparedMoveBundle`,
  `PreparedMoveResolution`, `PreparedMovePhase::BlockEntry`,
  `PreparedMoveAuthorityKind::OutOfSsaParallelCopy`,
  `find_prepared_move_bundle`,
  `find_prepared_out_of_ssa_parallel_copy_move_for_step`,
  `find_prepared_unique_move_bundle`, `make_prepared_move_bundle_lookups`,
  and `find_indexed_prepared_move_bundle` own prepared move insertion
  coordinates and out-of-SSA parallel-copy move resolution. Treat register
  spelling/scratch emission as target-local after these facts are consumed.
- Edge publication authority: `PreparedEdgePublication`,
  `PreparedEdgePublicationLookups`,
  `PreparedEdgePublicationSourceProducer`,
  `PreparedEdgePublicationSourceProducerLookups`, and
  `PreparedEdgePublicationSourceProducerKind::{Immediate, LoadLocal, Cast,
  Binary, SelectMaterialization}` own predecessor/successor labels,
  destination/source value identity, source producer kind and instruction
  index, source/destination homes, matching move, parallel-copy step metadata,
  coalesced/redundant move flags, and load-local source memory facts. Use
  `make_prepared_edge_publication_lookups`,
  `make_prepared_edge_publication_source_producer_lookups`,
  `find_indexed_prepared_edge_publication_source_producer`,
  `find_indexed_prepared_edge_publications`,
  `find_unique_indexed_prepared_edge_publication`, and
  `find_unique_indexed_block_entry_parallel_copy_edge_publication`.
- Edge publication source memory and stack-source facts:
  `PreparedEdgePublicationSourceMemoryAccessStatus`,
  `source_memory_access`, `source_memory_base_kind`,
  `source_memory_frame_slot_id`, `source_memory_symbol_name`,
  `source_memory_pointer_value_name`, `source_memory_byte_offset`,
  `source_memory_size_bytes`, `source_memory_align_bytes`,
  `source_memory_can_use_base_plus_offset`,
  `source_memory_requires_address_materialization`,
  `PreparedAggregateStackSourceAuthority`, and
  `PreparedTypedStackSourcePublication` are shared evidence for load-local
  source recovery and typed/aggregate stack publication. Use
  `prepare_aggregate_stack_source_authority` and
  `prepare_same_width_i32_stack_source_publication` before accepting local
  source scans in AArch64.
- Block-entry publication authority: `PreparedBlockEntryPublication`,
  `PreparedBlockEntryPublicationStatus`, `collect_prepared_block_entry_publications`,
  and `prepared_block_entry_publication_available` own destination value,
  destination storage kind, destination register name, matched move bundle,
  matched move, and home for successor block-entry publication.
- Scalar publication authority: `PreparedScalarPublicationPlan`,
  `PreparedScalarPublicationInputs`, `PreparedScalarPublicationStatus`,
  `PreparedScalarPublicationHookKind`,
  `prepared_scalar_publication_available`,
  `prepared_storage_encoding_from_value_home_kind`,
  `prepared_publication_storage_encoding_from_home`, and
  `plan_prepared_scalar_publication` own scalar publication status, hook kind,
  source value identity, destination home kind, storage encoding, stack slot,
  immediate payload, pointer base/delta, and current block-entry-publication
  availability.
- Store-source publication authority: `PreparedStoreSourcePublicationPlan`,
  `PreparedStoreSourcePublicationInputs`,
  `PreparedStoreSourcePublicationIntent`,
  `PreparedStoreSourcePublicationStatus`,
  `prepared_store_source_publication_available`, and
  `plan_prepared_store_source_publication` own store source value/home,
  destination access, destination frame/object/stack facts, recovered narrow
  source value, byval load-local source, direct-global-select-chain dependency,
  pending/duplicate/stack-homes-only flags, pointer-store writeback, source
  producer, and pointer-base home.
- Store-source recovery helpers to name explicitly when encountered:
  `find_prepared_recovered_narrow_store_source_for_wide_local_load`,
  `prepared_store_source_load_local_is_byval_formal_pointer_source`, and
  `find_prepared_store_source_direct_global_select_chain_dependency`. These
  are shared prepared recovery helpers; AArch64 should not duplicate their
  same-block scan behavior unless the audit proves a missing shared query.
- Call plan authority: `PreparedCallPlan`, `PreparedCallArgumentPlan`,
  `PreparedCallResultPlan`, `PreparedCallPreservedValue`,
  `PreparedIndirectCalleePlan`, `PreparedMemoryReturnPlan`,
  `PreparedCallArgumentSourceSelection`,
  `PreparedCallArgumentSourceSelectionKind::{PriorPreservation,
  LocalFrameAddressMaterialization, FrameSlotAddress, FrameSlotValue,
  ByvalRegisterLane}`, `PreparedCallBoundaryEffectPlan`,
  `PreparedCallPlanLookups`, `make_prepared_call_plan_lookups`,
  `find_indexed_prepared_call_plan`,
  `find_latest_indexed_prior_preserved_value`,
  `find_dominating_indexed_prior_preserved_value`,
  `find_unique_indexed_prior_preserved_value_source`,
  `find_latest_indexed_prior_stack_preserved_value_before_instruction`,
  `first_indexed_stack_preserved_values_for_call`,
  `indexed_block_entry_republication_effects_for_block`,
  `classify_prepared_call_boundary_move`, and
  `plan_prepared_call_boundary_effects` own call position, wrapper kind,
  indirect callee, memory return, argument/result source and destination
  encodings, preservation source, prior preserved-value lookup, first stack
  preservation, and block-entry republication effects.
- Function-level prepared lookup bundle:
  `PreparedFunctionLookups` and `make_prepared_function_lookups` aggregate
  `call_plans`, `address_materializations`, `move_bundles`, `value_homes`,
  `edge_publications`, and `edge_publication_source_producers`. Audit rows
  should treat repeated AArch64 reconstruction of those same indexes as
  suspect unless the local code is only choosing AArch64 registers, scratch
  registers, addressing encodings, or instruction spelling.

ARM reference target-local responsibilities:

- `emit.rs` keeps register naming and register banks (`callee_saved_name`,
  `callee_saved_name_32`, `is_arm_fp_reg`), opcode/condition spelling
  (`arm_alu_mnemonic`, `arm_int_cond_code`, `arm_invert_cond_code`), immediate
  fitting (`const_as_imm12`, `const_as_power_of_2`, `const_as_cmp_imm12`,
  `const_as_cmn_imm12`), operand loading and destination stores
  (`operand_to_x0`, `operand_to_x0_x1`, `operand_to_callee_reg`,
  `store_x0_to`, `store_x0_x1_to`), stack/frame offset emission
  (`emit_sub_sp`, `emit_add_sp`, `emit_store_to_sp`, `emit_load_from_sp`,
  `emit_add_sp_offset`, `emit_alloca_addr`, `emit_load_from_reg`), prologue
  and epilogue emission, inline-asm scratch prescan, and call argument
  staging helpers. These are target-local emission and hazard decisions.
- `calls.rs` keeps AAPCS64 call ABI configuration, stack-argument byte
  layout, register-argument staging, indirect/direct call instruction
  spelling, stack cleanup, and result storage. It may load from known homes or
  registers, but it does not own semantic call source selection when prepared
  `PreparedCallArgumentSourceSelection` or `PreparedCallPlan` facts exist.
- `memory.rs` keeps load/store instruction choice, F128 dispatch to softfloat,
  offset folding, over-aligned alloca address computation, indirect/direct
  slot addressing, pointer register materialization, typed load/store register
  width selection, stack pointer arithmetic, and memcpy byte-loop emission.
  It should remain target-local after shared `PreparedMemoryAccess`,
  `PreparedStoreSourcePublicationPlan`, and value-home facts choose the memory
  contract.
- `alu.rs` keeps AArch64 arithmetic instruction spelling, immediate
  optimizations, destination-register direct paths, scratch ordering for
  accumulator fallback, 32-bit sign/zero extension, division/remainder
  sequences, unary integer/float operations, and i128 copy emission.
- `comparison.rs` keeps compare instruction emission, float/f128 compare
  dispatch, condition-code selection, fused compare-branch shape, and select
  lowering. It can choose `cmp`, `cmn`, `fcmp`, `cset`, `csel`, and branch
  spelling but should not rederive semantic producer or value-home authority
  already present in prepared lookups.

Audit table column contract for Steps 2-4:

| Column | Required evidence |
| --- | --- |
| `file` | One of the seven audited AArch64 files. |
| `candidate helper/path` | Concrete local helper, fallback branch, scan, or call site name. |
| `local fact rederived` | The semantic fact AArch64 is deriving locally, such as source producer, source/home recovery, destination home, block-entry publication, call source, value home, producer index, prior preservation, or store source. |
| `shared authority to consume` | Concrete shared record/helper name, for example `PreparedEdgePublication`, `PreparedScalarPublicationPlan`, `PreparedStoreSourcePublicationPlan`, `PreparedBlockEntryPublication`, `PreparedCallPlan`, `PreparedValueHome`, `PreparedFunctionLookups`, or a specific `find_*`/`plan_*` helper. |
| `missing shared field/query` | Name the absent prepared field or lookup if existing shared authority is close but incomplete; otherwise write `none`. |
| `ARM reference responsibility` | `emit.rs`, `calls.rs`, `memory.rs`, `alu.rs`, or `comparison.rs` target-local analogue, with the concrete helper/responsibility. |
| `classification` | Exactly one of `consume-shared`, `missing-shared-field`, `target-emission`, `legacy-fallback`, or `needs-more-evidence`. |
| `proof needed` | Narrow proof or local read needed when classification is uncertain; otherwise `none`. |
| `reject signal` | The overfit/drift condition a reviewer should reject, such as expectation downgrade, named-case shortcut, duplicate same-block scan, or vague "move to BIR" wording. |

Step 2 audit table rows:

| file | candidate helper/path | local fact rederived | shared authority to consume | missing shared field/query | ARM reference responsibility | classification | proof needed | reject signal |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` | `prepared_edge_publication_producer_context` (`source_producer_block_label`, `source_producer_instruction_index`, `source_producer_kind`) | Edge source producer block, instruction index, and typed producer pointer for edge publication lowering. | `PreparedEdgePublication`, `PreparedEdgePublicationSourceProducer`, `find_indexed_prepared_edge_publication_source_producer`, `PreparedFunctionLookups::edge_publication_source_producers` | none | `emit.rs` operand loading after the source is known (`operand_to_x0`, `operand_to_callee_reg`) | consume-shared | none | Reject replacing the prepared source-producer fields with a fresh same-block scan or name-shaped producer match. |
| `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` | `find_edge_named_producer` plus `unique_branch_predecessor_context` fallback | Named value producer across successor, edge predecessor, and up to four linear predecessors when no prepared publication is required. | `PreparedEdgePublicationSourceProducerLookups`, `PreparedEdgePublication`, `find_indexed_prepared_edge_publication_source_producer`, `find_unique_indexed_prepared_edge_publication` | prepared edge dependency query by source value plus edge context, if fallback cases prove still unsupported by current lookups | `emit.rs` may load operands from already-known homes (`operand_to_x0`), but does not own predecessor producer discovery. | legacy-fallback | Local read of callers that pass `prepared_publication == nullptr` to decide whether each should be converted to prepared lookup consumption or left as pre-prepared fallback. | Reject extending the predecessor-depth scan or adding named-case shortcuts instead of consuming a prepared producer/publication fact. |
| `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` | `emit_edge_load_local_to_register_impl` `prepared_memory_access` path | Load-local source memory base, frame slot, pointer base value, and byte offset for edge publication materialization. | `PreparedEdgePublication::source_memory_*`, `PreparedEdgePublicationSourceMemoryAccessStatus`, `PreparedMemoryAccess`, `PreparedValueHome` | none | `emit.rs` owns load spelling and base-plus-offset emission (`emit_load_from_sp`, `emit_load_from_reg`). | consume-shared | none | Reject re-scanning BIR load-local/address operands when `PreparedEdgePublication` already carries source memory facts. |
| `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` | `edge_value_publication_may_read_register_index` and recursive operand checks | Register-read hazard for choosing target/scratch ordering while materializing cast, binary, and select producer chains. | `PreparedEdgePublication::source_home`, `PreparedEdgePublication::source_producer_kind`, `PreparedValueHome` | none | `emit.rs` owns scratch/register hazard decisions during compare and operand loading (`emit_int_cmp_insn`, `operand_to_x0`). | target-emission | none | Reject classifying scratch-order or register-alias checks as duplicate authority unless the route also rederives semantic source/home facts. |
| `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` | `should_emit_block_entry_edge_copy_move` and `lower_predecessor_select_parallel_copy_sources` | Out-of-SSA block-entry move redundancy and matching predecessor/successor edge publication for a parallel-copy move. | `PreparedMoveBundle`, `PreparedMoveAuthorityKind::OutOfSsaParallelCopy`, `find_prepared_move_bundle`, `find_unique_indexed_block_entry_parallel_copy_edge_publication`, `prepared_edge_publication_redundant_block_entry_parallel_copy_move`, `prepared_edge_publication_matches_parallel_copy_move_source` | none | `emit.rs` owns actual move/load/store emission after the copy contract is known (`store_x0_to`, `emit_store_to_sp`). | consume-shared | none | Reject local predecessor/successor/move matching that bypasses the prepared edge-publication and move-bundle helpers. |
| `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` | `emit_select_chain_value_to_register` and `materialize_direct_global_select_chain_call_argument` | Select-chain producer shape and direct-global-load dependency for late call-argument materialization. | `PreparedEdgePublicationSourceProducerKind::SelectMaterialization`, `PreparedEdgePublicationSourceProducerLookups`, `PreparedStoreSourcePublicationPlan::direct_global_select_chain_dependency` | prepared call-argument/select-chain dependency query if this call-argument path is not covered by current prepared call plans | `emit.rs` owns compare/select instruction spelling (`emit_int_cmp_insn`, `emit_select`) after source selection. | legacy-fallback | Local read during calls audit to decide whether this path belongs to `PreparedCallPlan` or a shared select-chain dependency helper. | Reject claiming progress by only special-casing direct-global select chains in this AArch64 helper. |
| `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` | `current_block_entry_publication_register` value-home name scan fallback | Value-name to `PreparedValueHome` recovery when `prepared_named_value_id` did not resolve the BIR value. | `PreparedValueHomeLookups::value_ids`, `find_prepared_value_id`, `find_indexed_prepared_value_home`, `PreparedBlockEntryPublication` | none | `emit.rs` can choose register names, but value-home lookup is shared prepared authority. | consume-shared | none | Reject manual iteration over `value_locations->value_homes` by spelling when indexed value-id/home lookups are available. |
| `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` | `collect_current_block_entry_publications`, `value_has_current_block_entry_publication`, `record_current_block_entry_publication_registers`, `block_entry_move_clobbers_current_join_publication` | Current block-entry publication availability, destination register, and clobber state for emitted scalar tracking. | `PreparedBlockEntryPublication`, `PreparedBlockEntryPublicationStatus`, `collect_prepared_block_entry_publications`, `prepared_block_entry_publication_available` | none | `emit.rs` owns register spelling and emitted move ordering, not block-entry publication discovery. | consume-shared | none | Reject rebuilding block-entry publication state from raw move bundles when `PreparedBlockEntryPublication` is already available. |
| `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` | `lower_missing_conditional_branch_condition_publication` | Conditional-branch condition producer instruction recovered by same-block producer scan before calling scalar lowering. | `PreparedBranchCondition`, `PreparedScalarPublicationPlan`, `PreparedEdgePublicationSourceProducerLookups` | prepared branch-condition producer instruction index or scalar-publication query for materialized branch conditions | `emit.rs` owns branch/compare emission (`emit_branch_nonzero`, `emit_int_cmp_insn`) after the condition source is known. | missing-shared-field | Inspect prepared control-flow branch-condition records and scalar publication planner before repair to decide where the producer index belongs. | Reject extending same-block condition scans or adding named branch-condition shortcuts instead of adding/consuming shared producer authority. |
| `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` | `lower_missing_fused_compare_operand_publication` and `lower_missing_fused_compare_operand_publications` | Fused-compare operand publication need, destination home, current block-entry publication, and scratch target selection. | `PreparedBranchCondition`, `PreparedScalarPublicationPlan`, `PreparedBlockEntryPublication`, `PreparedValueHome` | scalar-publication helper for branch/fused-compare operand publication, if `plan_prepared_scalar_publication` cannot represent this hook today | `emit.rs` owns fused compare-branch operand loading and compare spelling (`emit_int_cmp_insn`, `emit_fused_cmp_branch`). | consume-shared | Local read of `plan_prepared_scalar_publication` call sites before repair to verify the existing planner can cover fused-compare operands. | Reject lowering fused-compare operands through ad hoc same-block source scans when a scalar publication plan can supply the source/home contract. |
| `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` | `memory_load_result_feeds_before_return_fpr_abi` inside `retarget_memory_result_to_prepared_home` | Before-return FPR ABI consumer for a memory load result, found by scanning raw move bundles. | `PreparedMoveBundle`, `PreparedMovePhase::BeforeReturn`, `PreparedMoveBundleLookups`, `find_prepared_move_bundle`, `PreparedValueHome` | before-return move lookup by source value id and destination register bank | `emit.rs` owns result register spelling and loads/stores (`emit_load_from_sp`, `store_x0_to`), not return-move discovery. | missing-shared-field | Verify whether existing move-bundle indexes can answer source-value/destination-bank queries without raw bundle iteration. | Reject further raw scans of `move_bundles` for before-return publication facts. |
| `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` | `lower_local_slot_address_publication`, `local_slot_address_frame_offset`, `local_aggregate_address_frame_offset` | Local-slot/aggregate address base frame offset recovered from stack object source kind, object name, lane `.0`, and frame-slot scan. | `PreparedScalarPublicationPlan`, `PreparedValueHomeKind::PointerBasePlusOffset`, `PreparedAddressMaterialization`, `PreparedAggregateStackSourceAuthority` | prepared local-slot address/frame-offset query keyed by value or stack object | `emit.rs` owns `add sp/fp, #offset` address emission (`emit_add_sp_offset`, `emit_alloca_addr`). | missing-shared-field | Inspect prepared address-materialization records before repair to decide whether to add a query or consume an existing pointer-base-plus-offset home. | Reject string-shaped lane-name scans such as `.0` as the durable source of aggregate/local-slot address authority. |
| `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` | `lower_fixed_formal_store_local_publication` and `store_local_value_is_fixed_formal` | Fixed formal store source and destination frame-slot publication recovered from BIR params plus prepared memory access. | `PreparedStoreSourcePublicationPlan`, `PreparedStoreSourcePublicationInputs`, `PreparedMemoryAccess`, `plan_prepared_store_source_publication` | none | `emit.rs` owns final store mnemonic and frame-slot addressing (`emit_store_to_sp`, `store_x0_to`). | consume-shared | none | Reject rederiving formal-parameter identity from BIR params when the store-source publication planner can own source/destination intent. |
| `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` | `value_publication_may_read_register_index` recursive producer walk | Register-read hazard for scratch/target selection while publishing cast, binary, and select values. | `PreparedValueHome`, `PreparedBlockEntryPublication`, `PreparedEdgePublicationSourceProducerLookups` | none | `emit.rs` owns scratch ordering and compare operand loading hazards (`emit_int_cmp_insn`, `operand_to_x0`). | target-emission | none | Reject treating target-local register hazard analysis alone as duplicate authority without a semantic source/home rederivation. |

Step 3 audit table rows:

| file | candidate helper/path | local fact rederived | shared authority to consume | missing shared field/query | ARM reference responsibility | classification | proof needed | reject signal |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` | `emit_value_publication_to_register` same-block producer path (`find_same_block_named_producer`, recursive cast/binary/load/select lowering) | Same-block scalar source producer, operand source chain, and value materialization route for a named value. | `PreparedEdgePublicationSourceProducerLookups`, `PreparedEdgePublicationSourceProducer`, `PreparedScalarPublicationPlan`, `PreparedValueHome` | prepared scalar-materialization/source-producer query by value and instruction index if this helper must remain outside edge publication | `emit.rs` owns operand loading and instruction spelling (`operand_to_x0`, `emit_int_cmp_insn`) after the source contract is known. | legacy-fallback | During repair planning, inspect non-edge callers of `emit_value_publication_to_register` to decide whether they can consume an existing prepared producer or need a shared scalar materialization query. | Reject adding deeper same-block producer recursion or named-case producer shortcuts instead of consuming prepared source/home authority. |
| `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` | `current_block_entry_publication_register`, `value_has_current_block_entry_publication`, and `prepared_value_home_for_value` fast paths inside `emit_value_publication_to_register` | Current block-entry publication register and destination/source home for already-prepared values. | `PreparedBlockEntryPublication`, `PreparedValueHome`, `collect_prepared_block_entry_publications`, `prepared_block_entry_publication_available`, `find_indexed_prepared_value_home` | none | `emit.rs` can move between concrete registers, but does not own block-entry publication discovery. | consume-shared | none | Reject replacing prepared block-entry/value-home checks with raw move-bundle or value-name scans. |
| `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` | Load-local branch using `prepared_local_load_offset`, `emit_prepared_pointer_value_load_to_register`, and `find_prepared_recovered_narrow_store_source_for_wide_local_load` | Load-local source memory, frame slot/pointer base, recovered narrow store source, and stack offset for source reloading. | `PreparedMemoryAccess`, `PreparedAddressingFunction`, `PreparedValueHome`, `PreparedStoreSourcePublicationPlan`, `find_prepared_recovered_narrow_store_source_for_wide_local_load` | none | `memory.rs` owns frame-slot/pointer load emission and offset folding (`emit_load_with_const_offset_impl`, `emit_load_from_sp`, `emit_load_from_reg`). | consume-shared | none | Reject re-scanning load/store neighbors or BIR address operands when prepared memory access and recovered-source helpers already carry the facts. |
| `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` | Load-global branch (`find_load_global_target`, `load_global_symbol_label`, GOT/direct emission) | Global symbol target, symbol label, byte offset, and GOT-required policy for a load-global producer. | `PreparedMemoryAccess`, `PreparedAddressMaterialization`, `PreparedValueHome`; possibly a prepared global-symbol address query if global-load materialization remains separate from memory access | prepared global-load address/materialization query keyed by value or instruction, if existing prepared address materialization cannot carry GOT/direct policy | `memory.rs` owns global/pointer load address emission; `emit.rs` owns large-offset and load instruction spelling. | needs-more-evidence | Inspect prepared address-materialization records and global lowering callers before repair to decide whether global-load symbol policy is already shared. | Reject hard-coding global-name spelling or GOT policy in a value-shaped shortcut. |
| `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` | Select branch calling `emit_select_chain_value_to_register` with `prepared_named_value_id` | Select-chain source producer shape and selected value dependency for scalar materialization. | `PreparedEdgePublicationSourceProducerKind::SelectMaterialization`, `PreparedEdgePublicationSourceProducerLookups`, `PreparedStoreSourcePublicationPlan::direct_global_select_chain_source` | prepared scalar select-chain materialization query for non-edge/non-store consumers | `emit.rs` owns compare/select instruction spelling after the selected source is known. | legacy-fallback | Revisit with the calls/ALU audit to decide whether select-chain materialization belongs to a shared producer query or a call/store-specific prepared dependency. | Reject direct-global/select-chain named-case expansion in this local materializer. |
| `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` | `value_publication_may_write_scratch_register` plus `value_publication_may_read_register_index` ordering checks in binary lowering | Register-read/write hazard for target/scratch ordering while recursively materializing operands. | `PreparedValueHome`, `PreparedBlockEntryPublication`, `PreparedEdgePublicationSourceProducerLookups` | none | `emit.rs` owns scratch selection and operand loading hazards during target instruction emission. | target-emission | none | Reject classifying scratch-order checks alone as duplicate authority unless they start choosing semantic source/home facts. |
| `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` | Add/Sub address path via `emit_local_slot_address_publication_to_register` | Local-slot/aggregate address publication for pointer arithmetic materialization. | `PreparedScalarPublicationPlan`, `PreparedValueHomeKind::PointerBasePlusOffset`, `PreparedAddressMaterialization`, `PreparedAggregateStackSourceAuthority` | prepared local-slot address/frame-offset query remains the same Step 2 proof question if this helper cannot consume pointer-base-plus-offset homes directly | `memory.rs` owns GEP/direct constant address emission (`emit_gep_direct_const_impl`, `emit_add_sp_offset`). | missing-shared-field | Inspect prepared address-materialization records before repair; Step 2 already flagged the same local-slot address authority gap. | Reject string-shaped lane or local-slot scans as durable address authority. |
| `src/backend/mir/aarch64/codegen/memory.cpp` | `find_value_home_id` and `find_unique_value_home_id` loops over `value_locations.value_homes` | Value-name to prepared value-id recovery for memory result, stored value, and pointer-value base identity. | `PreparedValueHomeLookups::value_ids`, `find_prepared_value_id`, `find_indexed_prepared_value_id`, `PreparedFunctionLookups::value_homes` | none | `memory.rs` may choose load/store operands, but value-id/home lookup is shared prepared authority. | consume-shared | none | Reject new manual `value_homes` scans when indexed prepared value-id lookup is available. |
| `src/backend/mir/aarch64/codegen/memory.cpp` | `make_memory_record_from_prepared_access`, `make_prepared_*_memory_instruction_record`, `apply_load_identity`, and `apply_store_identity` | Memory base kind, frame/global/pointer/string identity, result/stored value identity, destination/source value home, and storage encoding for load/store records. | `PreparedMemoryAccess`, `PreparedAddressingFunction`, `PreparedValueHome`, `PreparedStoragePlanFunction`, `find_prepared_memory_access`, `find_prepared_value_home` | none | `memory.rs` owns load/store instruction choice, offset folding, and indirect/direct slot addressing after the memory contract is selected. | consume-shared | none | Reject rebuilding memory address/value identity from raw BIR when a `PreparedMemoryAccess` exists for the instruction. |
| `src/backend/mir/aarch64/codegen/memory.cpp` | `find_memory_return_abi_register` and `retarget_load_result_to_return_abi` | Before-return ABI destination register for a memory load result, found by scanning raw `move_bundles`. | `PreparedMoveBundle`, `PreparedMovePhase::BeforeReturn`, `PreparedMoveBundleLookups`, `find_prepared_move_bundle`, `PreparedValueHome` | before-return move lookup by source value id and destination register bank | `emit.rs` owns return register spelling and final load/store emission (`emit_load_from_sp`, `store_x0_to`). | missing-shared-field | Verify whether existing move-bundle indexes can answer source-value/destination-bank queries without raw bundle iteration. | Reject more raw scans of `move_bundles` for before-return publication/retargeting facts. |
| `src/backend/mir/aarch64/codegen/memory.cpp` | `find_stack_object_by_value_name` and `rewrite_local_address_store_value` | Stack object/frame slot for a pointer-valued store source recovered from value spelling and stack object names. | `PreparedMemoryAccess`, `PreparedStoreSourcePublicationPlan`, `PreparedAddressMaterialization`, `PreparedAggregateStackSourceAuthority` | prepared stack-object/frame-slot lookup keyed by value name or object id for local-address store values | `memory.rs` owns storing addresses to slots and alloca address emission (`emit_store_with_const_offset_impl`, `emit_alloca_addr`). | missing-shared-field | Inspect prepared stack-layout/address-materialization records before repair to decide whether a new lookup or existing object id should be consumed. | Reject text-name matching between value names and stack object names as durable source authority. |
| `src/backend/mir/aarch64/codegen/memory.cpp` | `emit_prepared_pointer_value_load_to_register` and `lower_stack_homed_pointer_value_load_publication` | Pointer-value base home, pointer base stack/register location, load result home, and publication destination for stack-homed pointer loads. | `PreparedMemoryAccess`, `PreparedValueHome`, `PreparedStoreSourcePublicationPlan`, `find_indexed_prepared_value_home` | none | `memory.rs` owns pointer-register materialization, offset folding, and load/store spelling (`emit_load_ptr_from_slot_impl`, `emit_load_with_const_offset_impl`). | consume-shared | none | Reject rederiving pointer-base identity from BIR address operands when `PreparedMemoryAccess::address.pointer_value_name` and value homes are available. |
| `src/backend/mir/aarch64/codegen/memory.cpp` | `plan_store_local_source_publication`, `prepared_store_source_producer`, and `prepared_recovered_narrow_store_source` | Store-local source value/home, source producer kind/index, destination frame/object facts, recovered narrow source, byval source, and direct-global select-chain dependency. | `PreparedStoreSourcePublicationPlan`, `PreparedStoreSourcePublicationInputs`, `PreparedEdgePublicationSourceProducerLookups`, `find_prepared_recovered_narrow_store_source_for_wide_local_load`, `prepared_store_source_load_local_is_byval_formal_pointer_source`, `find_prepared_store_source_direct_global_select_chain_dependency` | none | `memory.rs` owns actual store mnemonic, address register, and stack-slot emission after source/destination intent is known. | consume-shared | none | Reject duplicating source-producer scans in AArch64 instead of feeding/consuming the store-source publication planner. |
| `src/backend/mir/aarch64/codegen/memory.cpp` | `lower_store_local_value_publication` emission branches for select, cast, scalar-fp binary, recovered/byval, and direct-global select-chain sources | Late store-source publication emission route after a prepared plan selects source, destination, producer, and stack-home facts. | `PreparedStoreSourcePublicationPlan`, `PreparedValueHome`, `PreparedEdgePublicationSourceProducerKind`, `PreparedMemoryAccess` | none | `memory.rs` owns source reload, target register choice, and final stack store emission (`operand_to_x0`, `emit_store_to_sp`). | target-emission | none | Reject treating scratch/register ordering or duplicate-emission avoidance as shared semantic authority unless the route starts reselecting source/destination facts. |
| `src/backend/mir/aarch64/codegen/memory.cpp` | `lower_stack_homed_pointer_store_writeback` and `plan_stack_homed_pointer_store_writeback` | Pointer-store writeback intent, destination pointer value, pointer-base home, source value/home, and destination byte offset. | `PreparedStoreSourcePublicationPlan`, `PreparedStoreSourcePublicationIntent::PointerStoreWriteback`, `PreparedMemoryAccess`, `PreparedValueHome` | none | `memory.rs` owns pointer load, offset fold, and store instruction emission (`emit_load_ptr_from_slot_impl`, `emit_typed_store_indirect_impl`). | consume-shared | none | Reject bypassing `plan_prepared_store_source_publication` for stack-homed pointer writeback decisions. |
| `src/backend/mir/aarch64/codegen/memory.cpp` | `prepared_global_symbol_from_value_name`, `emit_pointer_base_plus_offset_to_register`, and `lower_pointer_base_plus_offset_store_local_publication` | Pointer-base-plus-offset source identity, global-symbol base recovery from value spelling, and destination frame-slot publication. | `PreparedValueHomeKind::PointerBasePlusOffset`, `PreparedStoreSourcePublicationPlan`, `PreparedAddressMaterialization`, `PreparedMemoryAccess` | prepared global-symbol pointer-base field/query for pointer-base-plus-offset homes, if value-name spelling is the only current source | `memory.rs` owns GEP/address emission and stack store spelling (`emit_gep_direct_const_impl`, `emit_add_offset_to_addr_reg_impl`). | missing-shared-field | Inspect prepared address-materialization and value-home records before repair to decide where global-symbol base identity should live. | Reject deriving global symbol identity from `@`-prefixed value spelling as the durable authority. |
| `src/backend/mir/aarch64/codegen/memory.cpp` | `lower_pending_store_global_stack_value_publications` and `lower_store_global_value_publication` | Pending store-global stack-home publication, duplicate stack publication state, source value materialization, and future same-block store-global scan. | `PreparedStoreSourcePublicationPlan`, `PreparedStoreSourcePublicationIntent::StoreGlobalPublication`, `PreparedMemoryAccess`, `PreparedEdgePublicationSourceProducerLookups` | prepared store-global publication plan/query with pending/duplicate stack-homes-only state | `memory.rs` owns global store instruction choice and source reload/store emission. | missing-shared-field | Inspect `plan_prepared_store_source_publication` support for `StoreGlobalPublication` before repair; the enum exists but this path does not currently consume a store-global plan. | Reject forward same-block scans for select/global snapshot runs or ad hoc published-stack-value sets as the long-term authority. |

## Suggested Next

Proceed to Step 4 of `plan.md`: audit
`src/backend/mir/aarch64/codegen/alu.cpp`,
`src/backend/mir/aarch64/codegen/calls.cpp`, and
`src/backend/mir/aarch64/codegen/comparison.cpp` against the retained Step 1
baseline and the Step 2/3 findings, preserving the same table column contract
and classification set.

## Watchouts

This remains audit-only. Do not edit implementation files, tests, build
metadata, expectations, `plan.md`, or
`ideas/open/46_aarch64_duplicate_prepared_authority_audit.md` during executor
audit packets unless the supervisor delegates the later durable source-idea
update step.

Do not classify target-local register spelling, scratch ordering, large-offset
materialization, instruction mnemonic choice, condition-code spelling, or ABI
register/stack emission as duplicate authority by itself. The duplicate signal
requires rederiving a semantic prepared fact that should come from the shared
records/helpers named above.

Step 3 adds proof questions for later repair planning, not this audit-only
packet: whether generic scalar materialization/select-chain consumers need a
shared prepared producer query, whether global-load and pointer-base-plus-offset
global symbol identity already fits prepared address materialization, whether
local-address store values need a stack-object/frame-slot lookup by value, and
whether store-global publication should consume the existing
`PreparedStoreSourcePublicationIntent::StoreGlobalPublication` route.

## Proof

No build/test proof was required or run for this audit-only packet. No
`test_after.log` was produced or updated.
