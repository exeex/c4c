#include "src/backend/mir/aarch64/codegen/instruction.hpp"

#include <iostream>
#include <variant>

namespace {

namespace aarch64_abi = c4c::backend::aarch64::abi;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

aarch64_codegen::OperandRecord prepared_register_operand(prepare::PreparedValueId value_id,
                                                         c4c::ValueNameId value_name,
                                                         bir::TypeKind type,
                                                         unsigned reg_index) {
  return aarch64_codegen::make_register_operand(aarch64_codegen::RegisterOperand{
      .reg = aarch64_abi::x_register(reg_index),
      .role = aarch64_codegen::RegisterOperandRole::PreparedAssignment,
      .value_id = value_id,
      .value_name = value_name,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = type == bir::TypeKind::I64 ? aarch64_abi::RegisterView::X
                                                  : aarch64_abi::RegisterView::W,
      .contiguous_width = 1,
  });
}

aarch64_codegen::ScalarCastRecord scalar_cast_record(
    aarch64_codegen::ScalarCastOperationKind operation,
    bir::CastOpcode source_opcode,
    bir::TypeKind source_type,
    bir::TypeKind result_type,
    prepare::PreparedValueId result_id,
    c4c::ValueNameId result_name,
    aarch64_codegen::OperandRecord source) {
  return aarch64_codegen::ScalarCastRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .operation = operation,
      .source_cast_opcode = source_opcode,
      .source_type = source_type,
      .result_value_id = result_id,
      .result_value_name = result_name,
      .result_type = result_type,
      .source = source,
      .supported_simple_integer_cast =
          aarch64_codegen::is_simple_integer_cast_opcode(source_opcode),
      .supported_float_integer_conversion =
          source_opcode == bir::CastOpcode::FPToSI ||
          source_opcode == bir::CastOpcode::FPToUI ||
          source_opcode == bir::CastOpcode::SIToFP ||
          source_opcode == bir::CastOpcode::UIToFP,
      .supported_float_width_conversion =
          source_opcode == bir::CastOpcode::FPExt ||
          source_opcode == bir::CastOpcode::FPTrunc,
  };
}

int sign_and_zero_extend_records_preserve_source_and_result_facts() {
  const auto source = prepared_register_operand(prepare::PreparedValueId{20},
                                                c4c::ValueNameId{7},
                                                bir::TypeKind::I32,
                                                1);
  const auto sext_record = scalar_cast_record(aarch64_codegen::ScalarCastOperationKind::SignExtend,
                                              bir::CastOpcode::SExt,
                                              bir::TypeKind::I32,
                                              bir::TypeKind::I64,
                                              prepare::PreparedValueId{21},
                                              c4c::ValueNameId{8},
                                              source);
  const auto sext_instruction = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(sext_record));

  const auto* scalar =
      std::get_if<aarch64_codegen::ScalarInstructionRecord>(&sext_instruction.payload);
  if (scalar == nullptr ||
      sext_instruction.family != aarch64_codegen::InstructionFamily::Scalar ||
      sext_instruction.surface != aarch64_codegen::RecordSurfaceKind::MachineInstructionNode ||
      scalar->source_cast_opcode != bir::CastOpcode::SExt || scalar->source_binary_opcode ||
      !scalar->scalar_cast.has_value() || scalar->scalar_alu.has_value()) {
    return fail("expected sign extension to be a scalar machine-node cast instruction");
  }

  const auto& cast = *scalar->scalar_cast;
  if (cast.operation != aarch64_codegen::ScalarCastOperationKind::SignExtend ||
      aarch64_codegen::scalar_cast_operation_kind_name(cast.operation) != "sign_extend" ||
      cast.source_cast_opcode != bir::CastOpcode::SExt ||
      cast.source_type != bir::TypeKind::I32 || cast.result_type != bir::TypeKind::I64 ||
      cast.result_value_id != prepare::PreparedValueId{21} ||
      cast.result_value_name != c4c::ValueNameId{8} || !cast.supported_simple_integer_cast ||
      scalar->result_value_id != prepare::PreparedValueId{21} ||
      scalar->result_value_name != c4c::ValueNameId{8} ||
      scalar->result_type != bir::TypeKind::I64 || scalar->inputs.size() != 1 ||
      scalar->inputs.front().kind != aarch64_codegen::OperandKind::Register) {
    return fail("expected sign extension to retain BIR cast, types, result id, and source");
  }

  const auto zext_record = scalar_cast_record(aarch64_codegen::ScalarCastOperationKind::ZeroExtend,
                                              bir::CastOpcode::ZExt,
                                              bir::TypeKind::I1,
                                              bir::TypeKind::I32,
                                              prepare::PreparedValueId{22},
                                              c4c::ValueNameId{9},
                                              prepared_register_operand(
                                                  prepare::PreparedValueId{23},
                                                  c4c::ValueNameId{10},
                                                  bir::TypeKind::I1,
                                                  2));
  if (zext_record.operation != aarch64_codegen::scalar_cast_operation_from_cast_opcode(
                                   bir::CastOpcode::ZExt) ||
      aarch64_codegen::scalar_cast_operation_kind_name(zext_record.operation) != "zero_extend" ||
      zext_record.source_type != bir::TypeKind::I1 ||
      zext_record.result_type != bir::TypeKind::I32 ||
      !zext_record.supported_simple_integer_cast) {
    return fail("expected zero extension to use typed simple integer cast vocabulary");
  }

  return 0;
}

int truncate_records_preserve_structured_source_operand() {
  const auto source = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
      .kind = aarch64_codegen::ImmediateKind::UnsignedInteger,
      .type = bir::TypeKind::I64,
      .unsigned_value = 0xff00U,
      .source_value_id = prepare::PreparedValueId{30},
      .source_value_name = c4c::ValueNameId{11},
  });
  const auto trunc_record = scalar_cast_record(aarch64_codegen::ScalarCastOperationKind::Truncate,
                                               bir::CastOpcode::Trunc,
                                               bir::TypeKind::I64,
                                               bir::TypeKind::I8,
                                               prepare::PreparedValueId{31},
                                               c4c::ValueNameId{12},
                                               source);

  if (trunc_record.operation != aarch64_codegen::ScalarCastOperationKind::Truncate ||
      aarch64_codegen::scalar_cast_operation_kind_name(trunc_record.operation) != "truncate" ||
      aarch64_codegen::scalar_cast_operation_from_cast_opcode(bir::CastOpcode::Trunc) !=
          aarch64_codegen::ScalarCastOperationKind::Truncate ||
      trunc_record.source.kind != aarch64_codegen::OperandKind::Immediate ||
      trunc_record.source_type != bir::TypeKind::I64 ||
      trunc_record.result_type != bir::TypeKind::I8 ||
      trunc_record.result_value_id != prepare::PreparedValueId{31}) {
    return fail("expected truncate record to preserve source operand and result identity");
  }

  return 0;
}

int unsupported_cast_forms_are_explicit_deferred_records() {
  if (aarch64_codegen::is_simple_integer_cast_opcode(bir::CastOpcode::FPExt) ||
      !aarch64_codegen::is_supported_scalar_conversion_cast_opcode(bir::CastOpcode::FPExt) ||
      !aarch64_codegen::is_supported_scalar_conversion_cast_opcode(bir::CastOpcode::FPToSI) ||
      aarch64_codegen::is_simple_integer_cast_opcode(bir::CastOpcode::Bitcast)) {
    return fail("expected conversion cast opcodes to be distinct from simple cast vocabulary");
  }
  if (aarch64_codegen::scalar_cast_operation_from_cast_opcode(bir::CastOpcode::FPExt) !=
          aarch64_codegen::ScalarCastOperationKind::FloatExtend ||
      aarch64_codegen::scalar_cast_operation_from_cast_opcode(bir::CastOpcode::FPToSI) !=
          aarch64_codegen::ScalarCastOperationKind::FloatToSignedInt ||
      aarch64_codegen::scalar_cast_operation_kind_name(
          aarch64_codegen::ScalarCastOperationKind::UnsignedIntToFloat) !=
          "unsigned_int_to_float" ||
      aarch64_codegen::scalar_cast_operation_from_cast_opcode(bir::CastOpcode::Bitcast) !=
          aarch64_codegen::ScalarCastOperationKind::Deferred ||
      aarch64_codegen::scalar_cast_operation_kind_name(
          aarch64_codegen::ScalarCastOperationKind::Deferred) != "deferred") {
    return fail("expected unsupported cast forms to have explicit deferred diagnostics");
  }

  const auto deferred = scalar_cast_record(aarch64_codegen::ScalarCastOperationKind::Deferred,
                                           bir::CastOpcode::Bitcast,
                                           bir::TypeKind::F32,
                                           bir::TypeKind::F64,
                                           prepare::PreparedValueId{40},
                                           c4c::ValueNameId{13},
                                           prepared_register_operand(prepare::PreparedValueId{41},
                                                                     c4c::ValueNameId{14},
                                                                     bir::TypeKind::I32,
                                                                     3));
  if (deferred.operation != aarch64_codegen::ScalarCastOperationKind::Deferred ||
      deferred.source_cast_opcode != bir::CastOpcode::Bitcast ||
      deferred.supported_simple_integer_cast ||
      deferred.supported_float_integer_conversion ||
      deferred.supported_float_width_conversion) {
    return fail("expected deferred cast record to preserve source opcode without claiming support");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status = sign_and_zero_extend_records_preserve_source_and_result_facts();
      status != 0) {
    return status;
  }
  if (const int status = truncate_records_preserve_structured_source_operand(); status != 0) {
    return status;
  }
  if (const int status = unsupported_cast_forms_are_explicit_deferred_records(); status != 0) {
    return status;
  }
  return 0;
}
