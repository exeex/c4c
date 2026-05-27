Status: Active
Source Idea Path: ideas/open/46_aarch64_duplicate_prepared_authority_audit.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Shared Authority And Reference Responsibilities

# Current Packet

## Just Finished

Step 1 - Inventory Shared Authority And Reference Responsibilities is complete
for the AArch64 duplicate-authority audit.

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

## Suggested Next

Proceed to Step 2 of `plan.md`: audit
`src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` and
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp` against this
baseline, filling the table columns above with concrete helper names and
classifications.

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

## Proof

No build/test proof was required or run for this audit-only packet. No
`test_after.log` was produced or updated.
