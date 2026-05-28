#pragma once

#include "alu.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

[[nodiscard]] bool registers_alias(const RegisterOperand& lhs,
                                   const RegisterOperand& rhs);

[[nodiscard]] std::optional<unsigned> integer_bit_width(bir::TypeKind type);

[[nodiscard]] std::optional<unsigned> power_of_two_shift(std::uint64_t value);

[[nodiscard]] const prepare::PreparedFrameSlot* find_frame_slot(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedFrameSlotId slot_id);

[[nodiscard]] const prepare::PreparedStackObject* find_stack_object(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedObjectId object_id);

[[nodiscard]] std::optional<std::string> prepared_frame_slot_load_address(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);

[[nodiscard]] std::string relocation_operand(std::string_view label,
                                             std::size_t byte_offset);

[[nodiscard]] std::string register_indirect_address(std::string_view base,
                                                    std::size_t byte_offset);

[[nodiscard]] bool fixed_slots_use_frame_pointer(
    const module::FunctionLoweringContext& context);

[[nodiscard]] std::string frame_slot_address(std::size_t offset_bytes,
                                             std::string_view base_register = "sp");

[[nodiscard]] std::string frame_slot_address(
    const module::FunctionLoweringContext& context,
    std::size_t offset_bytes);

[[nodiscard]] std::optional<abi::RegisterView> scalar_view_for_type(
    bir::TypeKind type);

[[nodiscard]] std::optional<std::string> gp_register_name(std::uint8_t index,
                                                          abi::RegisterView view);

[[nodiscard]] std::optional<std::string_view> scalar_load_mnemonic(bir::TypeKind type);

[[nodiscard]] std::optional<std::size_t> dispatch_publication_scalar_type_size_bytes(
    bir::TypeKind type);

[[nodiscard]] std::optional<std::string_view> scalar_load_mnemonic_for_width(
    std::size_t width_bytes);

[[nodiscard]] std::optional<std::string_view> scalar_store_mnemonic(bir::TypeKind type);

[[nodiscard]] std::optional<unsigned> scalar_integer_width_bits(bir::TypeKind type);

[[nodiscard]] std::optional<abi::RegisterReference> scalar_gp_register_view(
    abi::RegisterReference reg,
    bir::TypeKind type);

[[nodiscard]] std::optional<abi::RegisterReference> scalar_fp_register_view(
    abi::RegisterReference reg,
    bir::TypeKind type);

[[nodiscard]] std::uint64_t immediate_integer_bits(const bir::Value& value,
                                                   unsigned width_bits);

[[nodiscard]] bool is_byval_formal_value_name(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name);

[[nodiscard]] std::optional<std::size_t> prepared_local_load_offset(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);

[[nodiscard]] std::optional<std::size_t> publication_parse_va_list_field_suffix(
    std::string_view base,
    std::string_view slot_name);

[[nodiscard]] std::optional<std::string> prepared_va_list_field_address(
    const module::BlockLoweringContext& context,
    std::string_view slot_name);

[[nodiscard]] std::optional<std::size_t> local_slot_address_frame_offset(
    const module::BlockLoweringContext& context,
    std::string_view local_slot_name);

[[nodiscard]] std::optional<std::size_t> local_aggregate_address_frame_offset(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name);

[[nodiscard]] bool emit_local_slot_address_publication_to_register(
    const module::BlockLoweringContext& context,
    const bir::BinaryInst& binary,
    std::uint8_t target_index,
    std::optional<std::size_t> before_or_at_instruction_index,
    std::vector<std::string>& lines);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_local_slot_address_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] bool register_operands_share_physical_register(
    const RegisterOperand& lhs,
    const RegisterOperand& rhs);

[[nodiscard]] const prepare::PreparedValueHome* prepared_value_home_for_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value);

[[nodiscard]] bool value_has_current_block_entry_publication(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& home);

[[nodiscard]] std::optional<RegisterOperand> current_block_entry_publication_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    abi::RegisterView expected_view);

void record_current_block_entry_publication_registers(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_missing_conditional_branch_condition_publication(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_missing_fused_compare_operand_publication(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics,
    std::optional<std::uint8_t> preferred_target_index = std::nullopt);

[[nodiscard]] std::vector<module::MachineInstruction>
lower_missing_fused_compare_operand_publications(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics);

void record_address_materialization_result(
    BlockScalarLoweringState& scalar_state,
    const module::MachineInstruction& instruction);

void record_memory_result(BlockScalarLoweringState& scalar_state,
                          const module::MachineInstruction& instruction);

void retarget_memory_result_to_prepared_home(
    const module::BlockLoweringContext& context,
    module::MachineInstruction& instruction);

void retarget_pointer_store_value_to_materialized_address(
    module::MachineInstruction& instruction,
    const RegisterOperand& materialized_address);

void retarget_store_address_to_materialized_pointer(
    const bir::StoreLocalInst& store,
    module::MachineInstruction& instruction,
    const RegisterOperand& materialized_address);

void retarget_pointer_store_value_to_emitted_scalar(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    module::MachineInstruction& instruction);

void retarget_store_local_value_to_emitted_scalar(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    module::MachineInstruction& instruction);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_fixed_formal_store_local_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const BlockScalarLoweringState& scalar_state);

[[nodiscard]] bool block_entry_move_clobbers_current_join_publication(
    const module::BlockLoweringContext& context,
    const module::MachineInstruction& instruction);

[[nodiscard]] bool prepared_value_home_reads_register_index(
    const prepare::PreparedValueHome& home,
    std::uint8_t register_index);

[[nodiscard]] bool value_publication_may_read_register_index(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t register_index,
    unsigned depth = 0);

[[nodiscard]] std::optional<bir::Value> instruction_result_value(
    const bir::Inst& inst);

[[nodiscard]] const bir::Value* instruction_result_value_ref(const bir::Inst& inst);

}  // namespace c4c::backend::aarch64::codegen
