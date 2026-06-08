Status: Active
Source Idea Path: ideas/open/124_aarch64_memory_post_contract_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Build the evidence inventory

# Current Packet

## Just Finished

Step 1 - Build the evidence inventory is complete. Built the read-only
inventory for `src/backend/mir/aarch64/codegen/memory.cpp` using
`c4c-clang-tool-ccdb function-signatures` plus targeted raw reads, and checked
closed-route notes for ideas 34, 39, 39a, 50, 70, 86, 88, 89, 111, 114, 116,
117, 122, and 123.

Function/helper cluster map for `memory.cpp`:

- Lines 46-198, public surface and basic AArch64 memory spelling:
  `register_indirect_address`, scalar load/store mnemonic helpers,
  `dispatch_publication_scalar_type_size_bytes`,
  `emit_same_width_i32_stack_source_publication`, and fixed-formal store
  mnemonic selection. Classification cue: target-local instruction spelling
  and stack-source publication emission around prepared facts.
- Lines 203-419, diagnostics, machine-record wrapping, byte-immediate store,
  and scalar store offset legality: `prepared_named_value_id`,
  `instruction_result_value`, memory record error wrappers,
  `append_memory_diagnostic`, `make_bir_machine_instruction`,
  `scalar_store_width_bytes`, `scalar_frame_slot_store_offset_is_encodable`,
  `append_scalar_store_to_memory`, and
  `make_byte_immediate_store_machine_instruction`. Classification cue:
  diagnostics/error spelling plus AArch64 offset/scratch/emission policy.
- Lines 421-555, prepared storage and before-return retargeting:
  `find_storage_plan_value`,
  `memory_load_result_feeds_before_return_fpr_abi`,
  `symbol_fp_load_has_explicit_storage_placement`,
  `apply_frame_pointer_base_policy`, `find_memory_return_abi_register`, and
  `retarget_load_result_to_return_abi`. Classification cue: consume prepared
  move/storage/frame policy while retaining ABI register conversion locally.
- Lines 556-918, prepared memory identity and operand construction:
  indexed value-home lookup helpers, memory fact validators,
  `global_symbol_id_from_address_or_inst`,
  `validate_memory_base_identity`, `apply_load_identity`,
  `apply_store_identity`, and both `make_memory_record_from_prepared_access`
  overloads. Classification cue: prepared-record construction and identity
  validation; watch for accidental raw name/value-home rediscovery.
- Lines 919-1379, register/value-home decoding and stack-source scratch
  selection: register class/view helpers, occupied-register helpers,
  `make_prepared_register_operand`, `make_value_home_register_operand`,
  pointer-value base-register resolution, prepared load-result/stored-value
  storage decoding, load-result stack publication scratch selection, and
  unique load-result stack-source publication lookup. Classification cue:
  prepared value-home/storage consumption with target-local register views and
  scratch choice.
- Lines 1381-1538, machine node metadata and selection status:
  memory opcode/effect/side-effect construction, immediate-store support,
  base support, symbol identity checks, and `memory_selection_status`.
  Classification cue: selected machine-record construction and fail-closed
  status/diagnostic rules.
- Lines 1549-1839, public prepared-access queries, names, addresses,
  diagnostics, result recording, and result retargeting:
  `prepared_memory_access`, `prepared_memory_access_matches_instruction`,
  name-format helpers, `memory_address`, error-name/message helpers,
  f128/i128 transport error messages, `record_memory_result`,
  `retarget_memory_result_to_prepared_home`, `make_memory_operand`, and
  `make_memory_instruction`. Classification cue: diagnostics, record
  construction, and prepared-home result retargeting.
- Lines 1841-2199 and 2564-2699, prepared scalar load/store record builders
  for local/global memory: overloads for load-local, store-local, load-global,
  and store-global, plus shared `make_load_memory_instruction_record` and
  `make_store_memory_instruction_record`. Classification cue: consume
  `PreparedMemoryAccess`, value homes, storage plan, addressing, and stack
  publication facts before local instruction selection.
- Lines 2204-2559, variadic `va_list` memory field handling:
  field suffix parsing, `find_va_list_field_address`,
  `make_va_list_field_memory_operand`, va-list field load/store records,
  cursor-update producer detection, and cursor-update machine instruction
  emission. Classification cue: ABI/variadic-specific memory consumer;
  historical evidence says this stayed outside the frame-slot owner.
- Lines 2706-2869, main scalar memory lowering:
  `lower_memory_instruction` routes BIR loads/stores through prepared
  addressing/storage, va-list special handling, stack-layout retargeting,
  return-ABI retargeting, frame-pointer policy, byte-immediate special
  emission, and final machine-record construction. Classification cue:
  central target-local memory lowering consumer.
- Lines 2872-3181, f128/i128 memory-backed transport lowering:
  `lower_f128_transport_instruction` and `lower_i128_transport_instruction`.
  Classification cue: memory-backed transport consumers using prepared carrier
  facts and prepared memory operands; not a generic memory-authority owner.
- Lines 3184-3526, pointer-value load publication:
  `prepared_store_local_access`,
  `emit_prepared_pointer_value_load_to_register`, and
  `lower_stack_homed_pointer_value_load_publication`. Classification cue:
  prepared pointer/value-home consumer plus AArch64 materialization lines;
  watch byval and variadic exclusions.
- Lines 3530-3939, store-source publication planning glue:
  `prepared_recovered_narrow_store_source`, `prepared_store_source_producer`,
  source-producer completeness checks, direct/global load-local helpers,
  scalar conversion cast emission, destination frame-slot/object helpers,
  `plan_store_local_source_publication`,
  `plan_stack_homed_pointer_store_writeback`, and
  `plan_pointer_base_plus_offset_store_local_publication`. Classification cue:
  consumes shared store-source facts, but this is the highest boundary-risk
  cluster because it still assembles the target-side publication plan inputs.
- Lines 3949-4084, fixed-formal store-local publication:
  `plan_fixed_formal_store_local_publication` and
  `lower_fixed_formal_store_local_publication`. Classification cue: shared
  formal/store-source planning plus target-local scalar store emission.
- Lines 4086-4424, store-local value publication emission:
  `lower_store_local_value_publication` emits around prepared store-source
  producers, recovered sources, direct-global select-chain facts, emitted
  scalar state, scratch selection, and stack-home publication. Classification
  cue: target-local publication emission that must remain a consumer of shared
  producer/select-chain facts.
- Lines 4427-4573, stack-homed pointer store writeback:
  `lower_stack_homed_pointer_store_writeback`. Classification cue: prepared
  pointer-store writeback plan consumption plus AArch64 scratch/address/store
  emission.
- Lines 4575-4854, pointer-base-plus-offset materialization and publication:
  global-symbol address emission,
  `find_prepared_pointer_base_plus_offset_materialization`,
  `emit_pointer_base_plus_offset_to_register`, and
  `lower_pointer_base_plus_offset_store_local_publication`. Classification
  cue: consumes prepared pointer-base/value-home/frame-address facts while
  owning AArch64 address materialization lines.
- Lines 4858-5267, store-global publication and pending stack publication:
  `prepared_store_global_addressing`,
  `find_prepared_store_global_stack_publication_home`,
  `lower_pending_store_global_stack_value_publications`,
  `lower_published_store_global_stack_value_publication`,
  `future_store_local_stack_value_publication_covers_instruction`,
  `lower_store_global_value_publication_from_plan`, and
  `lower_store_global_value_publication`. Classification cue: prepared
  store-global publication consumption plus one live local scan for pending
  producer instruction recovery that should be classified against idea 50/111
  evidence before any implementation route.

Closed-route evidence for the next classification step:

- Idea 34: Store-source semantic source selection had to move out of AArch64
  and into shared prepared/MIR/BIR planning; AArch64 must consume prepared
  store-source facts and fail closed when missing. Rejects same-block producer
  rediscovery, name/slot/value-id matching, and expectation downgrades.
- Idea 39: Mechanical fold-back is already closed; former
  `memory_store_sources.*` residue now lives under `memory.cpp`, but only after
  semantic source-choice blockers were resolved. Use this as evidence against
  any new monolithic shrink route or renewed semantic rediscovery in
  `memory.cpp`.
- Idea 39a: The original blocked helpers were same-block producer recovery,
  narrow-store recovery, byval classification, select-chain/global-load
  inspection, and pointer-base load-local recovery. Closure says recovered
  narrow-store, byval load-local, direct-global select-chain, and pointer-base
  authority now flow through shared prepared helpers or fail closed.
- Idea 50: Memory authority repair removed duplicate value-home, move-bundle,
  stack-object/name, pointer-base, store-source, and store-global publication
  recovery from `memory.cpp`. Use it to classify manual value-home scans, raw
  move scans, text-name matching, and local pending/duplicate publication state
  as boundary risks unless they are now thin prepared-query consumers.
- Idea 70: Prepared address authority cleanup established that memory lowering
  should consume prepared addressing, memory access, address materialization,
  frame offsets, stack-source authority, typed stack-source publication, value
  homes, and storage plans, while keeping AArch64 offset legality, scratch
  selection, opcode choice, and address spelling local.
- Idea 86: Prior memory owner audit classified the large owner and created
  only two local-owner follow-ups, frame-slot address materialization and
  store retargeting. It explicitly kept load/store emission, scalar opcode
  selection, operand support, frame-slot base/offset policy, stack-source
  emission, store retargeting around memory records, identity validation,
  prepared record construction, diagnostics, and variadic field memory
  handling target-local.
- Idea 88: Frame-slot/address materialization split is closed. Memory should
  consume the narrow owner for frame-slot lookup, fixed-slot base policy,
  offset folding, address formatting, and materialization lines; it must not
  re-derive value homes, storage, stack-source authority, or frame offsets.
- Idea 89: Store-retargeting split is closed. Pointer store-value/address
  retargeting, materialized-address retargeting, emitted-scalar reuse,
  local-address rewrites, and stack-layout application are AArch64
  memory-local record rewrite responsibilities, not shared dispatch/BIR/storage
  authority.
- Idea 111: Store-source publication dump visibility made
  `PreparedStoreSourcePublicationPlan` facts reviewable, including
  source-producer fields and direct-global select-chain fields. Use this to
  require current store-source publication helpers to consume visible prepared
  plan facts rather than derive producer/select-chain state locally.
- Idea 114: Outgoing stack argument area is a shared prepared call-plan
  contract; AArch64 owns physical stack adjustment/restoration, `x16` scratch
  base, concrete store ordering, and target ABI placement. Any memory/call
  stack publication classification should preserve that split.
- Idea 116: Dispatch producer/current-block routing is a shared prepared
  contract now; remaining AArch64 code should be emission glue and machine
  register hazard policy. Memory publication paths that call dispatch producer
  helpers must not rebuild edge/current-block/select-chain routing.
- Idea 117: Fused/materialized compare publication facts are shared prepared
  authority; AArch64 comparison keeps compare/branch spelling local. Memory
  should treat current-block publication consumption as shared evidence when
  interacting with emitted scalar or publication state.
- Idea 122: Call-argument producer materializability, publication-source
  routing, direct-global select-chain dependency, and missing frame-slot
  argument publication needs are shared prepared facts. AArch64 keeps scalar
  instruction emission, register retargeting, select-chain assembler lines,
  aggregate address payload emission, and machine wrapping local.
- Idea 123: Call-result late publication, source-register publication,
  source-in-destination aliasing, FPR/VREG store-value retargeting, and
  current-block publication consumption are shared prepared facts. AArch64
  remains the target-local emission and recording consumer; memory should not
  duplicate comparison/dispatch/current-block producer authority.

## Suggested Next

Execute Step 2 classification against this inventory. Start with the
highest-risk clusters: store-source publication planning/emission
(3530-4424), pointer-base-plus-offset publication (4575-4854), and
store-global pending/publication handling (4858-5267). Classify each as
target-local emission, prepared consumer, candidate local subowner, or
move-forward/shared-authority risk using the closed-route evidence above.

## Watchouts

- Do not edit implementation files, tests, build metadata, or `ideas/closed/*`.
- The cluster map is an evidence inventory, not an implementation plan.
- Treat physical splitting as local clarity only when the helper is already a
  prepared consumer; otherwise classify the missing shared fact first.
- Reject any route that reintroduces same-block producer scans,
  text/name/slot/value-id matching, or local direct-global select-chain
  discovery as semantic authority.
- The pending store-global cluster still has a producer-instruction search in
  `lower_pending_store_global_stack_value_publications`; Step 2 should classify
  it explicitly against idea 50 and idea 111 rather than assuming it is safe.

## Proof

No build/test proof required; analysis-only evidence inventory.
