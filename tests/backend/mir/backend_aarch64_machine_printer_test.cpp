#include "src/backend/mir/aarch64/codegen/machine_printer.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace {

namespace aarch64_abi = c4c::backend::aarch64::abi;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(const std::string& message) {
  std::cerr << message << "\n";
  return 1;
}

int expect_equal(std::string_view actual, std::string_view expected, std::string_view context) {
  if (actual != expected) {
    return fail(std::string(context) + " expected '" + std::string(expected) + "', got '" +
                std::string(actual) + "'");
  }
  return 0;
}

int expect_assembly(std::string_view actual,
                    std::string_view expected_from_helpers,
                    std::string_view expected_canonical,
                    std::string_view context) {
  if (expected_from_helpers != expected_canonical) {
    return fail(std::string(context) +
                " helper-derived assembly drifted from canonical expected spelling");
  }
  return expect_equal(actual, expected_from_helpers, context);
}

aarch64_codegen::RegisterOperand xreg(unsigned index) {
  return aarch64_codegen::RegisterOperand{
      .reg = aarch64_abi::x_register(static_cast<std::uint8_t>(index)),
      .role = aarch64_codegen::RegisterOperandRole::SpillAuthority,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = aarch64_abi::RegisterView::X,
      .contiguous_width = 1,
  };
}

aarch64_codegen::RegisterOperand wreg(unsigned index) {
  return aarch64_codegen::RegisterOperand{
      .reg = aarch64_abi::w_register(static_cast<std::uint8_t>(index)),
      .role = aarch64_codegen::RegisterOperandRole::PreparedAssignment,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = aarch64_abi::RegisterView::W,
      .contiguous_width = 1,
  };
}

aarch64_codegen::MemoryOperand frame_slot(std::int64_t offset) {
  return aarch64_codegen::MemoryOperand{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
      .function_name = c4c::FunctionNameId{2},
      .block_label = c4c::BlockLabelId{3},
      .base_kind = aarch64_codegen::MemoryBaseKind::FrameSlot,
      .frame_slot_id = prepare::PreparedFrameSlotId{4},
      .byte_offset = offset,
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = 8,
      .align_bytes = 8,
      .address_space = bir::AddressSpace::Default,
      .can_use_base_plus_offset = true,
  };
}

int selected_spill_reload_nodes_print_gnu_aarch64_text() {
  int source_op = 0;
  const auto spill = aarch64_codegen::make_spill_reload_instruction(
      aarch64_codegen::SpillReloadInstructionRecord{
          .value_id = prepare::PreparedValueId{10},
          .value_name = c4c::ValueNameId{11},
          .value_type = bir::TypeKind::I64,
          .op_kind = prepare::PreparedSpillReloadOpKind::Spill,
          .pseudo_kind = aarch64_codegen::MachinePseudoKind::SpillToSlot,
          .slot = frame_slot(16),
          .scratch = xreg(9),
          .occupied_scratch_register_references = {aarch64_abi::x_register(9)},
          .occupied_scratch_registers = {"x9"},
          .scratch_register_authority = std::size_t{0},
          .slot_id = prepare::PreparedFrameSlotId{4},
          .stack_offset_bytes = std::size_t{16},
          .stack_offset_is_prepared_snapshot = true,
          .source_spill_reload =
              reinterpret_cast<const prepare::PreparedSpillReloadOp*>(&source_op),
      });
  const auto reload = aarch64_codegen::make_spill_reload_instruction(
      aarch64_codegen::SpillReloadInstructionRecord{
          .value_id = prepare::PreparedValueId{10},
          .value_name = c4c::ValueNameId{11},
          .value_type = bir::TypeKind::I64,
          .op_kind = prepare::PreparedSpillReloadOpKind::Reload,
          .pseudo_kind = aarch64_codegen::MachinePseudoKind::ReloadFromSlot,
          .slot = frame_slot(16),
          .scratch = xreg(9),
          .occupied_scratch_register_references = {aarch64_abi::x_register(9)},
          .occupied_scratch_registers = {"x9"},
          .scratch_register_authority = std::size_t{0},
          .slot_id = prepare::PreparedFrameSlotId{4},
          .stack_offset_bytes = std::size_t{16},
          .stack_offset_is_prepared_snapshot = true,
          .source_spill_reload =
              reinterpret_cast<const prepare::PreparedSpillReloadOp*>(&source_op),
      });

  const auto result = aarch64_codegen::print_machine_instruction_nodes({spill, reload});
  if (!result.ok || !result.diagnostic.empty()) {
    return fail("expected selected spill/reload nodes to print canonical AArch64 text");
  }
  const auto spill_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(spill);
  const auto reload_mnemonic =
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(reload);
  const std::string expected =
      "    " + std::string(spill_mnemonic) + " x9, [sp, #16]\n" +
      "    " + std::string(reload_mnemonic) + " x9, [sp, #16]\n";
  if (const int check = expect_assembly(result.assembly,
                                        expected,
                                        "    str x9, [sp, #16]\n    ldr x9, [sp, #16]\n",
                                        "spill/reload helper-printer drift guard");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(
          spill_mnemonic, "str", "spill helper mnemonic");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(
          reload_mnemonic, "ldr", "reload helper mnemonic");
      check != 0) {
    return check;
  }
  return 0;
}

int selected_branch_and_store_nodes_print_without_semantic_roundtrip() {
  const auto condition = aarch64_codegen::make_register_operand(xreg(1));
  const auto branch = aarch64_codegen::make_branch_instruction(
      aarch64_codegen::BranchInstructionRecord{
          .target =
              aarch64_codegen::BranchTargetOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .block_label = c4c::BlockLabelId{7},
                  .function_name = c4c::FunctionNameId{2},
                  .condition_value_id = prepare::PreparedValueId{20},
              },
          .target_pair =
              aarch64_codegen::BranchTargetPairRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .true_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{7},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{20},
                      },
                  .false_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{8},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{20},
                      },
              },
          .condition = condition,
          .condition_record =
              aarch64_codegen::BranchConditionRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .form = aarch64_codegen::BranchConditionForm::MaterializedBool,
                  .condition_value_id = prepare::PreparedValueId{20},
                  .condition_value_name = c4c::ValueNameId{21},
                  .condition_type = bir::TypeKind::I1,
              },
          .conditional = true,
      });

  auto store_address = frame_slot(24);
  store_address.stored_value_id = prepare::PreparedValueId{22};
  store_address.stored_value_name = c4c::ValueNameId{23};
  const auto store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address = store_address,
          .value = aarch64_codegen::make_register_operand(xreg(10)),
          .value_type = bir::TypeKind::I64,
      });

  const auto result = aarch64_codegen::print_machine_instruction_nodes({branch, store});
  if (!result.ok) {
    return fail("expected branch and store nodes to print from structured operands: " +
                result.diagnostic);
  }
  const auto conditional_branch_mnemonic =
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(branch);
  const auto unconditional_branch_mnemonic = aarch64_codegen::machine_printer_mnemonic_kind_name(
      aarch64_codegen::MachinePrinterMnemonicKind::Branch);
  const auto store_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(store);
  const std::string expected =
      "    " + std::string(conditional_branch_mnemonic) + " x1, .LBB2_7\n" +
      "    " + std::string(unconditional_branch_mnemonic) + " .LBB2_8\n" +
      "    " + std::string(store_mnemonic) + " x10, [sp, #24]\n";
  if (const int check = expect_assembly(result.assembly,
                                        expected,
                                        "    cbnz x1, .LBB2_7\n"
                                        "    b .LBB2_8\n"
                                        "    str x10, [sp, #24]\n",
                                        "branch/store helper-printer drift guard");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(
          conditional_branch_mnemonic, "cbnz", "conditional branch helper mnemonic");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(
          unconditional_branch_mnemonic,
          "b",
          "unconditional branch helper mnemonic");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(
          store_mnemonic, "str", "store helper mnemonic");
      check != 0) {
    return check;
  }
  return 0;
}

int selected_scalar_add_sub_and_register_return_print_from_structured_operands() {
  const auto add = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{40},
          .result_value_name = c4c::ValueNameId{41},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(0),
          .lhs = aarch64_codegen::make_register_operand(xreg(1)),
          .rhs = aarch64_codegen::make_register_operand(xreg(2)),
          .supported_integer_operation = true,
      }));
  const auto sub = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Sub,
          .source_binary_opcode = bir::BinaryOpcode::Sub,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{42},
          .result_value_name = c4c::ValueNameId{43},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(0),
          .lhs = aarch64_codegen::make_register_operand(wreg(1)),
          .rhs = aarch64_codegen::make_register_operand(wreg(2)),
          .supported_integer_operation = true,
      }));
  const auto ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{
          .value = aarch64_codegen::make_register_operand(wreg(0)),
          .value_type = bir::TypeKind::I32,
      });

  const auto result = aarch64_codegen::print_machine_instruction_nodes({add, sub, ret});
  if (!result.ok) {
    return fail("expected scalar add/sub and register return to print from structured operands: " +
                result.diagnostic);
  }
  const auto add_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(add);
  const auto sub_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(sub);
  const auto return_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(ret);
  const std::string expected =
      "    " + std::string(add_mnemonic) + " x0, x1, x2\n" +
      "    " + std::string(sub_mnemonic) + " w0, w1, w2\n" +
      "    " + std::string(return_mnemonic) + "\n";
  if (const int check = expect_assembly(result.assembly,
                                        expected,
                                        "    add x0, x1, x2\n"
                                        "    sub w0, w1, w2\n"
                                        "    ret\n",
                                        "scalar add/sub helper-printer drift guard");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(add_mnemonic, "add", "add helper mnemonic"); check != 0) {
    return check;
  }
  if (const int check = expect_equal(sub_mnemonic, "sub", "sub helper mnemonic"); check != 0) {
    return check;
  }
  if (const int check =
          expect_equal(return_mnemonic, "ret", "register return helper primary mnemonic");
      check != 0) {
    return check;
  }
  return 0;
}

int selected_immediate_return_node_prints_callable_epilogue() {
  const auto ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{
          .value = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::SignedInteger,
                  .type = bir::TypeKind::I32,
                  .signed_value = 0,
              }),
          .value_type = bir::TypeKind::I32,
      });

  const auto result = aarch64_codegen::print_machine_instruction_nodes({ret});
  if (!result.ok) {
    return fail("expected selected immediate return node to print AArch64 return text: " +
                result.diagnostic);
  }
  const auto move_mnemonic = aarch64_codegen::machine_instruction_auxiliary_printer_mnemonic(ret);
  const auto return_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(ret);
  const std::string expected =
      "    " + std::string(move_mnemonic) + " w0, #0\n" +
      "    " + std::string(return_mnemonic) + "\n";
  if (const int check = expect_assembly(result.assembly,
                                        expected,
                                        "    mov w0, #0\n    ret\n",
                                        "return helper-printer drift guard");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(
          move_mnemonic, "mov", "return helper auxiliary mnemonic");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(
          return_mnemonic, "ret", "return helper primary mnemonic");
      check != 0) {
    return check;
  }
  return 0;
}

int selected_scalar_add_with_immediate_operands_prints_structured_add() {
  const auto add = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{44},
          .result_value_name = c4c::ValueNameId{45},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(0),
          .lhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = 2,
          }),
          .rhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = 3,
          }),
          .supported_integer_operation = true,
      }));
  const auto ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{
          .value = aarch64_codegen::make_register_operand(wreg(0)),
          .value_type = bir::TypeKind::I32,
      });

  const auto result = aarch64_codegen::print_machine_instruction_nodes({add, ret});
  if (!result.ok) {
    return fail("expected scalar add with immediate operands to print structured add: " +
                result.diagnostic);
  }
  const auto add_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(add);
  const auto return_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(ret);
  const std::string expected =
      "    mov w0, #2\n"
      "    " +
      std::string(add_mnemonic) + " w0, w0, #3\n" +
      "    " + std::string(return_mnemonic) + "\n";
  if (const int check = expect_assembly(result.assembly,
                                        expected,
                                        "    mov w0, #2\n"
                                        "    add w0, w0, #3\n"
                                        "    ret\n",
                                        "scalar immediate add helper-printer drift guard");
      check != 0) {
    return check;
  }
  return 0;
}

int selected_scalar_add_sub_reject_nonencodable_immediates() {
  const auto add_out_of_range = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{46},
          .result_value_name = c4c::ValueNameId{47},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(0),
          .lhs = aarch64_codegen::make_register_operand(wreg(1)),
          .rhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = 4096,
          }),
          .supported_integer_operation = true,
      }));
  const auto out_of_range_result =
      aarch64_codegen::print_machine_instruction_node(add_out_of_range);
  if (out_of_range_result.ok ||
      out_of_range_result.diagnostic.find("plain #imm encoding range 0..4095") ==
          std::string::npos) {
    return fail("expected scalar add with out-of-range immediate to fail closed");
  }

  const auto sub_negative = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Sub,
          .source_binary_opcode = bir::BinaryOpcode::Sub,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{48},
          .result_value_name = c4c::ValueNameId{49},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(0),
          .lhs = aarch64_codegen::make_register_operand(wreg(1)),
          .rhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = -1,
          }),
          .supported_integer_operation = true,
      }));
  const auto negative_result = aarch64_codegen::print_machine_instruction_node(sub_negative);
  if (negative_result.ok ||
      negative_result.diagnostic.find("plain #imm encoding range 0..4095") ==
          std::string::npos) {
    return fail("expected scalar sub with negative immediate to fail closed");
  }

  return 0;
}

int unsupported_surfaces_statuses_and_missing_operands_fail_closed() {
  const auto assembler = aarch64_codegen::make_assembler_instruction(
      aarch64_codegen::AssemblerInstructionRecord{});
  const auto assembler_result = aarch64_codegen::print_machine_instruction_node(assembler);
  if (assembler_result.ok ||
      assembler_result.diagnostic.find("surface machine_instruction_node") ==
          std::string::npos) {
    return fail("expected external assembler input to fail closed before printing");
  }

  const auto unsupported = aarch64_codegen::make_unsupported_machine_instruction(
      aarch64_codegen::InstructionFamily::Memory,
      aarch64_codegen::MachineNodeSelectionStatus::DeferredUnsupported,
      "fixture unsupported");
  const auto unsupported_result =
      aarch64_codegen::print_machine_instruction_node(unsupported);
  if (unsupported_result.ok ||
      unsupported_result.diagnostic.find("deferred_unsupported: fixture unsupported") ==
          std::string::npos) {
    return fail("expected non-selected machine node to report selection diagnostic");
  }

  const auto scalar = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{30},
          .result_value_name = c4c::ValueNameId{31},
          .result_type = bir::TypeKind::I64,
          .lhs = aarch64_codegen::make_register_operand(xreg(2)),
          .rhs = aarch64_codegen::make_register_operand(xreg(3)),
          .supported_integer_operation = true,
      }));
  const auto scalar_result = aarch64_codegen::print_machine_instruction_node(scalar);
  if (scalar_result.ok ||
      scalar_result.diagnostic.find("missing a structured destination register operand") ==
          std::string::npos) {
    return fail("expected selected scalar without destination register to fail closed");
  }

  const auto scalar_with_unprintable_sub_operands = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Sub,
          .source_binary_opcode = bir::BinaryOpcode::Sub,
          .operand_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{32},
          .result_value_name = c4c::ValueNameId{33},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(0),
          .lhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I64,
              .signed_value = 1,
          }),
          .rhs = aarch64_codegen::make_register_operand(xreg(2)),
          .supported_integer_operation = true,
      }));
  const auto scalar_with_unprintable_sub_operands_result =
      aarch64_codegen::print_machine_instruction_node(scalar_with_unprintable_sub_operands);
  if (scalar_with_unprintable_sub_operands_result.ok ||
      scalar_with_unprintable_sub_operands_result.diagnostic.find(
          "scalar sub with an immediate lhs and register rhs is not printable") ==
          std::string::npos) {
    return fail("expected selected scalar with unprintable sub operands to fail closed");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int result = selected_spill_reload_nodes_print_gnu_aarch64_text(); result != 0) {
    return result;
  }
  if (const int result = selected_branch_and_store_nodes_print_without_semantic_roundtrip();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_scalar_add_sub_and_register_return_print_from_structured_operands();
      result != 0) {
    return result;
  }
  if (const int result = selected_immediate_return_node_prints_callable_epilogue();
      result != 0) {
    return result;
  }
  if (const int result = selected_scalar_add_with_immediate_operands_prints_structured_add();
      result != 0) {
    return result;
  }
  if (const int result = selected_scalar_add_sub_reject_nonencodable_immediates();
      result != 0) {
    return result;
  }
  if (const int result = unsupported_surfaces_statuses_and_missing_operands_fail_closed();
      result != 0) {
    return result;
  }
  return 0;
}
