#pragma once

#include "alu.hpp"
#include "../abi/abi.hpp"
#include "../module/module.hpp"

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

[[nodiscard]] std::optional<std::size_t> dispatch_publication_scalar_type_size_bytes(bir::TypeKind type);

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

}  // namespace c4c::backend::aarch64::codegen
