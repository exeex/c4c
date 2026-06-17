#pragma once

#include "../../../bir/bir.hpp"
#include "../../../prealloc/module.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::riscv::codegen {

[[nodiscard]] bool fits_signed_12_bit_load_offset(std::size_t offset_bytes);
[[nodiscard]] bool fits_signed_12_bit_immediate(std::int64_t value);

[[nodiscard]] std::size_t align_riscv_stack_frame(std::size_t size_bytes);
[[nodiscard]] std::size_t align_riscv_stack_slot(std::size_t offset_bytes,
                                                 std::size_t align_bytes);

[[nodiscard]] std::optional<std::size_t> prepared_saved_register_stack_end(
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan);

[[nodiscard]] std::string riscv_local_block_label(std::string_view function_name,
                                                  std::string_view block_label);

[[nodiscard]] std::string bir_block_label_spelling(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Block& block);

[[nodiscard]] std::string bir_target_label_spelling(
    const c4c::backend::bir::Module& module,
    c4c::BlockLabelId label_id,
    std::string_view fallback);

[[nodiscard]] std::optional<std::int64_t> simple_frame_slot_sp_offset_for(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedMemoryAccess& access);

[[nodiscard]] std::optional<std::int64_t> simple_frame_slot_sp_offset_for(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedAddressMaterialization& materialization);

[[nodiscard]] std::optional<std::string> emit_i32_load_from_stack_offset(
    std::string_view destination_register,
    std::int64_t stack_offset);

[[nodiscard]] std::optional<std::string> emit_i32_store_to_stack_offset(
    std::string_view source_register,
    std::int64_t stack_offset);

}  // namespace c4c::backend::riscv::codegen
