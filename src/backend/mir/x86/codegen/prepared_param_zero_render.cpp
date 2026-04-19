#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

namespace {

std::optional<std::string> render_prepared_param_zero_branch_prefix(
    PreparedParamZeroCompareShape compare_shape,
    std::string_view function_name,
    std::string_view false_label,
    const char* false_branch_opcode,
    std::string_view param_register_name) {
  if (false_branch_opcode == nullptr || false_label.empty()) {
    return std::nullopt;
  }

  std::string rendered_compare;
  switch (compare_shape) {
    case PreparedParamZeroCompareShape::SelfTest:
      rendered_compare = "    test " + std::string(param_register_name) + ", " +
                         std::string(param_register_name) + "\n";
      break;
  }

  return rendered_compare + "    " + std::string(false_branch_opcode) + " .L" +
         std::string(function_name) + "_" + std::string(false_label) + "\n";
}

}  // namespace

std::optional<std::string> render_prepared_param_zero_branch_function(
    std::string_view asm_prefix,
    std::string_view function_name,
    PreparedParamZeroCompareShape compare_shape,
    std::string_view false_label,
    const char* false_branch_opcode,
    std::string_view param_register_name,
    std::string_view true_body,
    std::string_view false_body,
    std::string_view trailing_data) {
  const auto rendered_branch_prefix =
      render_prepared_param_zero_branch_prefix(compare_shape,
                                               function_name,
                                               false_label,
                                               false_branch_opcode,
                                               param_register_name);
  if (!rendered_branch_prefix.has_value()) {
    return std::nullopt;
  }

  const std::string rendered_false_label =
      ".L" + std::string(function_name) + "_" + std::string(false_label);
  return std::string(asm_prefix) + *rendered_branch_prefix + std::string(true_body) +
         rendered_false_label + ":\n" + std::string(false_body) + std::string(trailing_data);
}

std::optional<std::string> render_prepared_supported_immediate_binary(
    std::string_view return_register,
    const c4c::backend::prepare::PreparedSupportedImmediateBinary& binary) {
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Add) {
    return "    add " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Sub) {
    return "    sub " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Mul) {
    return "    imul " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::And) {
    return "    and " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Or) {
    return "    or " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Xor) {
    return "    xor " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Shl) {
    return "    shl " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::LShr) {
    return "    shr " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::AShr) {
    return "    sar " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  return std::nullopt;
}

std::optional<std::string> render_prepared_materialized_compare_join_value_if_supported(
    std::string_view return_register,
    const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm&
        prepared_return_arm,
    const c4c::backend::bir::Param& param,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<bool(const c4c::backend::bir::Global&,
                             c4c::backend::bir::TypeKind,
                             std::size_t)>& same_module_global_supports_scalar_load) {
  const auto& computed_value = prepared_return_arm.arm.context.selected_value;
  std::string rendered;
  switch (computed_value.base.kind) {
    case c4c::backend::prepare::PreparedComputedBaseKind::ImmediateI32:
      rendered = "    mov " + std::string(return_register) + ", " +
                 std::to_string(static_cast<std::int32_t>(computed_value.base.immediate)) +
                 "\n";
      break;
    case c4c::backend::prepare::PreparedComputedBaseKind::ParamValue: {
      if (computed_value.base.param_name != param.name) {
        return std::nullopt;
      }
      const auto param_register = minimal_param_register(param);
      if (!param_register.has_value()) {
        return std::nullopt;
      }
      rendered = "    mov " + std::string(return_register) + ", " + *param_register + "\n";
      break;
    }
    case c4c::backend::prepare::PreparedComputedBaseKind::GlobalI32Load: {
      if (prepared_return_arm.global == nullptr ||
          !same_module_global_supports_scalar_load(
              *prepared_return_arm.global,
              c4c::backend::bir::TypeKind::I32,
              computed_value.base.global_byte_offset)) {
        return std::nullopt;
      }
      rendered = "    mov " + std::string(return_register) + ", DWORD PTR [rip + " +
                 render_asm_symbol_name(prepared_return_arm.global->name);
      if (computed_value.base.global_byte_offset != 0) {
        rendered += " + " + std::to_string(computed_value.base.global_byte_offset);
      }
      rendered += "]\n";
      break;
    }
    case c4c::backend::prepare::PreparedComputedBaseKind::PointerBackedGlobalI32Load: {
      if (prepared_return_arm.pointer_root_global == nullptr ||
          prepared_return_arm.pointer_root_global->type != c4c::backend::bir::TypeKind::Ptr ||
          prepared_return_arm.global == nullptr ||
          !same_module_global_supports_scalar_load(
              *prepared_return_arm.global,
              c4c::backend::bir::TypeKind::I32,
              computed_value.base.global_byte_offset)) {
        return std::nullopt;
      }
      rendered = "    mov rax, QWORD PTR [rip + " +
                 render_asm_symbol_name(prepared_return_arm.pointer_root_global->name) +
                 "]\n    mov " + std::string(return_register) + ", DWORD PTR [rip + " +
                 render_asm_symbol_name(prepared_return_arm.global->name);
      if (computed_value.base.global_byte_offset != 0) {
        rendered += " + " + std::to_string(computed_value.base.global_byte_offset);
      }
      rendered += "]\n";
      break;
    }
  }

  for (const auto& operation : computed_value.operations) {
    const auto operation_render =
        render_prepared_supported_immediate_binary(return_register, operation);
    if (!operation_render.has_value()) {
      return std::nullopt;
    }
    rendered += *operation_render;
  }
  return rendered;
}

std::optional<std::string> render_prepared_materialized_compare_join_return_if_supported(
    std::string_view return_register,
    const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm&
        prepared_return_arm,
    const c4c::backend::bir::Param& param,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<bool(const c4c::backend::bir::Global&,
                             c4c::backend::bir::TypeKind,
                             std::size_t)>& same_module_global_supports_scalar_load) {
  const auto value_render = render_prepared_materialized_compare_join_value_if_supported(
      return_register,
      prepared_return_arm,
      param,
      minimal_param_register,
      render_asm_symbol_name,
      same_module_global_supports_scalar_load);
  if (!value_render.has_value()) {
    return std::nullopt;
  }

  const auto binary_plan =
      c4c::backend::prepare::find_prepared_materialized_compare_join_return_binary_plan(
          prepared_return_arm.arm);
  if (!binary_plan.has_value()) {
    return std::nullopt;
  }
  if (!binary_plan->trailing_binary.has_value()) {
    return render_prepared_return_body(*value_render);
  }

  const auto trailing_render =
      render_prepared_supported_immediate_binary(return_register, *binary_plan->trailing_binary);
  if (!trailing_render.has_value()) {
    return std::nullopt;
  }
  return render_prepared_return_body(*value_render, *trailing_render);
}

std::string render_prepared_return_body(std::string_view value_render,
                                        std::string_view trailing_render) {
  return std::string(value_render) + std::string(trailing_render) + "    ret\n";
}

}  // namespace c4c::backend::x86
