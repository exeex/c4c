#include "src/backend/mir/aarch64/codegen/cast_ops.hpp"
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
      .reg = type == bir::TypeKind::I64 ? aarch64_abi::x_register(reg_index)
                                        : aarch64_abi::w_register(reg_index),
      .role = aarch64_codegen::RegisterOperandRole::StoragePlan,
      .value_id = value_id,
      .value_name = value_name,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = type == bir::TypeKind::I64 ? aarch64_abi::RegisterView::X
                                                  : aarch64_abi::RegisterView::W,
      .contiguous_width = 1,
  });
}

aarch64_codegen::ScalarAluRecord alu_record(bir::BinaryOpcode opcode) {
  return aarch64_codegen::ScalarAluRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .operation = aarch64_codegen::scalar_alu_operation_from_binary_opcode(opcode),
      .source_binary_opcode = opcode,
      .operand_type = bir::TypeKind::I64,
      .result_value_id = prepare::PreparedValueId{30},
      .result_value_name = c4c::ValueNameId{7},
      .result_type = bir::TypeKind::I64,
      .lhs = prepared_register_operand(prepare::PreparedValueId{31},
                                       c4c::ValueNameId{8},
                                       bir::TypeKind::I64,
                                       1),
      .rhs = prepared_register_operand(prepare::PreparedValueId{32},
                                       c4c::ValueNameId{9},
                                       bir::TypeKind::I64,
                                       2),
      .supported_integer_operation = aarch64_codegen::is_scalar_alu_integer_opcode(opcode),
  };
}

aarch64_codegen::ScalarCastRecord cast_record(bir::CastOpcode opcode,
                                              bir::TypeKind source_type,
                                              bir::TypeKind result_type) {
  return aarch64_codegen::ScalarCastRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .operation = aarch64_codegen::scalar_cast_operation_from_cast_opcode(opcode),
      .source_cast_opcode = opcode,
      .source_type = source_type,
      .result_value_id = prepare::PreparedValueId{40},
      .result_value_name = c4c::ValueNameId{10},
      .result_type = result_type,
      .source = prepared_register_operand(prepare::PreparedValueId{41},
                                          c4c::ValueNameId{11},
                                          source_type,
                                          3),
      .supported_simple_integer_cast = aarch64_codegen::is_simple_integer_cast_opcode(opcode),
      .supported_float_integer_conversion =
          opcode == bir::CastOpcode::FPToSI || opcode == bir::CastOpcode::FPToUI ||
          opcode == bir::CastOpcode::SIToFP || opcode == bir::CastOpcode::UIToFP,
      .supported_float_width_conversion =
          opcode == bir::CastOpcode::FPExt || opcode == bir::CastOpcode::FPTrunc,
  };
}

int scalar_records_preserve_bir_prepared_identity_and_make_machine_nodes() {
  const auto scalar_alu = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(alu_record(bir::BinaryOpcode::Add)));
  const auto scalar_cast = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(
          cast_record(bir::CastOpcode::SExt, bir::TypeKind::I32, bir::TypeKind::I64)));

  const auto* alu_payload =
      std::get_if<aarch64_codegen::ScalarInstructionRecord>(&scalar_alu.payload);
  const auto* cast_payload =
      std::get_if<aarch64_codegen::ScalarInstructionRecord>(&scalar_cast.payload);
  if (scalar_alu.family != aarch64_codegen::InstructionFamily::Scalar ||
      scalar_cast.family != aarch64_codegen::InstructionFamily::Scalar ||
      scalar_alu.surface != aarch64_codegen::RecordSurfaceKind::MachineInstructionNode ||
      scalar_cast.surface != aarch64_codegen::RecordSurfaceKind::MachineInstructionNode ||
      scalar_alu.opcode != aarch64_codegen::MachineOpcode::Add ||
      scalar_cast.opcode != aarch64_codegen::MachineOpcode::SignExtend ||
      aarch64_codegen::machine_opcode_name(scalar_cast.opcode) != "sign_extend" ||
      scalar_alu.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      scalar_cast.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      scalar_alu.operands.size() != 2 || scalar_cast.operands.size() != 1 ||
      scalar_alu.defs.size() != 1 || scalar_cast.defs.size() != 1 ||
      scalar_alu.uses.size() != 2 || scalar_cast.uses.size() != 1 ||
      alu_payload == nullptr || cast_payload == nullptr) {
    return fail("expected scalar ALU and cast instructions to be machine-node wrappers");
  }
  if (scalar_alu.defs.front().kind != aarch64_codegen::MachineEffectResourceKind::PreparedValue ||
      scalar_alu.defs.front().value_id != prepare::PreparedValueId{30} ||
      scalar_alu.uses.front().kind != aarch64_codegen::MachineEffectResourceKind::Register ||
      scalar_alu.uses.front().reg != aarch64_abi::x_register(1) ||
      aarch64_codegen::machine_effect_resource_kind_name(scalar_alu.uses.front().kind) !=
          "register") {
    return fail("expected scalar machine node to publish def/use register metadata");
  }

  if (!alu_payload->scalar_alu.has_value() || alu_payload->scalar_cast.has_value() ||
      alu_payload->source_binary_opcode != bir::BinaryOpcode::Add ||
      alu_payload->source_cast_opcode.has_value() ||
      alu_payload->result_value_id != prepare::PreparedValueId{30} ||
      alu_payload->result_value_name != c4c::ValueNameId{7} ||
      alu_payload->result_type != bir::TypeKind::I64 || alu_payload->inputs.size() != 2) {
    return fail("expected scalar ALU wrapper to preserve result ids, type, and BIR opcode");
  }
  const auto& alu = *alu_payload->scalar_alu;
  if (alu.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
      alu.operation != aarch64_codegen::ScalarAluOperationKind::Add ||
      alu.source_binary_opcode != bir::BinaryOpcode::Add ||
      alu.operand_type != bir::TypeKind::I64 || alu.result_type != bir::TypeKind::I64 ||
      alu.result_value_id != prepare::PreparedValueId{30} ||
      alu.result_value_name != c4c::ValueNameId{7} || !alu.supported_integer_operation ||
      alu.lhs.kind != aarch64_codegen::OperandKind::Register ||
      alu.rhs.kind != aarch64_codegen::OperandKind::Register) {
    return fail("expected scalar ALU record to preserve explicit supported operation facts");
  }

  if (!cast_payload->scalar_cast.has_value() || cast_payload->scalar_alu.has_value() ||
      cast_payload->source_cast_opcode != bir::CastOpcode::SExt ||
      cast_payload->source_binary_opcode.has_value() ||
      cast_payload->result_value_id != prepare::PreparedValueId{40} ||
      cast_payload->result_value_name != c4c::ValueNameId{10} ||
      cast_payload->result_type != bir::TypeKind::I64 || cast_payload->inputs.size() != 1) {
    return fail("expected scalar cast wrapper to preserve result ids, type, and BIR opcode");
  }
  const auto& cast = *cast_payload->scalar_cast;
  if (cast.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
      cast.operation != aarch64_codegen::ScalarCastOperationKind::SignExtend ||
      cast.source_cast_opcode != bir::CastOpcode::SExt ||
      cast.source_type != bir::TypeKind::I32 || cast.result_type != bir::TypeKind::I64 ||
      cast.result_value_id != prepare::PreparedValueId{40} ||
      cast.result_value_name != c4c::ValueNameId{10} ||
      !cast.supported_simple_integer_cast ||
      cast.source.kind != aarch64_codegen::OperandKind::Register) {
    return fail("expected scalar cast record to preserve explicit supported cast facts");
  }

  return 0;
}

int supported_and_deferred_scalar_vocabulary_is_explicit() {
  if (!aarch64_codegen::is_scalar_alu_integer_opcode(bir::BinaryOpcode::Add) ||
      !aarch64_codegen::is_scalar_alu_integer_opcode(bir::BinaryOpcode::Sub) ||
      !aarch64_codegen::is_scalar_alu_integer_opcode(bir::BinaryOpcode::And) ||
      !aarch64_codegen::is_scalar_alu_integer_opcode(bir::BinaryOpcode::Or) ||
      !aarch64_codegen::is_scalar_alu_integer_opcode(bir::BinaryOpcode::Xor) ||
      !aarch64_codegen::is_scalar_alu_floating_opcode(bir::BinaryOpcode::Mul) ||
      !aarch64_codegen::is_scalar_alu_floating_opcode(bir::BinaryOpcode::SDiv) ||
      !aarch64_codegen::is_scalar_alu_floating_type(bir::TypeKind::F32) ||
      !aarch64_codegen::is_scalar_alu_floating_type(bir::TypeKind::F64) ||
      aarch64_codegen::scalar_alu_operation_kind_name(
          aarch64_codegen::ScalarAluOperationKind::Add) != "add" ||
      aarch64_codegen::scalar_alu_operation_kind_name(
          aarch64_codegen::ScalarAluOperationKind::Xor) != "xor" ||
      aarch64_codegen::scalar_alu_operation_kind_name(
          aarch64_codegen::ScalarAluOperationKind::Mul) != "mul" ||
      aarch64_codegen::scalar_alu_operation_kind_name(
          aarch64_codegen::ScalarAluOperationKind::Div) != "div" ||
      aarch64_codegen::scalar_alu_operation_kind_name(
          aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight) !=
          "logical_shift_right") {
    return fail("expected supported scalar ALU vocabulary to be explicit");
  }
  if (aarch64_codegen::is_scalar_alu_integer_opcode(bir::BinaryOpcode::Mul) ||
      aarch64_codegen::is_scalar_alu_integer_opcode(bir::BinaryOpcode::Eq) ||
      !aarch64_codegen::is_scalar_alu_integer_opcode(bir::BinaryOpcode::Shl) ||
      !aarch64_codegen::is_scalar_alu_integer_opcode(bir::BinaryOpcode::LShr) ||
      aarch64_codegen::scalar_alu_operation_from_binary_opcode(bir::BinaryOpcode::Mul) !=
          aarch64_codegen::ScalarAluOperationKind::Mul ||
      aarch64_codegen::scalar_alu_operation_from_binary_opcode(bir::BinaryOpcode::SDiv) !=
          aarch64_codegen::ScalarAluOperationKind::Div ||
      aarch64_codegen::scalar_alu_operation_from_binary_opcode(bir::BinaryOpcode::Shl) !=
          aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight ||
      aarch64_codegen::scalar_alu_operation_from_binary_opcode(bir::BinaryOpcode::LShr) !=
          aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight ||
      aarch64_codegen::scalar_alu_operation_from_binary_opcode(bir::BinaryOpcode::Eq) !=
          aarch64_codegen::ScalarAluOperationKind::Deferred) {
    return fail("expected unsupported scalar ALU forms to defer explicitly");
  }

  if (!aarch64_codegen::is_simple_integer_cast_opcode(bir::CastOpcode::SExt) ||
      !aarch64_codegen::is_simple_integer_cast_opcode(bir::CastOpcode::ZExt) ||
      !aarch64_codegen::is_simple_integer_cast_opcode(bir::CastOpcode::Trunc) ||
      aarch64_codegen::scalar_cast_operation_kind_name(
          aarch64_codegen::ScalarCastOperationKind::SignExtend) != "sign_extend" ||
      aarch64_codegen::scalar_cast_operation_kind_name(
          aarch64_codegen::ScalarCastOperationKind::Truncate) != "truncate") {
    return fail("expected supported scalar cast vocabulary to be explicit");
  }
  if (aarch64_codegen::is_simple_integer_cast_opcode(bir::CastOpcode::FPExt) ||
      !aarch64_codegen::is_supported_scalar_conversion_cast_opcode(bir::CastOpcode::FPExt) ||
      aarch64_codegen::is_simple_integer_cast_opcode(bir::CastOpcode::Bitcast) ||
      aarch64_codegen::scalar_cast_operation_from_cast_opcode(bir::CastOpcode::FPExt) !=
          aarch64_codegen::ScalarCastOperationKind::FloatExtend ||
      aarch64_codegen::scalar_cast_operation_from_cast_opcode(bir::CastOpcode::Bitcast) !=
          aarch64_codegen::ScalarCastOperationKind::Deferred ||
      aarch64_codegen::scalar_cast_operation_kind_name(
          aarch64_codegen::ScalarCastOperationKind::Deferred) != "deferred") {
    return fail("expected unsupported scalar cast forms to defer explicitly");
  }

  const auto deferred_alu = alu_record(bir::BinaryOpcode::Mul);
  const auto deferred_cast = cast_record(bir::CastOpcode::Bitcast,
                                         bir::TypeKind::F32,
                                         bir::TypeKind::F64);
  if (deferred_alu.operation != aarch64_codegen::ScalarAluOperationKind::Mul ||
      deferred_alu.supported_integer_operation ||
      deferred_alu.supported_floating_operation ||
      deferred_alu.source_binary_opcode != bir::BinaryOpcode::Mul ||
      deferred_cast.operation != aarch64_codegen::ScalarCastOperationKind::Deferred ||
      deferred_cast.supported_simple_integer_cast ||
      deferred_cast.supported_float_integer_conversion ||
      deferred_cast.supported_float_width_conversion ||
      deferred_cast.source_cast_opcode != bir::CastOpcode::Bitcast) {
    return fail("expected deferred scalar records to preserve source opcodes without support");
  }

  const auto deferred_alu_node = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(deferred_alu));
  const auto deferred_cast_node = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(deferred_cast));
  if (deferred_alu_node.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::DeferredUnsupported ||
      deferred_cast_node.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::DeferredUnsupported ||
      deferred_alu_node.opcode != aarch64_codegen::MachineOpcode::Mul ||
      deferred_cast_node.opcode != aarch64_codegen::MachineOpcode::Unspecified ||
      deferred_alu_node.selection.diagnostic !=
          "scalar ALU operation is outside the selected subset" ||
      deferred_cast_node.selection.diagnostic !=
          "scalar cast operation is outside the selected subset") {
    return fail("expected unsupported scalar machine nodes to fail closed");
  }

  return 0;
}

int scalar_records_do_not_own_other_instruction_families() {
  const auto scalar_alu = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(alu_record(bir::BinaryOpcode::Or)));
  const auto scalar_cast = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(
          cast_record(bir::CastOpcode::Trunc, bir::TypeKind::I64, bir::TypeKind::I32)));

  if (std::holds_alternative<aarch64_codegen::MemoryInstructionRecord>(scalar_alu.payload) ||
      std::holds_alternative<aarch64_codegen::CallInstructionRecord>(scalar_alu.payload) ||
      std::holds_alternative<aarch64_codegen::ReturnInstructionRecord>(scalar_alu.payload) ||
      std::holds_alternative<aarch64_codegen::AssemblerInstructionRecord>(scalar_alu.payload) ||
      std::holds_alternative<aarch64_codegen::ObjectInstructionRecord>(scalar_alu.payload) ||
      std::holds_alternative<aarch64_codegen::MemoryInstructionRecord>(scalar_cast.payload) ||
      std::holds_alternative<aarch64_codegen::CallInstructionRecord>(scalar_cast.payload) ||
      std::holds_alternative<aarch64_codegen::ReturnInstructionRecord>(scalar_cast.payload) ||
      std::holds_alternative<aarch64_codegen::AssemblerInstructionRecord>(scalar_cast.payload) ||
      std::holds_alternative<aarch64_codegen::ObjectInstructionRecord>(scalar_cast.payload)) {
    return fail("expected scalar records not to own memory, call, return, assembler, or object payloads");
  }
  if (aarch64_codegen::instruction_family_name(scalar_alu.family) != "scalar" ||
      aarch64_codegen::record_surface_kind_name(scalar_alu.surface) != "machine_instruction_node" ||
      aarch64_codegen::machine_opcode_name(scalar_alu.opcode) == "unspecified" ||
      aarch64_codegen::machine_pseudo_kind_name(scalar_alu.pseudo) != "none" ||
      aarch64_codegen::instruction_family_name(scalar_cast.family) != "scalar" ||
      aarch64_codegen::record_surface_kind_name(scalar_cast.surface) !=
          "machine_instruction_node") {
    return fail("expected scalar wrappers to expose machine-node diagnostics");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status = scalar_records_preserve_bir_prepared_identity_and_make_machine_nodes();
      status != 0) {
    return status;
  }
  if (const int status = supported_and_deferred_scalar_vocabulary_is_explicit(); status != 0) {
    return status;
  }
  if (const int status = scalar_records_do_not_own_other_instruction_families(); status != 0) {
    return status;
  }
  return 0;
}
