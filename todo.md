# Current Packet

Status: Active
Source Idea Path: ideas/open/86_aarch64_memory_owner_subresponsibility_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Build The Memory Function Inventory

## Just Finished

Completed Step 1, "Build The Memory Function Inventory", by using
`c4c-clang-tool-ccdb function-signatures` / `list-symbols` on
`src/backend/mir/aarch64/codegen/memory.cpp`, cross-checking
`src/backend/mir/aarch64/codegen/memory.hpp`, and targeted text reads for
helper responsibilities the AST cannot summarize.

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

## Suggested Next

Proceed to Step 2 by grouping the inventoried functions into the plan's
responsibility categories and deciding which clusters are intentionally
target-local versus candidate local subowners.

## Watchouts

- This is audit-only; do not edit implementation files.
- Do not reopen dispatch publication or edge-copy relocation from ideas 80 and
  81.
- Do not turn line-count reduction or vague shared-authority speculation into
  implementation scope.
- Obvious later comparison markers are present above as `[dispatch publication]`,
  `[edge-copy]`, `[prepared address]`, `[local-helper]`, and
  `[prepared-wrapper history]`.
- The header AST query produced useful declarations but hit an include-path
  error on `ast.hpp`, so the public surface was cross-checked by targeted
  `memory.hpp` text read.

## Proof

Ran the delegated audit-only proof command:
`printf 'Audit-only Step 1; no backend tests required.\n' > test_after.log && git diff --name-only >> test_after.log && if git diff --name-only | rg -q '^src/backend/mir/aarch64/codegen/memory\.(cpp|hpp)$'; then printf 'ERROR: implementation memory files changed during audit-only packet.\n' >> test_after.log; exit 1; fi`

Result: passed. `test_after.log` contains the audit-only note and only
`todo.md` in the diff list, confirming `memory.cpp` and `memory.hpp` were not
modified.
