#pragma once

#include "../../../bir/bir.hpp"
#include "../../../prealloc/module.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::riscv::codegen {

struct RiscvEncodedFragment;

[[nodiscard]] bool fits_signed_12_bit_load_offset(std::size_t offset_bytes);
[[nodiscard]] bool fits_signed_12_bit_immediate(std::int64_t value);

[[nodiscard]] std::size_t align_riscv_stack_frame(std::size_t size_bytes);
[[nodiscard]] std::size_t align_riscv_stack_slot(std::size_t offset_bytes,
                                                 std::size_t align_bytes);

[[nodiscard]] std::optional<std::size_t> prepared_saved_register_stack_end(
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan);

[[nodiscard]] bool rv64_prepared_supported_fixed_frame_alignment(
    std::size_t alignment);

[[nodiscard]] std::optional<std::size_t>
align_rv64_prepared_object_stack_frame_size(std::size_t frame_size);

[[nodiscard]] const c4c::backend::prepare::PreparedFrameSlot*
rv64_prepared_find_function_frame_slot(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    c4c::backend::prepare::PreparedFrameSlotId slot_id,
    c4c::FunctionNameId function_name);

[[nodiscard]] bool rv64_prepared_frame_slot_extent_is_supported(
    const c4c::backend::prepare::PreparedFrameSlot& slot,
    std::size_t frame_size);

[[nodiscard]] std::optional<std::size_t>
rv64_prepared_validated_fixed_frame_size(
    const c4c::backend::prepare::PreparedAddressingFunction* addressing,
    const c4c::backend::prepare::PreparedFramePlanFunction& frame_plan,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout);

[[nodiscard]] std::optional<std::size_t>
rv64_prepared_validated_dynamic_saved_gpr_frame_size(
    const c4c::backend::prepare::PreparedAddressingFunction* addressing,
    const c4c::backend::prepare::PreparedFramePlanFunction& frame_plan,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout);

[[nodiscard]] std::optional<std::size_t> rv64_prepared_object_stack_frame_size(
    const c4c::backend::prepare::PreparedAddressingFunction* addressing,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout);

[[nodiscard]] std::optional<std::size_t> rv64_prepared_call_frame_size(
    std::size_t local_frame_bytes);

[[nodiscard]] std::optional<std::size_t> rv64_prepared_call_frame_ra_offset(
    std::size_t local_frame_bytes);

[[nodiscard]] bool rv64_prepared_is_callee_saved_gpr_register_name(
    std::string_view name);

[[nodiscard]] bool rv64_prepared_is_callee_saved_fpr_register_name(
    std::string_view name);

[[nodiscard]] std::optional<std::int32_t>
rv64_prepared_saved_callee_gpr_stack_offset(
    const c4c::backend::prepare::PreparedSavedRegister& saved,
    std::size_t stack_frame_bytes);

[[nodiscard]] std::optional<std::int32_t>
rv64_prepared_saved_callee_fpr_stack_offset(
    const c4c::backend::prepare::PreparedSavedRegister& saved,
    std::size_t stack_frame_bytes);

[[nodiscard]] std::optional<std::size_t>
rv64_prepared_stack_slot_home_absolute_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedValueHome& home,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes = 4);

[[nodiscard]] std::optional<std::int32_t> rv64_prepared_stack_slot_home_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedValueHome& home,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes = 4);

[[nodiscard]] std::optional<std::uint32_t> rv64_prepared_register_number(
    std::string_view name);

[[nodiscard]] std::optional<std::uint32_t>
rv64_prepared_gpr_register_number_for_home(
    const c4c::backend::prepare::PreparedValueHome& home);

[[nodiscard]] std::optional<std::uint32_t>
rv64_prepared_load_store_funct3_for_size(std::size_t size_bytes);

void append_rv64_prepared_move(RiscvEncodedFragment& fragment,
                                std::uint32_t destination,
                                std::uint32_t source);

void append_rv64_prepared_add_registers(RiscvEncodedFragment& fragment,
                                         std::uint32_t destination,
                                         std::uint32_t lhs,
                                         std::uint32_t rhs);

void append_rv64_prepared_load_immediate(RiscvEncodedFragment& fragment,
                                          std::uint32_t destination,
                                          std::int64_t immediate);

[[nodiscard]] bool append_rv64_prepared_store_register_to_stack(
    RiscvEncodedFragment& fragment,
    std::uint32_t source_register,
    std::int32_t offset,
    std::size_t size_bytes = 4);

[[nodiscard]] bool append_rv64_prepared_load_stack_to_register(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination_register,
    std::int32_t offset,
    std::size_t size_bytes = 4);

[[nodiscard]] bool append_rv64_prepared_store_register_to_stack_offset(
    RiscvEncodedFragment& fragment,
    std::uint32_t source_register,
    std::size_t offset,
    std::size_t size_bytes);

[[nodiscard]] bool append_rv64_prepared_load_stack_offset_to_register(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination_register,
    std::size_t offset,
    std::size_t size_bytes);

[[nodiscard]] bool append_rv64_prepared_store_fpr_to_stack_offset(
    RiscvEncodedFragment& fragment,
    std::uint32_t source_register,
    std::int32_t offset);

[[nodiscard]] bool append_rv64_prepared_load_stack_offset_to_fpr(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination_register,
    std::int32_t offset);

[[nodiscard]] bool append_rv64_prepared_stack_pointer_adjustment(
    RiscvEncodedFragment& fragment,
    std::int64_t byte_delta);

[[nodiscard]] bool append_rv64_prepared_saved_callee_gpr_spills(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t stack_frame_bytes);

[[nodiscard]] bool append_rv64_prepared_saved_callee_gpr_restores(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t stack_frame_bytes);

[[nodiscard]] std::optional<RiscvEncodedFragment>
make_rv64_prepared_call_frame_prologue_fragment(
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t local_frame_bytes);

[[nodiscard]] std::optional<RiscvEncodedFragment>
make_rv64_prepared_stack_frame_prologue_fragment(
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t stack_frame_bytes);

[[nodiscard]] bool append_rv64_prepared_call_frame_epilogue(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t local_frame_bytes);

[[nodiscard]] bool append_rv64_prepared_stack_frame_epilogue(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t stack_frame_bytes);

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
