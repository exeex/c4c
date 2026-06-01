#pragma once

#include "instruction.hpp"
#include "alu.hpp"
#include "../module/module.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace c4c::backend::aarch64::codegen {

struct MemoryInstructionLoweringResult {
  bool handled = false;
  std::optional<module::MachineInstruction> instruction;
};

[[nodiscard]] std::string_view memory_base_kind_name(MemoryBaseKind kind);
[[nodiscard]] std::string_view memory_operand_support_kind_name(MemoryOperandSupportKind kind);
[[nodiscard]] std::string_view memory_instruction_kind_name(MemoryInstructionKind kind);
[[nodiscard]] std::string_view prepared_memory_operand_record_error_name(
    PreparedMemoryOperandRecordError error);
[[nodiscard]] std::string memory_address(const MemoryOperand& address);
[[nodiscard]] std::string memory_error_message(PreparedMemoryOperandRecordError error);
[[nodiscard]] std::string register_indirect_address(std::string_view base,
                                                    std::size_t byte_offset);
[[nodiscard]] bool fixed_slots_use_frame_pointer(
    const module::FunctionLoweringContext& context);
[[nodiscard]] std::string frame_slot_address(std::size_t offset_bytes,
                                             std::string_view base_register = "sp");
[[nodiscard]] std::string frame_slot_address(
    const module::FunctionLoweringContext& context,
    std::size_t offset_bytes);
[[nodiscard]] std::optional<MemoryOperand> make_prepared_frame_slot_memory_operand(
    c4c::FunctionNameId function_name,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    prepare::PreparedFrameSlotId slot_id,
    std::size_t stack_offset_bytes,
    std::size_t size_bytes,
    std::size_t align_bytes,
    bool uses_frame_pointer_base = false);
[[nodiscard]] std::vector<std::string> materialize_frame_slot_memory_address_lines(
    abi::RegisterReference scratch,
    const MemoryOperand& address);
[[nodiscard]] std::optional<std::string_view> scalar_load_mnemonic(bir::TypeKind type);
[[nodiscard]] std::optional<std::size_t> dispatch_publication_scalar_type_size_bytes(
    bir::TypeKind type);
[[nodiscard]] std::optional<std::string_view> scalar_load_mnemonic_for_width(
    std::size_t width_bytes);
[[nodiscard]] std::optional<std::string_view> scalar_store_mnemonic(bir::TypeKind type);

[[nodiscard]] OperandRecord make_memory_operand(MemoryOperand operand);
[[nodiscard]] InstructionRecord make_memory_instruction(MemoryInstructionRecord instruction);
[[nodiscard]] MemoryInstructionLoweringResult lower_memory_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] MemoryInstructionLoweringResult lower_f128_transport_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] MemoryInstructionLoweringResult lower_i128_transport_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);

void record_memory_result(BlockScalarLoweringState& scalar_state,
                          const module::MachineInstruction& instruction);

void retarget_memory_result_to_prepared_home(
    const module::BlockLoweringContext& context,
    module::MachineInstruction& instruction);

[[nodiscard]] bool store_local_uses_pointer_value_address(
    const bir::StoreLocalInst& store);

[[nodiscard]] std::optional<RegisterOperand> prepared_or_emitted_store_value_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state);

[[nodiscard]] bool emit_prepared_pointer_value_load_to_register(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load_local,
    std::size_t instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_stack_homed_pointer_value_load_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_store_local_value_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const module::MachineInstruction& lowered_memory,
    const BlockScalarLoweringState& scalar_state,
    const module::MachineBlock& block);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_stack_homed_pointer_store_writeback(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_pointer_base_plus_offset_store_local_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index);

void lower_pending_store_global_stack_value_publications(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::unordered_set<c4c::ValueNameId>& published_stack_values,
    module::MachineBlock& block);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_published_store_global_stack_value_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const std::unordered_set<c4c::ValueNameId>& published_stack_values);

[[nodiscard]] bool future_store_local_stack_value_publication_covers_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_store_global_value_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const module::MachineInstruction& lowered_memory,
    std::unordered_set<c4c::ValueNameId>* published_stack_values = nullptr,
    bool stack_homes_only = false);

[[nodiscard]] PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadLocalInst& load);
[[nodiscard]] PreparedMemoryInstructionRecordResult
make_prepared_frame_slot_load_memory_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadLocalInst& load);
[[nodiscard]] PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreLocalInst& store);
[[nodiscard]] PreparedMemoryInstructionRecordResult
make_prepared_store_memory_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreLocalInst& store);
[[nodiscard]] PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadGlobalInst& load);
[[nodiscard]] PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreGlobalInst& store);
[[nodiscard]] PreparedMemoryInstructionRecordResult
make_prepared_store_memory_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreGlobalInst& store);

}  // namespace c4c::backend::aarch64::codegen
