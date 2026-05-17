#include "cast_ops.hpp"

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace mir = c4c::backend::mir;
namespace abi = c4c::backend::aarch64::abi;

namespace {

[[nodiscard]] mir::TargetInstructionPrintResult target_unsupported(std::string diagnostic) {
  return mir::target_instruction_unsupported(std::move(diagnostic));
}

[[nodiscard]] mir::TargetInstructionPrintResult target_printed(
    std::vector<std::string> lines) {
  return mir::target_instruction_lines_printed(std::move(lines));
}

[[nodiscard]] std::string diagnostic(std::string_view prefix, std::string_view message) {
  std::string out{prefix};
  out += message;
  return out;
}

[[nodiscard]] std::optional<unsigned> integer_type_bit_width(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
      return 1U;
    case bir::TypeKind::I8:
      return 8U;
    case bir::TypeKind::I16:
      return 16U;
    case bir::TypeKind::I32:
      return 32U;
    case bir::TypeKind::I64:
      return 64U;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string> register_name_with_view(
    const RegisterOperand& operand,
    abi::RegisterView view) {
  if (!abi::is_gp_register(operand.reg)) {
    return std::nullopt;
  }
  const auto viewed = abi::gp_register(operand.reg.index, view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*viewed);
}

[[nodiscard]] std::optional<std::string> fp_register_name_with_view(
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

[[nodiscard]] std::optional<abi::RegisterView> integer_register_view(unsigned bit_width) {
  if (bit_width <= 32U) {
    return abi::RegisterView::W;
  }
  if (bit_width == 64U) {
    return abi::RegisterView::X;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<abi::RegisterView> floating_register_view(
    bir::TypeKind type) {
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

[[nodiscard]] const prepare::PreparedValueHome* find_named_value_home(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named) {
    return nullptr;
  }
  return prepare::find_prepared_value_home(names, value_locations, value.name);
}

[[nodiscard]] const prepare::PreparedStoragePlanValue* find_storage_plan_value(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    prepare::PreparedValueId value_id) {
  for (const auto& value : storage_plan.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_fp_register_view(
    bir::TypeKind type) {
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

[[nodiscard]] std::optional<abi::RegisterView> scalar_storage_register_view(
    bir::TypeKind type) {
  if (const auto integer_view = scalar_register_view(type)) {
    return integer_view;
  }
  return scalar_fp_register_view(type);
}

[[nodiscard]] prepare::PreparedRegisterClass register_class_from_bank(
    prepare::PreparedRegisterBank bank) {
  switch (bank) {
    case prepare::PreparedRegisterBank::Gpr:
      return prepare::PreparedRegisterClass::General;
    case prepare::PreparedRegisterBank::Fpr:
      return prepare::PreparedRegisterClass::Float;
    case prepare::PreparedRegisterBank::Vreg:
      return prepare::PreparedRegisterClass::Vector;
    case prepare::PreparedRegisterBank::AggregateAddress:
      return prepare::PreparedRegisterClass::AggregateAddress;
    case prepare::PreparedRegisterBank::None:
      return prepare::PreparedRegisterClass::None;
  }
  return prepare::PreparedRegisterClass::None;
}

[[nodiscard]] std::string_view register_display_name(abi::RegisterReference reg) {
  static constexpr std::string_view x_names[] = {
      "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
      "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
      "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
      "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"};
  static constexpr std::string_view w_names[] = {
      "w0",  "w1",  "w2",  "w3",  "w4",  "w5",  "w6",  "w7",
      "w8",  "w9",  "w10", "w11", "w12", "w13", "w14", "w15",
      "w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23",
      "w24", "w25", "w26", "w27", "w28", "w29", "w30", "w31"};
  static constexpr std::string_view s_names[] = {
      "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
      "s8",  "s9",  "s10", "s11", "s12", "s13", "s14", "s15",
      "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
      "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31"};
  static constexpr std::string_view d_names[] = {
      "d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
      "d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
      "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
      "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"};
  if (reg.index >= 32U) {
    return {};
  }
  switch (reg.view) {
    case abi::RegisterView::X:
      return x_names[reg.index];
    case abi::RegisterView::W:
      return w_names[reg.index];
    case abi::RegisterView::S:
      return s_names[reg.index];
    case abi::RegisterView::D:
      return d_names[reg.index];
    case abi::RegisterView::V:
    case abi::RegisterView::Q:
    case abi::RegisterView::Sp:
      return {};
  }
  return {};
}

[[nodiscard]] std::optional<RegisterOperand> make_prepared_register_operand(
    const prepare::PreparedValueHome& home,
    const prepare::PreparedStoragePlanValue& storage,
    bir::TypeKind type,
    RegisterOperandRole role) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      storage.encoding != prepare::PreparedStorageEncodingKind::Register) {
    return std::nullopt;
  }

  const auto expected_view = scalar_storage_register_view(type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  const auto prepared_class = register_class_from_bank(storage.bank);
  abi::PreparedRegisterConversionResult converted;
  if (storage.register_placement.has_value()) {
    converted = abi::convert_prepared_register(
        *storage.register_placement, prepared_class, expected_view);
  } else {
    if (!home.register_name.has_value() || !storage.register_name.has_value() ||
        *home.register_name != *storage.register_name) {
      return std::nullopt;
    }
    converted = abi::convert_prepared_register(
        *storage.register_name, storage.bank, prepared_class, expected_view);
  }
  if (!converted.has_value()) {
    return std::nullopt;
  }

  const auto display_name = register_display_name(*converted.reg);
  if (display_name.empty()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = role,
      .value_id = home.value_id,
      .value_name = home.value_name,
      .prepared_class = prepared_class,
      .prepared_bank = storage.bank,
      .expected_view = expected_view,
      .contiguous_width = storage.contiguous_width,
      .occupied_register_references = {*converted.reg},
      .occupied_registers = {display_name},
  };
}

[[nodiscard]] PreparedScalarCastRecordResult scalar_cast_record_error(
    PreparedScalarCastRecordError error) {
  return PreparedScalarCastRecordResult{.record = std::nullopt, .error = error};
}

[[nodiscard]] PreparedScalarCastInstructionRecordResult
scalar_cast_instruction_record_error(PreparedScalarCastRecordError error) {
  return PreparedScalarCastInstructionRecordResult{.record = std::nullopt, .error = error};
}

[[nodiscard]] PreparedScalarCastRecordError scalar_cast_operand_error_from_alu_error(
    PreparedScalarAluRecordError error) {
  switch (error) {
    case PreparedScalarAluRecordError::None:
      return PreparedScalarCastRecordError::None;
    case PreparedScalarAluRecordError::UnsupportedOperandValue:
      return PreparedScalarCastRecordError::UnsupportedOperandValue;
    case PreparedScalarAluRecordError::MissingOperandValueHome:
      return PreparedScalarCastRecordError::MissingOperandValueHome;
    case PreparedScalarAluRecordError::MissingOperandStorage:
      return PreparedScalarCastRecordError::MissingOperandStorage;
    case PreparedScalarAluRecordError::UnsupportedOperandStorage:
      return PreparedScalarCastRecordError::UnsupportedOperandStorage;
    case PreparedScalarAluRecordError::UnsupportedOperandType:
      return PreparedScalarCastRecordError::UnsupportedOperandType;
    case PreparedScalarAluRecordError::RegisterConversionFailed:
      return PreparedScalarCastRecordError::RegisterConversionFailed;
    case PreparedScalarAluRecordError::InvalidFunction:
      return PreparedScalarCastRecordError::InvalidFunction;
    case PreparedScalarAluRecordError::UnsupportedOpcode:
      return PreparedScalarCastRecordError::UnsupportedOpcode;
    case PreparedScalarAluRecordError::UnsupportedResultValue:
      return PreparedScalarCastRecordError::UnsupportedResultValue;
    case PreparedScalarAluRecordError::MissingResultValueHome:
      return PreparedScalarCastRecordError::MissingResultValueHome;
    case PreparedScalarAluRecordError::MissingResultStorage:
      return PreparedScalarCastRecordError::MissingResultStorage;
    case PreparedScalarAluRecordError::UnsupportedResultStorage:
      return PreparedScalarCastRecordError::UnsupportedResultStorage;
  }
  return PreparedScalarCastRecordError::UnsupportedOperandType;
}

}  // namespace

bool is_simple_integer_cast_opcode(bir::CastOpcode opcode) {
  switch (opcode) {
    case bir::CastOpcode::SExt:
    case bir::CastOpcode::ZExt:
    case bir::CastOpcode::Trunc:
      return true;
    case bir::CastOpcode::FPTrunc:
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI:
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP:
    case bir::CastOpcode::PtrToInt:
    case bir::CastOpcode::IntToPtr:
    case bir::CastOpcode::Bitcast:
      return false;
  }
  return false;
}

bool is_supported_scalar_conversion_cast_opcode(bir::CastOpcode opcode) {
  switch (opcode) {
    case bir::CastOpcode::FPTrunc:
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI:
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP:
      return true;
    case bir::CastOpcode::SExt:
    case bir::CastOpcode::ZExt:
    case bir::CastOpcode::Trunc:
    case bir::CastOpcode::PtrToInt:
    case bir::CastOpcode::IntToPtr:
    case bir::CastOpcode::Bitcast:
      return false;
  }
  return false;
}

ScalarCastOperationKind scalar_cast_operation_from_cast_opcode(
    bir::CastOpcode opcode) {
  switch (opcode) {
    case bir::CastOpcode::SExt:
      return ScalarCastOperationKind::SignExtend;
    case bir::CastOpcode::ZExt:
      return ScalarCastOperationKind::ZeroExtend;
    case bir::CastOpcode::Trunc:
      return ScalarCastOperationKind::Truncate;
    case bir::CastOpcode::FPTrunc:
      return ScalarCastOperationKind::FloatTruncate;
    case bir::CastOpcode::FPExt:
      return ScalarCastOperationKind::FloatExtend;
    case bir::CastOpcode::FPToSI:
      return ScalarCastOperationKind::FloatToSignedInt;
    case bir::CastOpcode::FPToUI:
      return ScalarCastOperationKind::FloatToUnsignedInt;
    case bir::CastOpcode::SIToFP:
      return ScalarCastOperationKind::SignedIntToFloat;
    case bir::CastOpcode::UIToFP:
      return ScalarCastOperationKind::UnsignedIntToFloat;
    case bir::CastOpcode::PtrToInt:
    case bir::CastOpcode::IntToPtr:
    case bir::CastOpcode::Bitcast:
      return ScalarCastOperationKind::Deferred;
  }
  return ScalarCastOperationKind::Deferred;
}

MachineOpcode machine_opcode_from_scalar_cast(ScalarCastOperationKind operation) {
  switch (operation) {
    case ScalarCastOperationKind::SignExtend:
      return MachineOpcode::SignExtend;
    case ScalarCastOperationKind::ZeroExtend:
      return MachineOpcode::ZeroExtend;
    case ScalarCastOperationKind::Truncate:
      return MachineOpcode::Truncate;
    case ScalarCastOperationKind::FloatExtend:
    case ScalarCastOperationKind::FloatTruncate:
    case ScalarCastOperationKind::SignedIntToFloat:
    case ScalarCastOperationKind::UnsignedIntToFloat:
    case ScalarCastOperationKind::FloatToSignedInt:
    case ScalarCastOperationKind::FloatToUnsignedInt:
    case ScalarCastOperationKind::Deferred:
      return MachineOpcode::Unspecified;
  }
  return MachineOpcode::Unspecified;
}

ScalarInstructionRecord make_scalar_cast_instruction_record(ScalarCastRecord cast) {
  return ScalarInstructionRecord{
      .result_value_id = cast.result_value_id,
      .result_value_name = cast.result_value_name,
      .result_type = cast.result_type,
      .result_register = cast.result_register,
      .inputs = {cast.source},
      .source_cast_opcode = cast.source_cast_opcode,
      .scalar_cast = cast,
  };
}

PreparedScalarCastRecordResult make_prepared_scalar_cast_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::CastInst& cast) {
  if (value_locations.function_name == c4c::kInvalidFunctionName ||
      storage_plan.function_name != value_locations.function_name) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::InvalidFunction);
  }
  const bool is_simple_integer_cast = is_simple_integer_cast_opcode(cast.opcode);
  const bool is_conversion_cast = is_supported_scalar_conversion_cast_opcode(cast.opcode);
  if (!is_simple_integer_cast && !is_conversion_cast) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedOpcode);
  }
  if (cast.result.kind != bir::Value::Kind::Named || cast.result.name.empty()) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedResultValue);
  }
  const bool source_is_integer = scalar_register_view(cast.operand.type).has_value();
  const bool result_is_integer = scalar_register_view(cast.result.type).has_value();
  const bool source_is_float = is_scalar_alu_floating_type(cast.operand.type);
  const bool result_is_float = is_scalar_alu_floating_type(cast.result.type);
  const bool supported_float_width_conversion =
      (cast.opcode == bir::CastOpcode::FPExt && cast.operand.type == bir::TypeKind::F32 &&
       cast.result.type == bir::TypeKind::F64) ||
      (cast.opcode == bir::CastOpcode::FPTrunc && cast.operand.type == bir::TypeKind::F64 &&
       cast.result.type == bir::TypeKind::F32);
  const bool supported_float_integer_conversion =
      ((cast.opcode == bir::CastOpcode::SIToFP || cast.opcode == bir::CastOpcode::UIToFP) &&
       source_is_integer && result_is_float) ||
      ((cast.opcode == bir::CastOpcode::FPToSI || cast.opcode == bir::CastOpcode::FPToUI) &&
       source_is_float && result_is_integer);
  if ((!is_simple_integer_cast || !source_is_integer || !result_is_integer) &&
      !supported_float_width_conversion && !supported_float_integer_conversion) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedOperandType);
  }

  const auto* result_home = find_named_value_home(names, value_locations, cast.result);
  if (result_home == nullptr || result_home->value_name == c4c::kInvalidValueName) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::MissingResultValueHome);
  }
  const auto* result_storage = find_storage_plan_value(storage_plan, result_home->value_id);
  if (result_storage == nullptr || result_storage->value_name != result_home->value_name) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::MissingResultStorage);
  }
  if (result_home->kind != prepare::PreparedValueHomeKind::Register ||
      result_storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      (!result_storage->register_placement.has_value() &&
       (!result_home->register_name.has_value() || !result_storage->register_name.has_value() ||
        *result_home->register_name != *result_storage->register_name))) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedResultStorage);
  }
  const auto result_register = make_prepared_register_operand(
      *result_home, *result_storage, cast.result.type, RegisterOperandRole::StoragePlan);
  if (!result_register.has_value()) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::RegisterConversionFailed);
  }

  OperandRecord source;
  if (const auto error =
          make_prepared_scalar_operand(names, value_locations, storage_plan, cast.operand, source);
      error != PreparedScalarAluRecordError::None) {
    return scalar_cast_record_error(scalar_cast_operand_error_from_alu_error(error));
  }
  const auto* source_register = std::get_if<RegisterOperand>(&source.payload);
  if ((is_conversion_cast || supported_float_width_conversion) &&
      (source.kind != OperandKind::Register || source_register == nullptr)) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedOperandStorage);
  }
  const auto source_bank =
      source_register != nullptr ? source_register->prepared_bank
                                 : prepare::PreparedRegisterBank::None;
  const auto result_bank = result_register->prepared_bank;

  return PreparedScalarCastRecordResult{
      .record =
          ScalarCastRecord{
              .surface = RecordSurfaceKind::RecordOnly,
              .operation = scalar_cast_operation_from_cast_opcode(cast.opcode),
              .source_cast_opcode = cast.opcode,
              .source_type = cast.operand.type,
              .result_value_id = result_home->value_id,
              .result_value_name = result_home->value_name,
              .result_type = cast.result.type,
              .result_register = result_register,
              .source = source,
              .source_register_bank = source_bank,
              .result_register_bank = result_bank,
              .crosses_register_bank = source_bank != prepare::PreparedRegisterBank::None &&
                                       result_bank != prepare::PreparedRegisterBank::None &&
                                       source_bank != result_bank,
              .supported_simple_integer_cast = is_simple_integer_cast,
              .supported_float_integer_conversion = supported_float_integer_conversion,
              .supported_float_width_conversion = supported_float_width_conversion,
          },
      .error = PreparedScalarCastRecordError::None,
  };
}

PreparedScalarCastInstructionRecordResult make_prepared_scalar_cast_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::CastInst& cast) {
  const auto result = make_prepared_scalar_cast_record(names, value_locations, storage_plan, cast);
  if (!result.record.has_value()) {
    return scalar_cast_instruction_record_error(result.error);
  }
  return PreparedScalarCastInstructionRecordResult{
      .record = make_scalar_cast_instruction_record(*result.record),
      .error = PreparedScalarCastRecordError::None,
  };
}

mir::TargetInstructionPrintResult print_scalar_conversion_instruction(
    const InstructionRecord& instruction,
    const ScalarInstructionRecord& scalar,
    const ScalarCastRecord& cast,
    std::string_view diagnostic_prefix) {
  (void)instruction;
  if ((!cast.supported_float_integer_conversion &&
       !cast.supported_float_width_conversion) ||
      cast.operation == ScalarCastOperationKind::Deferred) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar conversion node is outside the printable subset"));
  }
  const auto* source_register = std::get_if<RegisterOperand>(&cast.source.payload);
  if (cast.source.kind != OperandKind::Register || source_register == nullptr) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar conversion node requires a structured register source operand"));
  }

  std::string_view mnemonic;
  std::optional<std::string> result;
  std::optional<std::string> source;
  switch (cast.operation) {
    case ScalarCastOperationKind::FloatExtend:
      mnemonic = "fcvt";
      result = fp_register_name_with_view(*scalar.result_register, abi::RegisterView::D);
      source = fp_register_name_with_view(*source_register, abi::RegisterView::S);
      break;
    case ScalarCastOperationKind::FloatTruncate:
      mnemonic = "fcvt";
      result = fp_register_name_with_view(*scalar.result_register, abi::RegisterView::S);
      source = fp_register_name_with_view(*source_register, abi::RegisterView::D);
      break;
    case ScalarCastOperationKind::SignedIntToFloat:
    case ScalarCastOperationKind::UnsignedIntToFloat: {
      const auto result_view = floating_register_view(cast.result_type);
      const auto source_bits = integer_type_bit_width(cast.source_type);
      if (!result_view.has_value() || !source_bits.has_value()) {
        return target_unsupported(diagnostic(
            diagnostic_prefix,
            "scalar int-to-float conversion has unsupported type width"));
      }
      const auto source_view = integer_register_view(*source_bits);
      if (!source_view.has_value()) {
        return target_unsupported(diagnostic(
            diagnostic_prefix,
            "scalar int-to-float conversion has unsupported integer width"));
      }
      mnemonic = cast.operation == ScalarCastOperationKind::SignedIntToFloat ? "scvtf" : "ucvtf";
      result = fp_register_name_with_view(*scalar.result_register, *result_view);
      source = register_name_with_view(*source_register, *source_view);
      break;
    }
    case ScalarCastOperationKind::FloatToSignedInt:
    case ScalarCastOperationKind::FloatToUnsignedInt: {
      const auto result_bits = integer_type_bit_width(cast.result_type);
      const auto source_view = floating_register_view(cast.source_type);
      if (!result_bits.has_value() || !source_view.has_value()) {
        return target_unsupported(diagnostic(
            diagnostic_prefix,
            "scalar float-to-int conversion has unsupported type width"));
      }
      const auto result_view = integer_register_view(*result_bits);
      if (!result_view.has_value()) {
        return target_unsupported(diagnostic(
            diagnostic_prefix,
            "scalar float-to-int conversion has unsupported integer width"));
      }
      mnemonic = cast.operation == ScalarCastOperationKind::FloatToSignedInt ? "fcvtzs"
                                                                             : "fcvtzu";
      result = register_name_with_view(*scalar.result_register, *result_view);
      source = fp_register_name_with_view(*source_register, *source_view);
      break;
    }
    case ScalarCastOperationKind::SignExtend:
    case ScalarCastOperationKind::ZeroExtend:
    case ScalarCastOperationKind::Truncate:
    case ScalarCastOperationKind::Deferred:
      return target_unsupported(diagnostic(
          diagnostic_prefix,
          "scalar conversion node is outside the printable subset"));
  }
  if (mnemonic.empty() || !result.has_value() || !source.has_value()) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar conversion node has incomplete printable register facts"));
  }
  std::ostringstream out;
  out << mnemonic << " " << *result << ", " << *source;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_scalar_cast_instruction(
    const InstructionRecord& instruction,
    const ScalarInstructionRecord& scalar,
    std::string_view diagnostic_prefix) {
  (void)instruction;
  if (!scalar.result_register.has_value()) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar node is missing a structured destination register operand"));
  }
  if (!scalar.scalar_cast.has_value()) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar node is missing scalar cast metadata"));
  }

  const auto& cast = *scalar.scalar_cast;
  if (cast.supported_float_integer_conversion || cast.supported_float_width_conversion) {
    return print_scalar_conversion_instruction(
        instruction, scalar, cast, diagnostic_prefix);
  }
  if (!cast.supported_simple_integer_cast ||
      cast.operation == ScalarCastOperationKind::Deferred) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar cast node is outside the printable simple integer subset"));
  }
  const auto source_bits = integer_type_bit_width(cast.source_type);
  const auto result_bits = integer_type_bit_width(cast.result_type);
  if (!source_bits.has_value() || !result_bits.has_value() ||
      ((*source_bits >= *result_bits) &&
       cast.operation != ScalarCastOperationKind::Truncate) ||
      ((*source_bits <= *result_bits) &&
       cast.operation == ScalarCastOperationKind::Truncate)) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar cast node requires a supported integer source/result width"));
  }
  const auto* source_register = std::get_if<RegisterOperand>(&cast.source.payload);
  if (cast.source.kind != OperandKind::Register || source_register == nullptr) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar cast node requires a structured register source operand"));
  }
  const auto result_view = integer_register_view(*result_bits);
  if (!result_view.has_value()) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar cast node has an unsupported result register width"));
  }
  const auto result = register_name_with_view(*scalar.result_register, *result_view);
  if (!result.has_value()) {
    return target_unsupported(diagnostic(
        diagnostic_prefix,
        "scalar cast node destination is not a printable GPR register"));
  }

  std::ostringstream out;
  switch (cast.operation) {
    case ScalarCastOperationKind::SignExtend: {
      if (*source_bits == 1U) {
        const auto source = register_name_with_view(
            *source_register,
            *result_bits <= 32U ? abi::RegisterView::W : abi::RegisterView::X);
        if (!source.has_value()) {
          return target_unsupported(diagnostic(
              diagnostic_prefix,
              "scalar sign-extend node source is not a printable GPR register"));
        }
        out << "sbfx " << *result << ", " << *source << ", #0, #1";
      } else {
        std::string_view mnemonic;
        if (*source_bits == 8U) {
          mnemonic = "sxtb";
        } else if (*source_bits == 16U) {
          mnemonic = "sxth";
        } else if (*source_bits == 32U && *result_bits == 64U) {
          mnemonic = "sxtw";
        } else {
          return target_unsupported(diagnostic(
              diagnostic_prefix,
              "scalar sign-extend node has no printable width form"));
        }
        const auto source = register_name_with_view(*source_register, abi::RegisterView::W);
        if (!source.has_value()) {
          return target_unsupported(diagnostic(
              diagnostic_prefix,
              "scalar sign-extend node source is not a printable GPR register"));
        }
        out << mnemonic << " " << *result << ", " << *source;
      }
      return target_printed({out.str()});
    }
    case ScalarCastOperationKind::ZeroExtend: {
      const auto source = register_name_with_view(
          *source_register,
          *result_bits <= 32U ? abi::RegisterView::W : abi::RegisterView::X);
      if (!source.has_value()) {
        return target_unsupported(diagnostic(
            diagnostic_prefix,
            "scalar zero-extend node source is not a printable GPR register"));
      }
      out << "ubfx " << *result << ", " << *source << ", #0, #" << *source_bits;
      return target_printed({out.str()});
    }
    case ScalarCastOperationKind::Truncate: {
      if (*result_bits > 32U) {
        return target_unsupported(diagnostic(
            diagnostic_prefix,
            "scalar truncate node has no printable width form"));
      }
      const auto source = register_name_with_view(*source_register, abi::RegisterView::W);
      if (!source.has_value()) {
        return target_unsupported(diagnostic(
            diagnostic_prefix,
            "scalar truncate node source is not a printable GPR register"));
      }
      if (*result_bits == 32U) {
        out << "mov " << *result << ", " << *source;
      } else {
        const std::uint64_t mask = (std::uint64_t{1} << *result_bits) - 1U;
        out << "and " << *result << ", " << *source << ", #" << mask;
      }
      return target_printed({out.str()});
    }
    case ScalarCastOperationKind::FloatExtend:
    case ScalarCastOperationKind::FloatTruncate:
    case ScalarCastOperationKind::SignedIntToFloat:
    case ScalarCastOperationKind::UnsignedIntToFloat:
    case ScalarCastOperationKind::FloatToSignedInt:
    case ScalarCastOperationKind::FloatToUnsignedInt:
    case ScalarCastOperationKind::Deferred:
      break;
  }
  return target_unsupported(diagnostic(
      diagnostic_prefix,
      "scalar cast node is outside the printable simple integer subset"));
}

std::optional<module::MachineInstruction> lower_scalar_cast_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
  }

  const auto* cast = std::get_if<bir::CastInst>(&inst);
  if (cast == nullptr) {
    return std::nullopt;
  }

  const auto prepared = make_prepared_scalar_cast_instruction_record(
      context.function.prepared->names,
      *context.function.value_locations,
      *context.function.storage_plan,
      *cast);
  if (!prepared.record.has_value()) {
    return std::nullopt;
  }

  InstructionRecord target = make_scalar_instruction(*prepared.record);
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    return std::nullopt;
  }
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (prepared.record->result_register.has_value()) {
    record_emitted_scalar_register(scalar_state,
                                   prepared.record->result_value_name,
                                   *prepared.record->result_register);
  }

  return module::MachineInstruction{
      .opcode = static_cast<mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          mir::MachineOrigin{
              .reason = mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow->function_name,
              .block_label = context.control_flow_block->block_label,
              .instruction_index = instruction_index,
          },
  };
}

}  // namespace c4c::backend::aarch64::codegen
