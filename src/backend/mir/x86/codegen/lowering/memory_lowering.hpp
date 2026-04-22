#pragma once

#include "../x86_codegen.hpp"

namespace c4c::backend::x86 {

struct PreparedNamedFloatLoadRender {
  std::string body;
  std::string destination_register;
};

std::string render_prepared_stack_memory_operand(std::size_t byte_offset,
                                                 std::string_view size_name);

std::string render_prepared_stack_address_expr(std::size_t byte_offset);

std::optional<std::string> render_prepared_local_slot_memory_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view slot_name,
    std::size_t stack_byte_bias,
    std::string_view size_name);

std::optional<std::string> render_prepared_value_home_stack_address_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::FunctionNameId function_name,
    std::string_view value_name);

std::optional<std::string> render_prepared_named_stack_address_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::FunctionNameId function_name,
    std::string_view value_name,
    std::size_t stack_byte_bias);

std::optional<std::string> render_prepared_named_payload_stack_address_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::FunctionNameId function_name,
    std::string_view pointer_name,
    std::size_t stack_byte_bias);

std::optional<std::string> render_prepared_named_i32_home_sync_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view value_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<std::string> render_prepared_named_i32_stack_home_sync_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view value_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<std::string> render_prepared_named_ptr_home_sync_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view value_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<std::string> render_prepared_named_qword_load_from_memory_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view value_name,
    std::string_view source_memory,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view scratch_register = "rax");

std::optional<PreparedNamedFloatLoadRender> render_prepared_named_float_load_from_memory_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    c4c::backend::bir::TypeKind type,
    std::string_view value_name,
    std::string_view source_memory,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

}  // namespace c4c::backend::x86
