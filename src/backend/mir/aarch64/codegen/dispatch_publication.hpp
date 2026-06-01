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

[[nodiscard]] std::optional<abi::RegisterView> scalar_view_for_type(
    bir::TypeKind type);

[[nodiscard]] std::optional<std::string> gp_register_name(std::uint8_t index,
                                                          abi::RegisterView view);

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

}  // namespace c4c::backend::aarch64::codegen
