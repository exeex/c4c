#pragma once

#include "../x86_codegen.hpp"

namespace c4c::backend::x86 {

struct PreparedCurrentFloatCarrier {
  std::string_view value_name;
  std::string register_name;
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

}  // namespace c4c::backend::x86
