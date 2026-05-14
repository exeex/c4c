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
      .expected_view = type == bir::TypeKind::I32 ? aarch64_abi::RegisterView::W
                                                  : aarch64_abi::RegisterView::X,
      .contiguous_width = 1,
  });
}

aarch64_codegen::ScalarAluRecord scalar_alu_record(
    aarch64_codegen::ScalarAluOperationKind operation,
    bir::BinaryOpcode source_opcode,
    bir::TypeKind type,
    prepare::PreparedValueId result_id,
    c4c::ValueNameId result_name,
    aarch64_codegen::OperandRecord lhs,
    aarch64_codegen::OperandRecord rhs) {
  return aarch64_codegen::ScalarAluRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .operation = operation,
      .source_binary_opcode = source_opcode,
      .operand_type = type,
      .result_value_id = result_id,
      .result_value_name = result_name,
      .result_type = type,
      .result_register =
          aarch64_codegen::RegisterOperand{
              .reg = aarch64_abi::x_register(0),
              .role = aarch64_codegen::RegisterOperandRole::StoragePlan,
              .value_id = result_id,
              .value_name = result_name,
              .prepared_class = prepare::PreparedRegisterClass::General,
              .prepared_bank = prepare::PreparedRegisterBank::Gpr,
              .expected_view = type == bir::TypeKind::I32 ? aarch64_abi::RegisterView::W
                                                          : aarch64_abi::RegisterView::X,
              .contiguous_width = 1,
              .occupied_registers = {"x0"},
          },
      .lhs = lhs,
      .rhs = rhs,
      .supported_integer_operation =
          aarch64_codegen::is_scalar_alu_integer_opcode(source_opcode),
  };
}

int add_and_sub_records_preserve_bir_and_prepared_value_facts() {
  const auto lhs = prepared_register_operand(prepare::PreparedValueId{10},
                                             c4c::ValueNameId{4},
                                             bir::TypeKind::I64,
                                             1);
  const auto rhs = prepared_register_operand(prepare::PreparedValueId{11},
                                             c4c::ValueNameId{5},
                                             bir::TypeKind::I64,
                                             2);
  const auto add_record = scalar_alu_record(aarch64_codegen::ScalarAluOperationKind::Add,
                                            bir::BinaryOpcode::Add,
                                            bir::TypeKind::I64,
                                            prepare::PreparedValueId{12},
                                            c4c::ValueNameId{6},
                                            lhs,
                                            rhs);
  const auto add_instruction = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(add_record));

  const auto* scalar =
      std::get_if<aarch64_codegen::ScalarInstructionRecord>(&add_instruction.payload);
  if (scalar == nullptr || add_instruction.family != aarch64_codegen::InstructionFamily::Scalar ||
      add_instruction.surface != aarch64_codegen::RecordSurfaceKind::MachineInstructionNode ||
      scalar->source_binary_opcode != bir::BinaryOpcode::Add || !scalar->scalar_alu.has_value()) {
    return fail("expected add to be a scalar machine-node instruction with ALU payload");
  }

  const auto& alu = *scalar->scalar_alu;
  if (alu.operation != aarch64_codegen::ScalarAluOperationKind::Add ||
      aarch64_codegen::scalar_alu_operation_kind_name(alu.operation) != "add" ||
      alu.source_binary_opcode != bir::BinaryOpcode::Add ||
      alu.operand_type != bir::TypeKind::I64 || alu.result_type != bir::TypeKind::I64 ||
      alu.result_value_id != prepare::PreparedValueId{12} ||
      alu.result_value_name != c4c::ValueNameId{6} || !alu.supported_integer_operation ||
      scalar->result_value_id != prepare::PreparedValueId{12} ||
      scalar->result_value_name != c4c::ValueNameId{6} ||
      scalar->result_type != bir::TypeKind::I64 || scalar->inputs.size() != 2 ||
      !alu.result_register.has_value() || !scalar->result_register.has_value() ||
      alu.result_register->reg != aarch64_abi::x_register(0) ||
      alu.result_register->value_id != prepare::PreparedValueId{12} ||
      scalar->result_register->expected_view != aarch64_abi::RegisterView::X ||
      add_instruction.defs.size() != 1 ||
      add_instruction.defs.front().kind != aarch64_codegen::MachineEffectResourceKind::Register ||
      add_instruction.defs.front().reg != aarch64_abi::x_register(0) ||
      add_instruction.defs.front().value_id != prepare::PreparedValueId{12}) {
    return fail("expected add ALU record to retain source opcode, types, and result identity");
  }

  const auto sub_record = scalar_alu_record(aarch64_codegen::ScalarAluOperationKind::Sub,
                                            bir::BinaryOpcode::Sub,
                                            bir::TypeKind::I32,
                                            prepare::PreparedValueId{20},
                                            c4c::ValueNameId{7},
                                            prepared_register_operand(prepare::PreparedValueId{18},
                                                                      c4c::ValueNameId{8},
                                                                      bir::TypeKind::I32,
                                                                      3),
                                            prepared_register_operand(prepare::PreparedValueId{19},
                                                                      c4c::ValueNameId{9},
                                                                      bir::TypeKind::I32,
                                                                      4));
  if (sub_record.operation != aarch64_codegen::scalar_alu_operation_from_binary_opcode(
                                  bir::BinaryOpcode::Sub) ||
      aarch64_codegen::scalar_alu_operation_kind_name(sub_record.operation) != "sub" ||
      !sub_record.supported_integer_operation) {
    return fail("expected sub to use typed scalar ALU vocabulary");
  }
  const auto sub_instruction = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(sub_record));
  if (!sub_record.result_register.has_value() ||
      sub_record.result_register->expected_view != aarch64_abi::RegisterView::W ||
      sub_instruction.defs.size() != 1 ||
      sub_instruction.defs.front().reg != aarch64_abi::x_register(0) ||
      sub_instruction.defs.front().value_id != prepare::PreparedValueId{20}) {
    return fail("expected sub selected machine node to carry a typed destination register");
  }

  return 0;
}

int bitwise_records_have_distinct_generic_diagnostic_names() {
  const auto lhs = prepared_register_operand(prepare::PreparedValueId{30},
                                             c4c::ValueNameId{10},
                                             bir::TypeKind::I64,
                                             5);
  const auto rhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
      .kind = aarch64_codegen::ImmediateKind::UnsignedInteger,
      .type = bir::TypeKind::I64,
      .unsigned_value = 255,
  });

  const auto and_record = scalar_alu_record(aarch64_codegen::ScalarAluOperationKind::And,
                                            bir::BinaryOpcode::And,
                                            bir::TypeKind::I64,
                                            prepare::PreparedValueId{31},
                                            c4c::ValueNameId{11},
                                            lhs,
                                            rhs);
  const auto or_record = scalar_alu_record(aarch64_codegen::ScalarAluOperationKind::Or,
                                           bir::BinaryOpcode::Or,
                                           bir::TypeKind::I64,
                                           prepare::PreparedValueId{32},
                                           c4c::ValueNameId{12},
                                           lhs,
                                           rhs);
  const auto xor_record = scalar_alu_record(aarch64_codegen::ScalarAluOperationKind::Xor,
                                            bir::BinaryOpcode::Xor,
                                            bir::TypeKind::I64,
                                            prepare::PreparedValueId{33},
                                            c4c::ValueNameId{13},
                                            lhs,
                                            rhs);

  if (aarch64_codegen::scalar_alu_operation_kind_name(and_record.operation) != "and" ||
      aarch64_codegen::scalar_alu_operation_kind_name(or_record.operation) != "or" ||
      aarch64_codegen::scalar_alu_operation_kind_name(xor_record.operation) != "xor") {
    return fail("expected bitwise scalar ALU operations to have distinct diagnostic names");
  }
  if (!and_record.supported_integer_operation || !or_record.supported_integer_operation ||
      !xor_record.supported_integer_operation ||
      aarch64_codegen::scalar_alu_operation_from_binary_opcode(bir::BinaryOpcode::And) !=
          aarch64_codegen::ScalarAluOperationKind::And ||
      aarch64_codegen::scalar_alu_operation_from_binary_opcode(bir::BinaryOpcode::Or) !=
          aarch64_codegen::ScalarAluOperationKind::Or ||
      aarch64_codegen::scalar_alu_operation_from_binary_opcode(bir::BinaryOpcode::Xor) !=
          aarch64_codegen::ScalarAluOperationKind::Xor) {
    return fail("expected bitwise BIR opcodes to map to supported ALU records");
  }
  if (and_record.rhs.kind != aarch64_codegen::OperandKind::Immediate ||
      or_record.lhs.kind != aarch64_codegen::OperandKind::Register ||
      xor_record.operand_type != bir::TypeKind::I64) {
    return fail("expected bitwise records to preserve structured source operands");
  }

  return 0;
}

int deferred_scalar_forms_are_explicit_records_not_generic_support() {
  if (aarch64_codegen::is_scalar_alu_integer_opcode(bir::BinaryOpcode::Mul) ||
      aarch64_codegen::is_scalar_alu_integer_opcode(bir::BinaryOpcode::Eq)) {
    return fail("expected non-slice opcodes to remain outside supported scalar ALU vocabulary");
  }
  if (aarch64_codegen::scalar_alu_operation_from_binary_opcode(bir::BinaryOpcode::Mul) !=
          aarch64_codegen::ScalarAluOperationKind::Deferred ||
      aarch64_codegen::scalar_alu_operation_from_binary_opcode(bir::BinaryOpcode::Eq) !=
          aarch64_codegen::ScalarAluOperationKind::Deferred ||
      aarch64_codegen::scalar_alu_operation_kind_name(
          aarch64_codegen::ScalarAluOperationKind::Deferred) != "deferred") {
    return fail("expected deferred scalar forms to be named explicitly");
  }

  const auto deferred = scalar_alu_record(aarch64_codegen::ScalarAluOperationKind::Deferred,
                                          bir::BinaryOpcode::Mul,
                                          bir::TypeKind::I64,
                                          prepare::PreparedValueId{40},
                                          c4c::ValueNameId{14},
                                          prepared_register_operand(prepare::PreparedValueId{41},
                                                                    c4c::ValueNameId{15},
                                                                    bir::TypeKind::I64,
                                                                    6),
                                          prepared_register_operand(prepare::PreparedValueId{42},
                                                                    c4c::ValueNameId{16},
                                                                    bir::TypeKind::I64,
                                                                    7));
  if (deferred.operation != aarch64_codegen::ScalarAluOperationKind::Deferred ||
      deferred.source_binary_opcode != bir::BinaryOpcode::Mul ||
      deferred.supported_integer_operation) {
    return fail("expected deferred record to preserve source opcode without claiming support");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status = add_and_sub_records_preserve_bir_and_prepared_value_facts();
      status != 0) {
    return status;
  }
  if (const int status = bitwise_records_have_distinct_generic_diagnostic_names();
      status != 0) {
    return status;
  }
  if (const int status = deferred_scalar_forms_are_explicit_records_not_generic_support();
      status != 0) {
    return status;
  }
  return 0;
}
