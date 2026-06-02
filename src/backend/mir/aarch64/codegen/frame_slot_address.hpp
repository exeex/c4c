#pragma once

#include "instruction.hpp"
#include "../abi/abi.hpp"
#include "../module/module.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] bool fixed_slots_use_frame_pointer(
    const module::FunctionLoweringContext& context);
[[nodiscard]] std::string frame_slot_address(std::size_t offset_bytes,
                                             std::string_view base_register = "sp");
[[nodiscard]] std::string frame_slot_address(
    const module::FunctionLoweringContext& context,
    std::size_t offset_bytes);
[[nodiscard]] std::vector<std::string> materialize_frame_slot_memory_address_lines(
    abi::RegisterReference scratch,
    const MemoryOperand& address);
[[nodiscard]] const prepare::PreparedFrameSlot* find_frame_slot(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedFrameSlotId slot_id);
[[nodiscard]] const prepare::PreparedStackObject* find_stack_object(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedObjectId object_id);
[[nodiscard]] std::optional<std::string> prepared_frame_slot_load_address(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);
[[nodiscard]] std::optional<std::size_t> prepared_local_load_offset(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);
[[nodiscard]] std::optional<MemoryOperand> make_prepared_frame_slot_memory_operand(
    c4c::FunctionNameId function_name,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    prepare::PreparedFrameSlotId slot_id,
    std::size_t stack_offset_bytes,
    std::size_t size_bytes,
    std::size_t align_bytes,
    bool uses_frame_pointer_base = false);

}  // namespace c4c::backend::aarch64::codegen
