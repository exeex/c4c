# Current Packet

Status: Complete
Source Idea Path: ideas/open/86_aarch64_memory_owner_subresponsibility_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prepare Audit Close Summary

## Just Finished

Completed Step 5, "Prepare Audit Close Summary", by recording a close-ready
audit summary for the memory owner subresponsibility audit. The summary names
the two created follow-up ideas, preserves the intentionally target-local
memory clusters, records rejected candidates with Step 2/Step 3 evidence, and
states that no backend tests were required because this packet changed only
`todo.md`.

### Step 2 Classification Clusters

- `target-local-memory-emission`: `lower_memory_instruction`,
  `lower_f128_transport_instruction`, `lower_i128_transport_instruction`,
  `make_byte_immediate_store_machine_instruction`,
  `make_load_memory_instruction_record`, `make_store_memory_instruction_record`,
  `machine_opcode_from_memory_instruction`, `effect_from_memory_operand`,
  `memory_effects_from_operands`, `memory_side_effects`,
  `memory_selection_status`, `is_supported_memory_base`,
  `is_supported_immediate_store_width`, `is_immediate_store_value`,
  `symbol_memory_has_identity`, `record_memory_result`,
  `retarget_memory_result_to_prepared_home`, `make_bir_machine_instruction`,
  `scalar_load_mnemonic`, `scalar_load_mnemonic_for_width`,
  `scalar_store_mnemonic`, and `fixed_formal_scalar_store_mnemonic`. Stays in
  memory because these helpers choose AArch64 load/store opcodes, mnemonics,
  address support, selected effects, ABI-sensitive result retargeting, and MIR
  machine-record emission.
- `frame-slot-addressing`: `register_indirect_address`,
  `fixed_slots_use_frame_pointer`, `frame_slot_address` overloads,
  `find_frame_slot`, `find_stack_object`, `prepared_frame_slot_load_address`,
  `prepared_local_load_offset`, `make_prepared_frame_slot_memory_operand`,
  `materialize_frame_slot_memory_address_lines`,
  `make_frame_slot_operand_from_stack_slot`,
  `resolve_frame_slot_memory_offset`, `apply_frame_pointer_base_policy`,
  `memory_address`, and `PreparedLocalAddressStoreValue`-related stack-layout
  rewrite helpers. Mostly stays in memory because the boundary includes AArch64
  base-register policy, immediate-offset legality, scratch materialization, and
  textual register spelling. The stable candidate-local-subowner subset is a
  local memory frame-slot/address materialization owner, but only inside the
  AArch64 memory codegen area, not shared BIR/prealloc authority.
- `stack-source-publication`: `emit_same_width_i32_stack_source_publication`,
  `make_load_result_stack_publication_scratch`,
  `find_unique_load_result_stack_source_publication`,
  `dispatch_publication_scalar_type_size_bytes`,
  `prepared_or_emitted_store_value_register`,
  `prepared_store_local_access`, `emit_prepared_pointer_value_load_to_register`,
  `lower_stack_homed_pointer_value_load_publication`,
  `prepared_recovered_narrow_store_source`,
  `prepared_store_source_producer` and its completeness helpers,
  `emit_scalar_conversion_cast_to_register`,
  `plan_store_local_source_publication`,
  `plan_stack_homed_pointer_store_writeback`,
  `plan_pointer_base_plus_offset_store_local_publication`,
  `plan_fixed_formal_store_local_publication`,
  `lower_fixed_formal_store_local_publication`,
  `lower_store_local_value_publication`,
  `lower_stack_homed_pointer_store_writeback`,
  `prepared_global_symbol_from_link_name`,
  `emit_global_symbol_address_to_register`,
  `find_prepared_pointer_base_plus_offset_materialization`,
  `emit_pointer_base_plus_offset_to_register`,
  `lower_pointer_base_plus_offset_store_local_publication`,
  `prepared_store_global_addressing`,
  `find_prepared_store_global_stack_publication_home`,
  `lower_store_global_value_publication_from_plan`,
  `lower_pending_store_global_stack_value_publications`,
  `lower_published_store_global_stack_value_publication`,
  `future_store_local_stack_value_publication_covers_instruction`, and
  `lower_store_global_value_publication`. Intentionally target-local for now:
  evidence is AArch64 scratch choice, scalar conversion emission,
  pointer-base materialization, global-symbol address spelling, and recent
  dispatch/edge-copy contraction history. Candidate-local-subowner boundary:
  a local memory publication planner/emitter could own store-local/store-global
  publication planning plus emission while remaining under AArch64 memory
  codegen; Step 3 must check this against closed ideas 80 and 81 before any
  follow-up idea.
- `store-retargeting`: `store_local_uses_pointer_value_address`,
  `retarget_pointer_store_value_to_materialized_address`,
  `retarget_store_address_to_materialized_pointer`,
  `retarget_pointer_store_value_to_emitted_scalar`,
  `retarget_store_local_value_to_emitted_scalar`,
  `find_prepared_local_address_store_value`,
  `rewrite_local_address_store_value`, and
  `apply_stack_layout_to_memory_record`. Stays in memory unless split as a
  local subowner because the helpers rewrite memory operands around prepared
  frame-slot addresses, pointer-value materialization, and emitted scalar
  registers. Candidate-local-subowner boundary: local store-retargeting helpers
  with no public authority beyond memory-owned record rewrites.
- `identity-validation`: `prepared_named_value_id`,
  `find_storage_plan_value`, `indexed_value_home_id`,
  `indexed_value_home_id_is_ambiguous`, `require_indexed_value_home_id`,
  `named_value_id`, `validate_structured_memory_address_facts`,
  `validate_unstructured_memory_instruction_facts`,
  `validate_memory_instruction_facts`,
  `global_symbol_id_from_address_or_inst`,
  `validate_memory_base_identity`, `apply_load_identity`, and
  `apply_store_identity`. Stays in memory because it compares prepared facts
  against selected memory operands and memory instruction identity. Shared
  relocation is `needs-shared-authority-evidence` until a missing
  target-neutral identity fact is named.
- `prepared-record-construction`: `memory_operand_record_error`,
  `memory_instruction_record_error`,
  `make_memory_record_from_prepared_access` overloads,
  `scalar_fp_register_view`, `register_class_from_bank`,
  `occupied_register_views`, `occupied_register_references`,
  `make_prepared_register_operand`, `make_value_home_register_operand`,
  `make_prepared_pointer_value_base_register`,
  `resolve_pointer_value_base_register`,
  `decode_prepared_load_result_value_storage`,
  `decode_prepared_stored_value_storage`, `prepared_memory_access`,
  `prepared_memory_access_matches_instruction`, `make_memory_operand`,
  `make_memory_instruction`, all `make_prepared_memory_operand_record`
  overloads, all `make_prepared_load_memory_instruction_record` overloads, all
  `make_prepared_store_memory_instruction_record` overloads, and local storage
  structs `PreparedPointerValueBaseRegisterResult`,
  `PreparedLoadResultValueStorage`, `PreparedStoredValueStorageKind`, and
  `PreparedStoredValueStorage`. Stays in memory because these helpers construct
  memory-specific MIR records from prepared AArch64 storage facts and preserve
  the prepared-wrapper contraction boundary from idea 84. Candidate-local
  subowner would need to be an AArch64 memory prepared-record builder, not a
  shared prepared-wrapper resurrection.
- `diagnostics-and-error-spelling`: `append_memory_diagnostic`,
  `memory_base_kind_name`, `memory_operand_support_kind_name`,
  `memory_instruction_kind_name`, `prepared_memory_operand_record_error_name`,
  `memory_error_message`, `f128_transport_error_message`, and
  `i128_transport_error_message`. Stays in memory because spellings are tied to
  memory operand support kinds, prepared-memory errors, and diagnostics emitted
  by the memory lowering entry points.
- `target-local-memory-emission` plus `frame-slot-addressing` special case:
  `parse_va_list_field_suffix`, `scalar_type_size_bytes`,
  `find_va_list_field_address`, `make_va_list_field_memory_operand`,
  `make_va_list_field_load_memory_instruction_record`,
  `make_va_list_field_store_memory_instruction_record`,
  `va_list_field_cursor_update_producer`,
  `make_va_list_field_cursor_update_machine_instruction`, and
  `VaListFieldAddress`. Stays in memory because the cluster combines ABI
  layout knowledge, field-specific address formation, cursor update emission,
  and memory-record construction; no stable non-memory owner boundary is visible.

### Candidate Local Subowners

- Local AArch64 frame-slot/address materialization owner: stable boundary would
  own frame-slot lookup/address formatting, base policy, offset folding, and
  materialization lines for memory operands. It must stay target-local because
  it depends on `sp`/`x29`, AArch64 immediate-offset legality, and scratch
  materialization.
- Local AArch64 memory publication owner: stable boundary would own
  stack-source/store-local/store-global publication planning and emission,
  including pointer-base-plus-offset materialization. It must be checked against
  ideas 80 and 81 before promotion because the visible evidence also supports
  intentional stay-in-memory treatment.
- Local AArch64 store-retargeting owner: stable boundary would own pointer
  store-value/address retargeting and local-address store rewrites. It should
  remain local to memory records and not claim shared dispatch or BIR authority.
- Local AArch64 prepared-record builder owner: possible boundary around
  prepared operand/instruction record construction and prepared storage decode.
  Step 3 must verify this does not undo idea 84's prepared-wrapper contraction.

### Needs Shared Authority Evidence

- Identity validation has no current shared-owner route: moving it out of
  memory would require a named target-neutral identity fact that memory lacks
  today.
- Prepared memory access matching has no current shared-owner route: it is tied
  to BIR memory instructions only after AArch64 memory access selection is
  being prepared.
- Variadic `va_list` memory handling has no separate owner boundary: it mixes
  ABI field decoding, address construction, cursor updates, and memory record
  emission.
- Dispatch/publication helpers must not be moved out solely to reduce
  `memory.cpp`; current evidence says any follow-up must stay under an
  AArch64-memory-local subowner and survive Step 3 closed-idea comparison.

### Public Surface Inventory From `memory.hpp`

- `MemoryInstructionLoweringResult`: public lowering result wrapper carrying `handled` plus an optional machine instruction.
- `PreparedTypedStackSourcePublicationEmission`: public same-width i32 stack-source publication emission payload with destination register and asm lines.
- `memory_base_kind_name`, `memory_operand_support_kind_name`, `memory_instruction_kind_name`: public enum-name spelling helpers for diagnostics/records.
- `prepared_memory_operand_record_error_name`, `memory_error_message`: public prepared-memory error spelling and diagnostic text helpers.
- `memory_address`, `register_indirect_address`, `frame_slot_address` overloads, `fixed_slots_use_frame_pointer`: public AArch64 address string builders and fixed-slot base policy helpers. [prepared address]
- `find_frame_slot`, `find_stack_object`: public prepared stack-layout lookup helpers for frame slots and stack objects.
- `prepared_frame_slot_load_address`, `prepared_local_load_offset`: public prepared-address convenience helpers for local frame-slot load consumers. [prepared address]
- `prepared_memory_access`, `prepared_memory_access_matches_instruction`, `prepared_store_local_access`: public prepared-memory access lookup/matching surfaces. [prepared address]
- `make_prepared_frame_slot_memory_operand`, `materialize_frame_slot_memory_address_lines`: public frame-slot memory operand construction and address materialization helpers. [prepared address]
- `scalar_load_mnemonic`, `dispatch_publication_scalar_type_size_bytes`, `scalar_load_mnemonic_for_width`, `scalar_store_mnemonic`, `fixed_formal_scalar_store_mnemonic`: public AArch64 scalar memory mnemonic/width helpers. [dispatch publication]
- `emit_same_width_i32_stack_source_publication`: public load-result stack-source publication emitter for same-width i32 edge-publication cases. [edge-copy]
- `make_memory_operand`, `make_memory_instruction`: public record-construction wrappers from memory operands/instructions to MIR record surfaces. [prepared-wrapper history]
- `lower_memory_instruction`, `lower_f128_transport_instruction`, `lower_i128_transport_instruction`: public memory-family lowering entry points for scalar memory and wide carrier transport.
- `record_memory_result`, `retarget_memory_result_to_prepared_home`: public scalar-state publication and prepared-home retarget helpers for memory results.
- `store_local_uses_pointer_value_address`, `prepared_or_emitted_store_value_register`, `retarget_pointer_store_value_to_materialized_address`, `retarget_store_address_to_materialized_pointer`, `retarget_pointer_store_value_to_emitted_scalar`, `retarget_store_local_value_to_emitted_scalar`: public store-retargeting helpers for pointer-address/value interactions. [local-helper]
- `plan_fixed_formal_store_local_publication`, `emit_prepared_pointer_value_load_to_register`, `lower_stack_homed_pointer_value_load_publication`, `lower_store_local_value_publication`, `lower_stack_homed_pointer_store_writeback`, `lower_pointer_base_plus_offset_store_local_publication`, `lower_fixed_formal_store_local_publication`: public store/local publication and pointer materialization helpers. [dispatch publication]
- `lower_pending_store_global_stack_value_publications`, `lower_published_store_global_stack_value_publication`, `future_store_local_stack_value_publication_covers_instruction`, `lower_store_global_value_publication`: public store-global publication helpers, including pending stack-home publication tracking. [dispatch publication]
- `make_prepared_memory_operand_record` overloads for `LoadLocalInst`, `StoreLocalInst`, `LoadGlobalInst`, `StoreGlobalInst`: public prepared operand record constructors for BIR memory forms. [prepared-wrapper history]
- `make_prepared_load_memory_instruction_record` overloads for local/global loads and `make_prepared_store_memory_instruction_record` overloads for local/global stores: public prepared instruction record constructors. [prepared-wrapper history]

### `memory.cpp` Function Inventory

- `register_indirect_address` (45): formats `[base, #offset]` register-indirect AArch64 memory syntax.
- `fixed_slots_use_frame_pointer` (57): reads prepared frame-plan policy for fixed-slot base selection.
- `frame_slot_address` overloads (63, 75): formats frame-slot addresses using explicit base or context-selected `sp`/`x29`. [prepared address]
- `find_frame_slot` (82), `find_stack_object` (93): linear lookup helpers over prepared stack layout.
- `prepared_frame_slot_load_address` (104), `prepared_local_load_offset` (143): resolve prepared frame-slot load addressing to textual address/offset. [prepared address]
- `make_prepared_frame_slot_memory_operand` (174): builds a prepared frame-slot `MemoryOperand` from stack-layout facts. [prepared address]
- `materialize_frame_slot_memory_address_lines` (205): emits `add`/`sub` or constant-materialization sequence for frame-slot address into scratch. [prepared address]
- `scalar_load_mnemonic` (230), `scalar_load_mnemonic_for_width` (274), `scalar_store_mnemonic` (330), `fixed_formal_scalar_store_mnemonic` (351): map BIR type/width to AArch64 load/store mnemonics.
- `dispatch_publication_scalar_type_size_bytes` (251): exposes scalar byte width for dispatch publication paths. [dispatch publication]
- `emit_same_width_i32_stack_source_publication` (289): emits stack-source publication load lines for same-width i32 edge publication. [edge-copy]
- Local struct `PreparedLocalAddressStoreValue` (554): carries rewritten store-value operand for local-address frame-slot stores. [prepared address]
- Local struct `PreparedPointerValueBaseRegisterResult` (1320): couples pointer-base register resolution with prepared-record error state.
- Local struct `PreparedLoadResultValueStorage` (1375): decoded load result home/storage plus stack offset/error.
- Local enum `PreparedStoredValueStorageKind` (1423) and struct `PreparedStoredValueStorage` (1430): classify stored values as register, rematerializable immediate, frame slot, or register-home/frame-slot-storage.
- Local struct `VaListFieldAddress` (2455): decoded variadic `va_list` field base register and field layout.
- Local struct `PreparedPointerBasePlusOffsetMaterialization` (4658): pointer-base-plus-offset materialization plan with source kind, names, and offsets.
- Local helper `prepared_named_value_id` (375): maps named BIR values through prepared name tables.
- `instruction_result_value` (387): extracts result values from producer instruction variants for publication scans.
- `memory_operand_record_error` (406), `memory_instruction_record_error` (411): wrap prepared-memory errors into operand/instruction result objects.
- `append_memory_diagnostic` (416): appends memory-family diagnostics with context coordinates.
- `make_bir_machine_instruction` (435): wraps a target instruction record into a MIR machine instruction with BIR origin.
- `make_byte_immediate_store_machine_instruction` (458): emits inline asm for unsupported byte immediate stores through scratch plus `strb`.
- `find_storage_plan_value` (511): finds a prepared storage-plan value by prepared value id.
- `memory_load_result_feeds_before_return_fpr_abi` (522), `symbol_fp_load_has_explicit_storage_placement` (533): detect result-retargeting exceptions around return/FPR ABI placement.
- `make_frame_slot_operand_from_stack_slot` (559), `resolve_frame_slot_memory_offset` (581): convert prepared stack slots into frame-slot operands and fold stack offsets. [prepared address]
- `find_prepared_local_address_store_value` (598), `rewrite_local_address_store_value` (639), `apply_stack_layout_to_memory_record` (648): rewrite local-address store values and apply stack layout to memory records. [prepared address]
- `apply_frame_pointer_base_policy` (674): applies `x29` frame-pointer base policy to memory records and nested source operands.
- `find_memory_return_abi_register` (698), `retarget_load_result_to_return_abi` (749): retarget load result registers for before-return ABI moves.
- `indexed_value_home_id` (766), `indexed_value_home_id_is_ambiguous` (778), `require_indexed_value_home_id` (795), `named_value_id` (818): prepared identity/value-name lookup and ambiguity checks.
- `validate_structured_memory_address_facts` (831), `validate_unstructured_memory_instruction_facts` (856), `validate_memory_instruction_facts` (869): validate BIR/prepared address facts against memory operands.
- `global_symbol_id_from_address_or_inst` (882), `validate_memory_base_identity` (899): validate symbol, frame-slot, pointer-value, and string-constant base identity. [prepared address]
- `apply_load_identity` (994), `apply_store_identity` (1015): attach/validate result or stored value identity on prepared memory operands.
- `make_memory_record_from_prepared_access` overloads (1042, 1114): build `MemoryOperand` from prepared addressing access by direct access or block/index. [prepared-wrapper history]
- `scalar_fp_register_view` (1129), `register_class_from_bank` (1136), `occupied_register_views` (1153), `occupied_register_references` (1220): register view/class/occupancy helpers for prepared record operands.
- `make_prepared_register_operand` (1225), `make_value_home_register_operand` (1270): convert prepared home/storage facts into `RegisterOperand`s.
- `make_prepared_pointer_value_base_register` (1325), `resolve_pointer_value_base_register` (1356): resolve pointer-value memory bases to physical registers. [prepared address]
- `decode_prepared_load_result_value_storage` (1382), `decode_prepared_stored_value_storage` (1439): decode prepared result/stored-value home/storage choices.
- `make_load_result_stack_publication_scratch` (1509): selects scratch/destination for load-result stack-source publication. [edge-copy]
- `find_unique_load_result_stack_source_publication` (1565): finds the unique prepared edge publication for a load result. [edge-copy]
- `machine_opcode_from_memory_instruction` (1591), `effect_from_memory_operand` (1602), `prepared_value_def` (1643), `is_immediate_store_value` (1653), `is_supported_immediate_store_width` (1661), `memory_effects_from_operands` (1666), `memory_side_effects` (1676), `is_supported_memory_base` (1689), `symbol_memory_has_identity` (1703), `memory_selection_status` (1711): derive MIR opcodes, effects, side effects, and selection status from memory records.
- `prepared_memory_access` (1759), `prepared_memory_access_matches_instruction` (1776): lookup/match prepared memory accesses against BIR memory instructions. [prepared address]
- `memory_base_kind_name` (1807), `memory_operand_support_kind_name` (1825), `memory_instruction_kind_name` (1835), `prepared_memory_operand_record_error_name` (1866), `memory_error_message` (1921), `f128_transport_error_message` (1930), `i128_transport_error_message` (1939): enum/error spelling and diagnostics.
- `memory_address` (1846): renders selected frame-slot/register/pointer memory operand addresses.
- `record_memory_result` (1948), `retarget_memory_result_to_prepared_home` (1962): publish emitted memory results and retarget selected load results to prepared homes.
- `make_memory_operand` (2014), `make_memory_instruction` (2018): construct MIR operand/instruction records from memory records. [prepared-wrapper history]
- `make_prepared_memory_operand_record` for local load (2051), local store (2346), global load (2774), global store (2848): validate prepared access/facts/identity and produce memory operands. [prepared-wrapper history]
- `make_load_memory_instruction_record` (2099), `make_store_memory_instruction_record` (2199): build load/store memory instruction records from prepared operands and storage-plan facts. [prepared-wrapper history]
- `make_prepared_load_memory_instruction_record` for local/global load (2173, 2822) and `make_prepared_store_memory_instruction_record` for local/global store (2394, 2896): public wrappers that validate function alignment and compose prepared operand/instruction records. [prepared-wrapper history]
- `parse_va_list_field_suffix` (2414), `scalar_type_size_bytes` (2432), `find_va_list_field_address` (2462), `make_va_list_field_memory_operand` (2543), `make_va_list_field_load_memory_instruction_record` (2568), `make_va_list_field_store_memory_instruction_record` (2602), `va_list_field_cursor_update_producer` (2634), `make_va_list_field_cursor_update_machine_instruction` (2669): variadic `va_list` field load/store/cursor-update special handling.
- `lower_memory_instruction` (2916): main scalar load/store lowering entry point, including prepared-memory prerequisites, va_list special cases, stack-layout application, frame-pointer policy, diagnostics, and selected machine instruction creation.
- `lower_f128_transport_instruction` (3082), `lower_i128_transport_instruction` (3246): wide f128/i128 memory transport lowering using prepared carrier facts and prepared memory records.
- `store_local_uses_pointer_value_address` (3381), `prepared_or_emitted_store_value_register` (3386), `retarget_pointer_store_value_to_materialized_address` (3404), `retarget_store_address_to_materialized_pointer` (3416), `retarget_pointer_store_value_to_emitted_scalar` (3446), `retarget_store_local_value_to_emitted_scalar` (3474): store retargeting helpers for pointer-address stores and emitted scalar reuse. [local-helper]
- `prepared_store_local_access` (3508), `emit_prepared_pointer_value_load_to_register` (3526), `lower_stack_homed_pointer_value_load_publication` (3609): prepared pointer-address lookup/load emission and stack-homed pointer load publication. [dispatch publication]
- `prepared_recovered_narrow_store_source` (3722), `prepared_store_source_producer` (3749), `prepared_store_source_producer_is_complete` (3766), `prepared_store_source_cast_producer_is_complete` (3777), `prepared_store_source_select_producer_is_complete` (3786), `prepared_store_source_scalar_fp_binary_producer_is_complete` (3795), `emit_scalar_conversion_cast_to_register` (3805): store-source producer recovery/completeness checks and scalar conversion emission for publication. [dispatch publication]
- `store_local_destination_frame_slot` (3924), `store_local_destination_stack_object` (3935), `plan_store_local_source_publication` (3944), `plan_stack_homed_pointer_store_writeback` (4024), `plan_pointer_base_plus_offset_store_local_publication` (4039), `plan_fixed_formal_store_local_publication` (4070): store-local publication planning helpers. [dispatch publication]
- `lower_fixed_formal_store_local_publication` (4118), `lower_store_local_value_publication` (4207), `lower_stack_homed_pointer_store_writeback` (4476): store-local/fixed-formal/pointer writeback publication emitters. [dispatch publication]
- `prepared_global_symbol_from_link_name` (4624), `emit_global_symbol_address_to_register` (4637), `find_prepared_pointer_base_plus_offset_materialization` (4664), `emit_pointer_base_plus_offset_to_register` (4694), `lower_pointer_base_plus_offset_store_local_publication` (4761): global-symbol and pointer-base-plus-offset materialization/publication helpers. [dispatch publication]
- `prepared_store_global_addressing` (4877), `find_prepared_store_global_stack_publication_home` (4886): store-global publication lookup helpers. [dispatch publication]
- `lower_store_global_value_publication_from_plan` declaration/definition (4905, 5119), `lower_pending_store_global_stack_value_publications` (4911), `lower_published_store_global_stack_value_publication` (4969), `future_store_local_stack_value_publication_covers_instruction` (5065), `lower_store_global_value_publication` (5259): store-global value/stack-home publication planning and emission path. [dispatch publication]

### Step 3 Closed-Idea Boundary Comparison

- Idea 70, `aarch64_memory_prepared_address_authority_cleanup`, rejects any
  frame-slot/address route that recreates local source-authority, value-home,
  storage-plan, stack-source, or frame-offset decisions. It explicitly keeps
  AArch64 encodable offset checks, base/scratch selection, opcode selection,
  address spelling, and `va_list` field addressing local. Result: the
  frame-slot/address materialization candidate survives only as a
  memory-local AArch64 owner for base policy, offset folding,
  materialization lines, and memory operand address spelling while continuing
  to consume prepared authority. A shared BIR/prealloc relocation is rejected.
- Idea 80, `aarch64_dispatch_publication_owner_relocation`, rejects a
  publication follow-up whose purpose is to move store-local/store-global
  publication helpers back into dispatch publication, value materialization,
  or producer owners. The closure says owner-local helpers were intentionally
  relocated into narrower AArch64 owners and the remaining dispatch
  publication surface is retained only for shared scalar utility and
  current-block publication authority. Result: the Step 2 local AArch64 memory
  publication owner is rejected as an implementation candidate if it would
  reopen dispatch-publication files or publish new dispatch-facing authority.
- Idea 81, `aarch64_dispatch_edge_copy_owner_contraction`, rejects a
  publication follow-up whose purpose is to move typed stack-source or
  prepared-memory edge emission back into `dispatch_edge_copies.*`. The closure
  says narrow prepared-memory and typed stack-source emission helpers were
  relocated to their natural AArch64 owners and retained edge-copy entry points
  remain external edge-copy hooks. Result: same-width stack-source publication
  and load-result stack-source publication helpers should stay in memory; a
  split that recreates an edge-copy owner is rejected.
- Idea 83, `aarch64_local_helper_duplication_tail_cleanup`, rejects promoting
  residual repeated private helper shapes such as storage-plan lookup,
  register-class decoding, prepared register operands, diagnostics, or scratch
  selection to a new public utility without target-neutral proof. Result:
  store-retargeting survives as a possible memory-local private subowner
  because it rewrites memory records around prepared frame-slot/pointer facts;
  any route that extracts generic register/storage/diagnostic utility from
  this audit is rejected.
- Idea 84, `aarch64_prepared_consumer_wrapper_contraction`, rejects
  resurrecting redundant prepared-memory wrapper surfaces or deleting wrappers
  without proving equivalent diagnostics and machine records. Its closure
  retained memory adapters and store-publication helpers that own AArch64
  ABI, addressing, diagnostics, scratch choices, or machine-record
  construction. Result: the prepared-record builder candidate is rejected as a
  follow-up when framed as a standalone prepared-wrapper owner. It remains only
  as a justified memory-local responsibility cluster for constructing
  prepared memory operand and instruction records from already-prepared facts.

### Step 3 Surviving Candidates

- Survives: local AArch64 frame-slot/address materialization owner. Boundary:
  memory-local frame-slot lookup/address formatting, fixed-slot base policy,
  offset folding, and scratch materialization lines. Constraints: must consume
  prepared address/value-home/storage facts, must not move AArch64 legality or
  spelling into shared prealloc, and must not include `va_list` handling.
- Survives: local AArch64 store-retargeting owner. Boundary:
  pointer store-value/address retargeting, local-address store-value rewrites,
  and stack-layout application to memory records. Constraints: private to
  memory lowering, no shared dispatch/BIR authority, no generic helper
  publication for storage or register facts.
- Rejected as Step 4 follow-up: local AArch64 memory publication owner, unless
  the supervisor/plan owner later asks for an internal organization-only
  refactor that does not reopen ideas 80 or 81. Current evidence says the
  growth is intentional owner-local memory lowering: AArch64 scratch choice,
  scalar conversion emission, pointer-base materialization, global-symbol
  address spelling, and memory record emission are all consumer-owned.
- Rejected as Step 4 follow-up: local AArch64 prepared-record builder owner
  when it means recreating the prepared wrapper surface contracted by idea 84.
  Current evidence says the remaining size is justified because memory owns the
  selected memory operand/instruction records, diagnostics, identity
  validation at the selected operand boundary, and machine-record construction.
- Rejected as shared-owner candidates: identity validation, prepared memory
  access matching, and `va_list` memory handling. Closed ideas 70, 83, and 84
  all require concrete missing target-neutral facts before shared relocation;
  Step 2 found none.

### Step 4 Follow-Up Ideas Created

- Created:
  `ideas/open/88_aarch64_memory_frame_slot_address_materialization_owner.md`.
  Scope: a narrow AArch64 memory-local owner for frame-slot lookup/address
  formatting, fixed-slot base policy, offset folding, and materialization
  lines. Proof route: focused frame-slot local load/store, prepared frame-slot
  operand, materialized address emission, and fixed-slot frame-pointer policy
  coverage plus affected backend build proof.
- Created:
  `ideas/open/89_aarch64_memory_store_retargeting_owner.md`. Scope: a narrow
  AArch64 memory-local owner for pointer store-value/address retargeting,
  emitted-scalar retargeting, local-address store-value rewrites, and
  stack-layout application to memory records. Proof route: focused
  pointer-address store, materialized-address, emitted-scalar, local-address,
  and stack-layout rewrite coverage plus affected backend build proof.
- No idea created for the local AArch64 memory publication owner. Step 3 found
  that such a route would reopen ideas 80 and 81 unless separately reframed as
  an internal organization-only cleanup; current evidence treats the helpers as
  intentional owner-local memory lowering.
- No idea created for the prepared-record builder candidate. Step 3 found that
  a standalone owner would recreate the prepared-wrapper surface contracted by
  idea 84; current evidence treats record construction as justified memory
  ownership.
- No idea created for identity validation, prepared memory access matching, or
  `va_list` memory handling. Step 3 found no missing target-neutral fact or
  stable non-memory owner boundary.

### Step 5 Audit Close Summary

Close-ready outcome: the audit satisfied the source idea by producing a
function-level inventory, classifying the major `memory.cpp`/`memory.hpp`
responsibility clusters, comparing candidates against recent closed owner
boundaries, and creating follow-up ideas only for concrete memory-local
subowners with bounded proof routes.

Follow-up ideas created:

- `ideas/open/88_aarch64_memory_frame_slot_address_materialization_owner.md`:
  created for the surviving frame-slot/address materialization boundary. It
  owns only AArch64 memory-local frame-slot lookup/address formatting,
  fixed-slot base policy, offset folding, and scratch materialization lines
  while continuing to consume prepared address/value-home/storage authority.
- `ideas/open/89_aarch64_memory_store_retargeting_owner.md`: created for the
  surviving store-retargeting boundary. It owns only memory-record rewrites
  around pointer store-value/address retargeting, emitted-scalar reuse,
  local-address store-value rewrites, and stack-layout application without
  publishing shared dispatch, BIR, storage, or register authority.

Clusters intentionally staying target-local in memory:

- Scalar and wide memory emission stays in `memory.cpp` because it owns AArch64
  load/store opcode and mnemonic selection, selected memory effects,
  immediate-store support, ABI-sensitive result retargeting, and MIR
  machine-record emission.
- Frame-slot/addressing broadly stays target-local because AArch64 base policy,
  encodable-offset legality, scratch materialization, and register/address
  spelling are target facts. Only the narrow memory-local materialization owner
  was split into a follow-up idea; shared BIR/prealloc relocation was rejected.
- Stack-source, store-local, and store-global publication helpers stay
  target-local because the evidence is AArch64 scratch choice, scalar
  conversion emission, pointer-base materialization, global-symbol address
  spelling, and memory record emission. Ideas 80 and 81 make this current
  memory ownership intentional rather than a regression.
- Store-retargeting stays memory-local; the follow-up idea is only a private
  AArch64 memory subowner for record rewrites and does not create shared
  storage/register/dispatch authority.
- Identity validation and prepared memory access matching stay in memory
  because the audit found no missing target-neutral identity fact or stable
  non-memory owner boundary.
- Prepared-record construction stays in memory because it constructs selected
  memory operands/instructions, diagnostics, identity attachments, and machine
  records from already-prepared facts; extracting a standalone prepared-record
  builder would recreate the wrapper surface contracted by idea 84.
- Diagnostics and error spelling stay in memory because the spellings are tied
  to memory operand support kinds, prepared-memory errors, and diagnostics
  emitted by memory lowering entry points.
- Variadic `va_list` memory handling stays in memory because it combines ABI
  field decoding, field address construction, cursor update emission, and
  memory-record construction with no stable separate owner.

Rejected candidates and evidence:

- Local AArch64 memory publication owner was rejected as a Step 4 follow-up
  because it would reopen dispatch publication and edge-copy owner decisions
  from ideas 80 and 81 unless later reframed as internal organization-only
  cleanup. Current evidence says the helpers are consumer-owned memory
  lowering.
- Prepared-record builder extraction was rejected because idea 84 already
  contracted redundant prepared wrapper surfaces; remaining prepared record
  construction owns selected memory records and diagnostics at the memory
  boundary.
- Shared identity-validation and prepared-memory-access matching routes were
  rejected because Step 2 found no missing target-neutral fact to move into a
  shared owner.
- `va_list` field handling was rejected because the cluster mixes ABI layout,
  address construction, cursor updates, and memory-record emission.
- Generic storage/register/diagnostic helper publication was rejected by idea
  83 because the audit found target-local helper shapes, not target-neutral
  authority.

Backend tests: none required. Step 5 was audit-only and changed no
implementation files; this packet changed only `todo.md`.

## Suggested Next

Supervisor should route this complete audit state to the plan owner for
lifecycle close/deactivation decision. No implementation packet is pending
under this runbook.

## Watchouts

- This was audit-only; implementation files remain untouched.
- Ideas 80 and 81 make publication growth in `memory.cpp` intentional
  owner-local memory lowering, not a regression by itself.
- Idea 84 blocks prepared-record-builder extraction when the route recreates
  redundant prepared wrapper layers.
- Idea 70 allows a frame-slot/address materialization split only if it keeps
  consuming prepared authority and keeps AArch64 legality/spelling local.
- Idea 83 allows memory-local helper organization but blocks publishing generic
  storage/register/diagnostic helper authority without target-neutral proof.
- Do not turn line-count reduction or vague shared-authority speculation into
  implementation scope when routing the follow-up ideas.
- Step 3 found no new shared-owner candidate; surviving candidates are
  target-local and memory-local follow-up ideas.

## Proof

Step 5 proof command:
`printf 'Audit-only Step 5; no backend tests required.\n' > test_after.log && git diff --name-only >> test_after.log && if git diff --name-only | rg -q '^src/backend/mir/aarch64/codegen/memory\.(cpp|hpp)$|^plan\.md$|^ideas/'; then printf 'ERROR: non-todo file changed during audit close-summary packet.\n' >> test_after.log; exit 1; fi`

Expected proof scope: audit-only `todo.md` update. No backend tests were
required because no implementation files changed.
