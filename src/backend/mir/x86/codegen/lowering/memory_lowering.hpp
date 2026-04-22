#pragma once

#include "../x86_codegen.hpp"

namespace c4c::backend::x86 {

struct PreparedNamedI32Source {
  std::optional<std::string> register_name;
  std::optional<std::string> stack_operand;
  std::optional<std::int64_t> immediate_i32;
};

struct PreparedNamedFloatLoadRender {
  std::string body;
  std::string destination_register;
};

struct PreparedI32BinaryRender {
  std::string body;
  bool rhs_now_in_ecx = false;
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

std::optional<PreparedNamedI32Source> select_prepared_named_i32_source_if_supported(
    std::string_view value_name,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& previous_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<PreparedNamedI32Source> select_prepared_i32_source_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& previous_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<PreparedNamedI32Source> select_prepared_named_i32_block_source_if_supported(
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::bir::Block* block,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label_id,
    std::string_view value_name,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& previous_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<PreparedNamedI32Source> select_prepared_i32_block_source_if_supported(
    const c4c::backend::bir::Value& value,
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::bir::Block* block,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label_id,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& previous_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<std::string> render_prepared_named_i32_block_operand_if_supported(
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::bir::Block* block,
    std::size_t instruction_index,
    std::string_view value_name,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label_id,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& previous_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<std::string> render_prepared_named_i32_operand_if_supported(
    std::string_view value_name,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& previous_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<std::string> render_prepared_i32_operand_from_source_if_supported(
    const PreparedNamedI32Source& source);

std::optional<PreparedI32BinaryRender> render_prepared_i32_binary_from_sources_in_eax_if_supported(
    c4c::backend::bir::BinaryOpcode opcode,
    PreparedNamedI32Source lhs_source,
    PreparedNamedI32Source rhs_source,
    std::optional<std::string_view> authoritative_lhs_value_name,
    std::optional<std::string_view> rhs_value_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<std::string> render_prepared_i32_select_from_sources_if_supported(
    const PreparedNamedI32Source& compared_source,
    std::int64_t compared_immediate,
    const PreparedNamedI32Source& true_source,
    const PreparedNamedI32Source& false_source,
    std::string_view false_label,
    std::string_view done_label);

bool append_prepared_named_i32_move_into_register_if_supported(
    std::string* body,
    std::string_view destination_register,
    const PreparedNamedI32Source& source);

bool append_prepared_named_i32_home_sync_if_supported(
    std::string* body,
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view value_name,
    const PreparedNamedI32Source& source,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<std::string> finish_prepared_named_i32_load_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view value_name,
    std::string rendered_load,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<std::string> publish_prepared_named_i32_result_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view value_name,
    std::string rendered_value,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view> published_previous_i32_name,
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
