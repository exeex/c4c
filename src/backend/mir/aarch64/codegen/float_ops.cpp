#include "float_ops.hpp"

#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {
namespace {

[[nodiscard]] PreparedScalarAluRecordResult scalar_float_alu_record_error(
    PreparedScalarAluRecordError error) {
  return PreparedScalarAluRecordResult{.record = std::nullopt, .error = error};
}

[[nodiscard]] std::optional<std::string> scalar_fp_register_name_with_view(
    const RegisterOperand& operand,
    abi::RegisterView view) {
  if (!abi::is_fp_simd_register(operand.reg)) {
    return std::nullopt;
  }
  const auto viewed = abi::fp_simd_register(operand.reg.index, view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*viewed);
}

[[nodiscard]] std::string_view scalar_floating_alu_mnemonic(
    ScalarAluOperationKind operation) {
  switch (operation) {
    case ScalarAluOperationKind::Add:
      return "fadd";
    case ScalarAluOperationKind::Sub:
      return "fsub";
    case ScalarAluOperationKind::Mul:
      return "fmul";
    case ScalarAluOperationKind::Div:
      return "fdiv";
    case ScalarAluOperationKind::And:
    case ScalarAluOperationKind::Or:
    case ScalarAluOperationKind::Xor:
    case ScalarAluOperationKind::LogicalShiftRight:
    case ScalarAluOperationKind::Deferred:
      return {};
  }
  return {};
}

}  // namespace

std::optional<abi::RegisterView> scalar_fp_register_view(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
      return abi::RegisterView::S;
    case bir::TypeKind::F64:
      return abi::RegisterView::D;
    case bir::TypeKind::Void:
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

bool is_scalar_alu_floating_opcode(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
      return true;
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return false;
  }
  return false;
}

bool is_scalar_alu_floating_type(bir::TypeKind type) {
  return type == bir::TypeKind::F32 || type == bir::TypeKind::F64;
}

bool is_prepared_scalar_float_alu_operation(const bir::BinaryInst& binary) {
  return is_scalar_alu_floating_type(binary.operand_type) &&
         is_scalar_alu_floating_type(binary.result.type) &&
         is_scalar_alu_floating_opcode(binary.opcode);
}

ScalarAluPrintResult make_scalar_float_alu_print_lines(
    const ScalarInstructionRecord& scalar) {
  if (!scalar.scalar_alu.has_value() ||
      !scalar.scalar_alu->supported_floating_operation) {
    return {.lines = std::nullopt,
            .diagnostic = "scalar FP node has incomplete printable register facts"};
  }
  const auto& alu = *scalar.scalar_alu;
  if (scalar.inputs.size() != 2) {
    return {.lines = std::nullopt,
            .diagnostic =
                "scalar FP node requires exactly two structured register operands"};
  }
  const auto result_view = scalar_fp_register_view(alu.result_type);
  const auto operand_view = scalar_fp_register_view(alu.operand_type);
  if (!result_view.has_value() || !operand_view.has_value() ||
      result_view != operand_view) {
    return {.lines = std::nullopt,
            .diagnostic =
                "scalar FP node requires matching F32/F64 operand and result widths"};
  }
  const auto* lhs_register = std::get_if<RegisterOperand>(&scalar.inputs[0].payload);
  const auto* rhs_register = std::get_if<RegisterOperand>(&scalar.inputs[1].payload);
  if (scalar.inputs[0].kind != OperandKind::Register ||
      scalar.inputs[1].kind != OperandKind::Register ||
      lhs_register == nullptr || rhs_register == nullptr) {
    return {.lines = std::nullopt,
            .diagnostic =
                "scalar FP node requires structured FP/SIMD register operands"};
  }
  const auto mnemonic = scalar_floating_alu_mnemonic(alu.operation);
  const auto result = scalar.result_register.has_value()
                          ? scalar_fp_register_name_with_view(*scalar.result_register,
                                                              *result_view)
                          : std::optional<std::string>{};
  const auto lhs = scalar_fp_register_name_with_view(*lhs_register, *operand_view);
  const auto rhs = scalar_fp_register_name_with_view(*rhs_register, *operand_view);
  if (mnemonic.empty() || !result.has_value() || !lhs.has_value() || !rhs.has_value()) {
    return {.lines = std::nullopt,
            .diagnostic = "scalar FP node has incomplete printable register facts"};
  }
  std::ostringstream out;
  out << mnemonic << " " << *result << ", " << *lhs << ", " << *rhs;
  return {.lines = std::vector<std::string>{out.str()}, .diagnostic = {}};
}

PreparedScalarAluRecordResult make_prepared_scalar_float_alu_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::BinaryInst& binary) {
  if (value_locations.function_name == c4c::kInvalidFunctionName ||
      storage_plan.function_name != value_locations.function_name) {
    return scalar_float_alu_record_error(PreparedScalarAluRecordError::InvalidFunction);
  }
  if (!is_prepared_scalar_float_alu_operation(binary)) {
    return scalar_float_alu_record_error(PreparedScalarAluRecordError::UnsupportedOpcode);
  }
  if (!scalar_fp_register_view(binary.result.type).has_value() ||
      !scalar_fp_register_view(binary.operand_type).has_value()) {
    return scalar_float_alu_record_error(PreparedScalarAluRecordError::UnsupportedOperandType);
  }

  RegisterOperand result_register;
  if (const auto error = make_prepared_scalar_result_register_operand(
          names, value_locations, storage_plan, binary.result, result_register);
      error != PreparedScalarAluRecordError::None) {
    return scalar_float_alu_record_error(error);
  }

  OperandRecord lhs;
  OperandRecord rhs;
  if (const auto error =
          make_prepared_scalar_operand(names, value_locations, storage_plan, binary.lhs, lhs);
      error != PreparedScalarAluRecordError::None) {
    return scalar_float_alu_record_error(error);
  }
  if (const auto error =
          make_prepared_scalar_operand(names, value_locations, storage_plan, binary.rhs, rhs);
      error != PreparedScalarAluRecordError::None) {
    return scalar_float_alu_record_error(error);
  }

  return PreparedScalarAluRecordResult{
      .record =
          ScalarAluRecord{
              .surface = RecordSurfaceKind::RecordOnly,
              .operation = scalar_alu_operation_from_binary_opcode(binary.opcode),
              .source_binary_opcode = binary.opcode,
              .operand_type = binary.operand_type,
              .result_value_id = result_register.value_id,
              .result_value_name = result_register.value_name,
              .result_type = binary.result.type,
              .result_register = result_register,
              .lhs = lhs,
              .rhs = rhs,
              .supported_floating_operation = true,
          },
      .error = PreparedScalarAluRecordError::None,
  };
}

}  // namespace c4c::backend::aarch64::codegen
