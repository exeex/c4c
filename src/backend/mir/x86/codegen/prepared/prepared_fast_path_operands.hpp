#pragma once

#include "prepared_query_context.hpp"

#include "../../../../prealloc/prealloc.hpp"

namespace c4c::backend::x86 {

struct PreparedCurrentFloatCarrier {
  std::string_view value_name;
  std::string register_name;
};

struct PreparedI32ValueSelection {
  std::optional<std::int32_t> immediate;
  std::optional<std::string> operand;
  bool in_eax = false;
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

struct PreparedBoundedMultiDefinedCurrentI32Carrier {
  std::string_view value_name;
  std::optional<std::string> register_name;
  std::optional<std::string> stack_operand;
};

struct PreparedBoundedMultiDefinedNamedI32Source {
  std::optional<std::string> register_name;
  std::optional<std::string> stack_operand;
  std::optional<std::int64_t> immediate_i32;
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

std::optional<PreparedBoundedMultiDefinedNamedI32Source>
select_prepared_bounded_multi_defined_named_i32_source_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const std::optional<PreparedBoundedMultiDefinedCurrentI32Carrier>& current_i32,
    std::string_view value_name);

bool append_prepared_bounded_multi_defined_i32_move_into_register_if_supported(
    std::string* body,
    std::string_view destination_register,
    const PreparedBoundedMultiDefinedNamedI32Source& source);

bool append_prepared_bounded_multi_defined_i32_move_into_memory_if_supported(
    std::string* body,
    std::string_view destination_memory,
    const PreparedBoundedMultiDefinedNamedI32Source& source);

bool finalize_prepared_bounded_multi_defined_call_result_if_supported(
    const c4c::backend::bir::CallInst& call,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    std::string* body,
    std::optional<PreparedBoundedMultiDefinedCurrentI32Carrier>* current_i32);

bool append_prepared_bounded_multi_defined_call_argument_if_supported(
    const c4c::backend::bir::Value& arg,
    c4c::backend::bir::TypeKind arg_type,
    std::size_t arg_index,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::FunctionNameId function_name,
    const PreparedModuleLocalSlotLayout& local_layout,
    const std::optional<PreparedBoundedMultiDefinedCurrentI32Carrier>& current_i32,
    const std::function<bool(std::string_view)>& has_string_constant,
    const std::function<bool(std::string_view)>& has_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    std::vector<std::string>* used_string_names,
    std::vector<std::string>* used_same_module_global_names,
    std::string* body);

template <class ResolveNamedOperand>
std::optional<PreparedI32ValueSelection> select_prepared_i32_value_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_i32_name,
    ResolveNamedOperand&& resolve_named_operand) {
  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return PreparedI32ValueSelection{
        .immediate = static_cast<std::int32_t>(value.immediate),
        .operand = std::nullopt,
        .in_eax = false,
    };
  }
  if (value.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  if (current_i32_name.has_value() && value.name == *current_i32_name) {
    return PreparedI32ValueSelection{
        .immediate = std::nullopt,
        .operand = std::nullopt,
        .in_eax = true,
    };
  }
  const auto operand = resolve_named_operand(value.name);
  if (!operand.has_value()) {
    return std::nullopt;
  }
  return PreparedI32ValueSelection{
      .immediate = std::nullopt,
      .operand = std::move(*operand),
      .in_eax = false,
  };
}

template <class ResolveNamedOperand>
std::optional<std::string> render_prepared_i32_operand_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_i32_name,
    ResolveNamedOperand&& resolve_named_operand) {
  const auto selection = select_prepared_i32_value_if_supported(
      value, current_i32_name, std::forward<ResolveNamedOperand>(resolve_named_operand));
  if (!selection.has_value()) {
    return std::nullopt;
  }
  if (selection->immediate.has_value()) {
    return std::to_string(*selection->immediate);
  }
  if (selection->in_eax) {
    return std::string("eax");
  }
  if (!selection->operand.has_value()) {
    return std::nullopt;
  }
  return *selection->operand;
}

template <class ResolveNamedOperand>
std::optional<std::string> render_prepared_i32_move_to_register_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_i32_name,
    std::string_view target_register,
    ResolveNamedOperand&& resolve_named_operand) {
  const auto operand = render_prepared_i32_operand_if_supported(
      value, current_i32_name, std::forward<ResolveNamedOperand>(resolve_named_operand));
  if (!operand.has_value()) {
    return std::nullopt;
  }
  if (*operand == target_register) {
    return std::string{};
  }
  return "    mov " + std::string(target_register) + ", " + *operand + "\n";
}

template <class ResolveNamedOperand>
std::optional<std::string> render_prepared_i32_setup_in_eax_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_i32_name,
    ResolveNamedOperand&& resolve_named_operand) {
  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return "    mov eax, " + std::to_string(static_cast<std::int32_t>(value.immediate)) + "\n";
  }
  return render_prepared_i32_move_to_register_if_supported(
      value, current_i32_name, "eax", std::forward<ResolveNamedOperand>(resolve_named_operand));
}

}  // namespace c4c::backend::x86
