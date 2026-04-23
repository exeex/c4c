#pragma once

#include "../x86_codegen.hpp"

namespace c4c::backend::x86 {

std::optional<std::string> render_prepared_constant_folded_single_block_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register);

std::optional<std::string> render_prepared_constant_folded_single_block_return_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_minimal_immediate_or_param_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register);

std::optional<std::string> render_prepared_minimal_immediate_or_param_return_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_minimal_local_slot_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix);

std::optional<std::string> render_prepared_minimal_local_slot_return_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_local_i16_i64_sub_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix);

std::optional<std::string> render_prepared_local_i16_i64_sub_return_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_minimal_direct_extern_call_sequence_if_supported(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register,
    const std::unordered_set<std::string_view>& bounded_same_module_helper_global_names,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>& find_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data,
    const std::function<std::string(std::string)>& prepend_bounded_same_module_helpers);

std::optional<std::string> render_prepared_minimal_direct_extern_call_sequence_if_supported(
    const PreparedX86FunctionDispatchContext& context);

}  // namespace c4c::backend::x86
