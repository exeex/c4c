#pragma once

#include "../x86_codegen.hpp"

namespace c4c::backend::x86 {

struct PreparedCurrentFloatCarrier {
  std::string_view value_name;
  std::string register_name;
};

struct PreparedDirectExternNamedI32Source {
  std::optional<std::string> register_name;
  std::optional<std::string> stack_operand;
  std::optional<std::int64_t> immediate_i32;
};

struct PreparedDirectExternCurrentI32Carrier {
  std::string_view value_name;
  std::optional<std::string> register_name;
  std::optional<std::string> stack_operand;
};

std::optional<std::string> choose_prepared_float_scratch_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<std::string> render_prepared_named_float_source_into_register_if_supported(
    c4c::backend::bir::TypeKind type,
    std::string_view value_name,
    std::string_view destination_register,
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

bool append_prepared_float_home_sync_if_supported(
    std::string* body,
    c4c::backend::bir::TypeKind type,
    std::string_view source_register,
    std::string_view value_name,
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<std::string> render_prepared_aggregate_slice_root_home_memory_operand_if_supported(
    c4c::backend::bir::TypeKind type,
    std::string_view slot_name,
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<std::string> render_prepared_named_f128_source_memory_operand_if_supported(
    std::string_view value_name,
    std::size_t stack_byte_bias,
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::bir::Function* function,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<std::string> render_prepared_named_f128_copy_into_memory_if_supported(
    std::string_view value_name,
    std::string_view destination_memory_operand,
    std::size_t stack_byte_bias,
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::bir::Function* function,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<PreparedDirectExternNamedI32Source>
select_prepared_direct_extern_named_i32_source_if_supported(
    std::string_view value_name,
    const std::optional<PreparedDirectExternCurrentI32Carrier>& current_i32,
    const std::unordered_map<std::string_view, std::int64_t>& i32_constants,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

bool append_prepared_direct_extern_call_argument_if_supported(
    const c4c::backend::bir::Value& arg,
    c4c::backend::bir::TypeKind arg_type,
    std::size_t arg_index,
    std::size_t instruction_index,
    const std::optional<PreparedDirectExternCurrentI32Carrier>& current_i32,
    const std::unordered_map<std::string_view, std::int64_t>& i32_constants,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>&
        find_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    std::unordered_set<std::string_view>* emitted_string_names,
    std::vector<const c4c::backend::bir::StringConstant*>* used_string_constants,
    std::unordered_set<std::string_view>* used_same_module_globals,
    std::string* body);

bool finalize_prepared_direct_extern_call_result_if_supported(
    const c4c::backend::bir::CallInst& call,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string* body,
    std::optional<PreparedDirectExternCurrentI32Carrier>* current_i32);

bool finalize_prepared_direct_extern_return_if_supported(
    const c4c::backend::bir::Value& returned,
    std::string_view return_register,
    const std::optional<PreparedDirectExternCurrentI32Carrier>& current_i32,
    const std::function<std::optional<std::int64_t>(const c4c::backend::bir::Value&)>&
        resolve_i32_constant,
    std::string* body);

}  // namespace c4c::backend::x86
