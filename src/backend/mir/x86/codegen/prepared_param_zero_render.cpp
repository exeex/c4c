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

std::string render_prepared_return_body(std::string_view value_render,
                                        std::string_view trailing_render) {
  return std::string(value_render) + std::string(trailing_render) + "    ret\n";
}

}  // namespace c4c::backend::x86
