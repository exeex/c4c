#include "src/backend/mir/aarch64/codegen/alu.hpp"

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

aarch64_codegen::OperandRecord prepared_fpr_operand(prepare::PreparedValueId value_id,
                                                    c4c::ValueNameId value_name,
                                                    bir::TypeKind type,
                                                    unsigned reg_index) {
  return aarch64_codegen::make_register_operand(aarch64_codegen::RegisterOperand{
      .reg = type == bir::TypeKind::F32 ? aarch64_abi::s_register(reg_index)
                                        : aarch64_abi::d_register(reg_index),
      .role = aarch64_codegen::RegisterOperandRole::PreparedAssignment,
      .value_id = value_id,
      .value_name = value_name,
      .prepared_class = prepare::PreparedRegisterClass::Float,
      .prepared_bank = prepare::PreparedRegisterBank::Fpr,
      .expected_view = type == bir::TypeKind::F32 ? aarch64_abi::RegisterView::S
                                                  : aarch64_abi::RegisterView::D,
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
      .supported_floating_operation =
          aarch64_codegen::is_scalar_alu_floating_type(type) &&
          aarch64_codegen::is_scalar_alu_floating_opcode(source_opcode),
  };
}

aarch64_codegen::ScalarUnaryRecord scalar_unary_record(
    aarch64_codegen::ScalarUnaryOperationKind operation,
    bir::TypeKind type,
    prepare::PreparedValueId result_id,
    c4c::ValueNameId result_name,
    aarch64_codegen::OperandRecord operand) {
  return aarch64_codegen::ScalarUnaryRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .operation = operation,
      .operand_type = type,
      .result_value_id = result_id,
      .result_value_name = result_name,
      .result_type = type,
      .result_register =
          aarch64_codegen::RegisterOperand{
              .reg = type == bir::TypeKind::I32 ? aarch64_abi::w_register(0)
                                                : aarch64_abi::x_register(0),
              .role = aarch64_codegen::RegisterOperandRole::StoragePlan,
              .value_id = result_id,
              .value_name = result_name,
              .prepared_class = prepare::PreparedRegisterClass::General,
              .prepared_bank = prepare::PreparedRegisterBank::Gpr,
              .expected_view = type == bir::TypeKind::I32 ? aarch64_abi::RegisterView::W
                                                          : aarch64_abi::RegisterView::X,
              .contiguous_width = 1,
              .occupied_registers = {type == bir::TypeKind::I32 ? "w0" : "x0"},
          },
      .operand = operand,
      .supported_integer_operation =
          aarch64_codegen::is_scalar_unary_integer_operation(operation, type),
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

int unary_integer_records_preserve_typed_register_facts() {
  const auto source32 = prepared_register_operand(prepare::PreparedValueId{70},
                                                 c4c::ValueNameId{30},
                                                 bir::TypeKind::I32,
                                                 3);
  const auto source64 = prepared_register_operand(prepare::PreparedValueId{71},
                                                 c4c::ValueNameId{31},
                                                 bir::TypeKind::I64,
                                                 4);
  const auto neg = scalar_unary_record(aarch64_codegen::ScalarUnaryOperationKind::Neg,
                                       bir::TypeKind::I32,
                                       prepare::PreparedValueId{72},
                                       c4c::ValueNameId{32},
                                       source32);
  const auto bit_not = scalar_unary_record(aarch64_codegen::ScalarUnaryOperationKind::BitNot,
                                           bir::TypeKind::I64,
                                           prepare::PreparedValueId{73},
                                           c4c::ValueNameId{33},
                                           source64);
  const auto clz = scalar_unary_record(
      aarch64_codegen::ScalarUnaryOperationKind::CountLeadingZeros,
      bir::TypeKind::I32,
      prepare::PreparedValueId{74},
      c4c::ValueNameId{34},
      source32);
  const auto ctz = scalar_unary_record(
      aarch64_codegen::ScalarUnaryOperationKind::CountTrailingZeros,
      bir::TypeKind::I64,
      prepare::PreparedValueId{75},
      c4c::ValueNameId{35},
      source64);
  const auto bswap16 = scalar_unary_record(aarch64_codegen::ScalarUnaryOperationKind::ByteSwap,
                                           bir::TypeKind::I16,
                                           prepare::PreparedValueId{76},
                                           c4c::ValueNameId{36},
                                           prepared_register_operand(prepare::PreparedValueId{77},
                                                                     c4c::ValueNameId{37},
                                                                     bir::TypeKind::I16,
                                                                     5));

  if (!neg.supported_integer_operation || !bit_not.supported_integer_operation ||
      !clz.supported_integer_operation || !ctz.supported_integer_operation ||
      !bswap16.supported_integer_operation ||
      aarch64_codegen::scalar_unary_operation_kind_name(neg.operation) != "neg" ||
      aarch64_codegen::scalar_unary_operation_kind_name(bit_not.operation) != "bit_not" ||
      aarch64_codegen::scalar_unary_operation_kind_name(clz.operation) !=
          "count_leading_zeros" ||
      aarch64_codegen::scalar_unary_operation_kind_name(ctz.operation) !=
          "count_trailing_zeros" ||
      aarch64_codegen::scalar_unary_operation_kind_name(bswap16.operation) != "byte_swap") {
    return fail("expected selected unary integer operations to have typed diagnostic names");
  }
  if (aarch64_codegen::is_scalar_unary_integer_operation(
          aarch64_codegen::ScalarUnaryOperationKind::Neg,
          bir::TypeKind::I16) ||
      aarch64_codegen::is_scalar_unary_integer_operation(
          aarch64_codegen::ScalarUnaryOperationKind::Deferred,
          bir::TypeKind::I64)) {
    return fail("expected unary integer support to be limited to selected I32/I64 routes");
  }

  const auto neg_instruction = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_unary_instruction_record(neg));
  const auto bit_not_instruction = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_unary_instruction_record(bit_not));
  const auto clz_instruction = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_unary_instruction_record(clz));
  const auto ctz_instruction = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_unary_instruction_record(ctz));
  const auto bswap16_instruction = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_unary_instruction_record(bswap16));
  if (neg_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      bit_not_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      clz_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      ctz_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      bswap16_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      neg_instruction.opcode != aarch64_codegen::MachineOpcode::Neg ||
      bit_not_instruction.opcode != aarch64_codegen::MachineOpcode::BitNot ||
      clz_instruction.opcode != aarch64_codegen::MachineOpcode::CountLeadingZeros ||
      ctz_instruction.opcode != aarch64_codegen::MachineOpcode::CountTrailingZeros ||
      bswap16_instruction.opcode != aarch64_codegen::MachineOpcode::ByteSwap) {
    return fail("expected unary integer records to select typed scalar machine opcodes");
  }
  const auto* scalar =
      std::get_if<aarch64_codegen::ScalarInstructionRecord>(&neg_instruction.payload);
  if (scalar == nullptr || !scalar->scalar_unary.has_value() ||
      scalar->scalar_alu.has_value() || scalar->inputs.size() != 1 ||
      !scalar->result_register.has_value() ||
      scalar->result_register->expected_view != aarch64_abi::RegisterView::W ||
      neg_instruction.defs.size() != 1 || neg_instruction.uses.size() != 1 ||
      neg_instruction.defs.front().reg != aarch64_abi::w_register(0) ||
      neg_instruction.uses.front().reg != aarch64_abi::x_register(3)) {
    return fail("expected unary scalar instruction wrapper to preserve one input and result");
  }
  return 0;
}

int deferred_scalar_forms_are_explicit_records_not_generic_support() {
  if (aarch64_codegen::is_scalar_alu_integer_opcode(bir::BinaryOpcode::Mul) ||
      aarch64_codegen::is_scalar_alu_integer_opcode(bir::BinaryOpcode::Eq)) {
    return fail("expected non-slice opcodes to remain outside supported scalar ALU vocabulary");
  }
  if (aarch64_codegen::scalar_alu_operation_from_binary_opcode(bir::BinaryOpcode::Mul) !=
          aarch64_codegen::ScalarAluOperationKind::Mul ||
      aarch64_codegen::scalar_alu_operation_from_binary_opcode(bir::BinaryOpcode::SDiv) !=
          aarch64_codegen::ScalarAluOperationKind::Div ||
      aarch64_codegen::scalar_alu_operation_from_binary_opcode(bir::BinaryOpcode::Eq) !=
          aarch64_codegen::ScalarAluOperationKind::Deferred ||
      aarch64_codegen::scalar_alu_operation_kind_name(
          aarch64_codegen::ScalarAluOperationKind::Mul) != "mul" ||
      aarch64_codegen::scalar_alu_operation_kind_name(
          aarch64_codegen::ScalarAluOperationKind::Div) != "div" ||
      !aarch64_codegen::is_scalar_alu_integer_opcode(bir::BinaryOpcode::Shl) ||
      !aarch64_codegen::is_scalar_alu_integer_opcode(bir::BinaryOpcode::LShr) ||
      aarch64_codegen::scalar_alu_operation_from_binary_opcode(bir::BinaryOpcode::Shl) !=
          aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight ||
      aarch64_codegen::scalar_alu_operation_from_binary_opcode(bir::BinaryOpcode::LShr) !=
          aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight ||
      aarch64_codegen::scalar_alu_operation_kind_name(
          aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight) !=
          "logical_shift_right" ||
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
      deferred.supported_integer_operation || deferred.supported_floating_operation) {
    return fail("expected deferred record to preserve source opcode without claiming support");
  }

  return 0;
}

int shift_immediate_records_select_and_print_width_correct_alu_nodes() {
  auto shift_left = scalar_alu_record(
      aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight,
      bir::BinaryOpcode::Shl,
      bir::TypeKind::I32,
      prepare::PreparedValueId{80},
      c4c::ValueNameId{80},
      prepared_register_operand(prepare::PreparedValueId{81},
                                c4c::ValueNameId{81},
                                bir::TypeKind::I32,
                                1),
      aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
          .kind = aarch64_codegen::ImmediateKind::UnsignedInteger,
          .type = bir::TypeKind::I32,
          .signed_value = 1,
          .unsigned_value = 1,
      }));
  shift_left.supported_integer_operation = true;

  const auto left_instruction = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(shift_left));
  const auto* left_scalar =
      std::get_if<aarch64_codegen::ScalarInstructionRecord>(&left_instruction.payload);
  if (left_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      left_instruction.opcode != aarch64_codegen::MachineOpcode::LogicalShiftRight ||
      left_scalar == nullptr || !left_scalar->scalar_alu.has_value() ||
      left_scalar->source_binary_opcode != bir::BinaryOpcode::Shl) {
    return fail("expected shift-left record to select a scalar shift machine node");
  }
  const auto left_printed =
      aarch64_codegen::make_scalar_alu_print_lines(left_instruction, *left_scalar);
  if (!left_printed.lines.has_value() || left_printed.lines->size() != 1 ||
      left_printed.lines->front() != "lsl w0, w1, #1") {
    return fail("expected shift-left record to print width-correct lsl immediate");
  }

  auto shift_right = shift_left;
  shift_right.source_binary_opcode = bir::BinaryOpcode::LShr;
  const auto right_instruction = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(shift_right));
  const auto* right_scalar =
      std::get_if<aarch64_codegen::ScalarInstructionRecord>(&right_instruction.payload);
  if (right_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      right_instruction.opcode != aarch64_codegen::MachineOpcode::LogicalShiftRight ||
      right_scalar == nullptr || !right_scalar->scalar_alu.has_value() ||
      right_scalar->source_binary_opcode != bir::BinaryOpcode::LShr) {
    return fail("expected logical-shift-right record to select a scalar shift machine node");
  }
  const auto right_printed =
      aarch64_codegen::make_scalar_alu_print_lines(right_instruction, *right_scalar);
  if (!right_printed.lines.has_value() || right_printed.lines->size() != 1 ||
      right_printed.lines->front() != "lsr w0, w1, #1") {
    return fail("expected logical-shift-right record to print width-correct lsr immediate");
  }

  return 0;
}

int unsigned_power_of_two_reduction_records_select_structured_alu_nodes() {
  auto shift = scalar_alu_record(
      aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight,
      bir::BinaryOpcode::UDiv,
      bir::TypeKind::I64,
      prepare::PreparedValueId{90},
      c4c::ValueNameId{90},
      prepared_register_operand(prepare::PreparedValueId{91},
                                c4c::ValueNameId{91},
                                bir::TypeKind::I64,
                                1),
      aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
          .kind = aarch64_codegen::ImmediateKind::UnsignedInteger,
          .type = bir::TypeKind::I64,
          .signed_value = 3,
          .unsigned_value = 3,
      }));
  shift.supported_integer_operation = true;
  auto mask = scalar_alu_record(
      aarch64_codegen::ScalarAluOperationKind::And,
      bir::BinaryOpcode::URem,
      bir::TypeKind::I32,
      prepare::PreparedValueId{92},
      c4c::ValueNameId{92},
      prepared_register_operand(prepare::PreparedValueId{93},
                                c4c::ValueNameId{93},
                                bir::TypeKind::I32,
                                2),
      aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
          .kind = aarch64_codegen::ImmediateKind::UnsignedInteger,
          .type = bir::TypeKind::I32,
          .signed_value = 15,
          .unsigned_value = 15,
      }));
  mask.supported_integer_operation = true;

  const auto shift_node = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(shift));
  const auto mask_node = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(mask));
  if (shift_node.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      mask_node.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      shift_node.opcode != aarch64_codegen::MachineOpcode::LogicalShiftRight ||
      mask_node.opcode != aarch64_codegen::MachineOpcode::And) {
    return fail("expected unsigned reduction records to select structured ALU opcodes");
  }
  return 0;
}

int floating_records_preserve_fp_simd_register_facts_without_integer_support() {
  const auto f32_mul = scalar_alu_record(
      aarch64_codegen::ScalarAluOperationKind::Mul,
      bir::BinaryOpcode::Mul,
      bir::TypeKind::F32,
      prepare::PreparedValueId{50},
      c4c::ValueNameId{17},
      prepared_fpr_operand(prepare::PreparedValueId{51}, c4c::ValueNameId{18}, bir::TypeKind::F32, 1),
      prepared_fpr_operand(prepare::PreparedValueId{52}, c4c::ValueNameId{19}, bir::TypeKind::F32, 2));
  const auto f64_div = scalar_alu_record(
      aarch64_codegen::ScalarAluOperationKind::Div,
      bir::BinaryOpcode::SDiv,
      bir::TypeKind::F64,
      prepare::PreparedValueId{53},
      c4c::ValueNameId{20},
      prepared_fpr_operand(prepare::PreparedValueId{54}, c4c::ValueNameId{21}, bir::TypeKind::F64, 3),
      prepared_fpr_operand(prepare::PreparedValueId{55}, c4c::ValueNameId{22}, bir::TypeKind::F64, 4));

  if (!f32_mul.supported_floating_operation || f32_mul.supported_integer_operation ||
      !f64_div.supported_floating_operation || f64_div.supported_integer_operation) {
    return fail("expected F32/F64 ALU records to claim only floating support");
  }
  const auto f32_node =
      aarch64_codegen::make_scalar_instruction(aarch64_codegen::make_scalar_alu_instruction_record(f32_mul));
  const auto f64_node =
      aarch64_codegen::make_scalar_instruction(aarch64_codegen::make_scalar_alu_instruction_record(f64_div));
  if (f32_node.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      f64_node.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      f32_node.opcode != aarch64_codegen::MachineOpcode::Mul ||
      f64_node.opcode != aarch64_codegen::MachineOpcode::Div) {
    return fail("expected F32/F64 ALU records to select structured machine nodes");
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
  if (const int status = unary_integer_records_preserve_typed_register_facts();
      status != 0) {
    return status;
  }
  if (const int status = deferred_scalar_forms_are_explicit_records_not_generic_support();
      status != 0) {
    return status;
  }
  if (const int status = shift_immediate_records_select_and_print_width_correct_alu_nodes();
      status != 0) {
    return status;
  }
  if (const int status = unsigned_power_of_two_reduction_records_select_structured_alu_nodes();
      status != 0) {
    return status;
  }
  if (const int status = floating_records_preserve_fp_simd_register_facts_without_integer_support();
      status != 0) {
    return status;
  }
  return 0;
}
