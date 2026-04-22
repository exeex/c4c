#pragma once

#include "../x86_codegen.hpp"

namespace c4c::backend::x86 {

struct PreparedNamedHomeSelection {
  std::optional<std::string> register_name;
  std::optional<std::size_t> frame_offset;
};

std::optional<std::size_t> find_prepared_authoritative_value_stack_offset_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::FunctionNameId function_name,
    std::string_view value_name);

std::optional<std::size_t> find_prepared_value_home_frame_offset(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view value_name);

std::optional<PreparedNamedHomeSelection> resolve_prepared_named_home_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view value_name);

std::optional<std::size_t> resolve_prepared_local_slot_base_offset_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view slot_name);

std::optional<std::size_t> resolve_prepared_named_payload_frame_offset_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    c4c::FunctionNameId function_name,
    std::string_view pointer_name);

std::optional<std::string> render_prepared_named_stack_memory_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view value_name,
    std::string_view size_name,
    std::size_t stack_byte_bias = 0);

std::optional<PreparedModuleLocalSlotLayout> build_prepared_module_local_slot_layout(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::TargetArch prepared_arch);

std::optional<PreparedModuleLocalSlotLayout> build_prepared_module_local_slot_layout(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::TargetArch prepared_arch);

}  // namespace c4c::backend::x86
