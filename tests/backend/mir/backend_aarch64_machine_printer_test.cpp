#include "src/backend/mir/aarch64/codegen/machine_printer.hpp"
#include "src/backend/mir/printer.hpp"

#include <iostream>
#include <initializer_list>
#include <string>
#include <string_view>

namespace {

namespace aarch64_abi = c4c::backend::aarch64::abi;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;
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

mir::MachinePrintResult print_common_instruction_nodes(
    std::initializer_list<aarch64_codegen::InstructionRecord> instructions) {
  mir::MachineFunction<aarch64_codegen::InstructionRecord> function;
  function.function_name = c4c::FunctionNameId{2};
  auto& block = function.blocks.emplace_back();
  block.block_label = c4c::BlockLabelId{3};
  block.index = 0;
  for (const auto& instruction : instructions) {
    block.instructions.push_back(
        mir::MachineInstruction<aarch64_codegen::InstructionRecord>{.target = instruction});
  }

  const auto target_printer = aarch64_codegen::MachineInstructionPrinter{};
  return mir::print_machine_function(function, target_printer);
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

  const auto result = print_common_instruction_nodes({spill, reload});
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
                                        "spill/reload common-printer drift guard");
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

  const auto result = print_common_instruction_nodes({branch, store});
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
                                        "branch/store common-printer drift guard");
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

int selected_structured_memory_subset_prints_loads_and_stores() {
  auto load_address = frame_slot(32);
  load_address.result_value_id = prepare::PreparedValueId{24};
  load_address.result_value_name = c4c::ValueNameId{25};
  const auto load = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Load,
          .address = load_address,
          .result_value_id = prepare::PreparedValueId{24},
          .result_value_name = c4c::ValueNameId{25},
          .value_type = bir::TypeKind::I32,
          .result_register = wreg(0),
      });

  auto frame_store_address = frame_slot(40);
  frame_store_address.size_bytes = 4;
  frame_store_address.align_bytes = 4;
  frame_store_address.stored_value_id = prepare::PreparedValueId{26};
  frame_store_address.stored_value_name = c4c::ValueNameId{27};
  const auto frame_store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address = frame_store_address,
          .value = aarch64_codegen::make_register_operand(wreg(1)),
          .value_type = bir::TypeKind::I32,
      });

  const auto pointer_base = xreg(2);
  const auto pointer_store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address =
              aarch64_codegen::MemoryOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
                  .function_name = c4c::FunctionNameId{2},
                  .block_label = c4c::BlockLabelId{3},
                  .stored_value_id = prepare::PreparedValueId{28},
                  .stored_value_name = c4c::ValueNameId{29},
                  .base_kind = aarch64_codegen::MemoryBaseKind::PointerValue,
                  .base_register = pointer_base,
                  .pointer_value_name = c4c::ValueNameId{30},
                  .pointer_value_id = prepare::PreparedValueId{31},
                  .byte_offset = 24,
                  .size_bytes = 4,
                  .align_bytes = 4,
                  .address_space = bir::AddressSpace::Tls,
                  .can_use_base_plus_offset = true,
              },
          .value = aarch64_codegen::make_register_operand(wreg(3)),
          .value_type = bir::TypeKind::I32,
      });

  const auto result = print_common_instruction_nodes({load, frame_store, pointer_store});
  if (!result.ok) {
    return fail("expected selected structured memory subset to print: " + result.diagnostic);
  }
  const std::string expected =
      "    ldr w0, [sp, #32]\n"
      "    str w1, [sp, #40]\n"
      "    str w3, [x2, #24]\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "structured memory load/store printer drift guard");
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

  const auto result = print_common_instruction_nodes({add, sub, ret});
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
                                        "scalar add/sub common-printer drift guard");
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

int selected_simple_integer_casts_print_from_structured_operands() {
  const auto sext = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::SignExtend,
          .source_cast_opcode = bir::CastOpcode::SExt,
          .source_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{52},
          .result_value_name = c4c::ValueNameId{53},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(0),
          .source = aarch64_codegen::make_register_operand(wreg(1)),
          .supported_simple_integer_cast = true,
      }));
  const auto zext = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::ZeroExtend,
          .source_cast_opcode = bir::CastOpcode::ZExt,
          .source_type = bir::TypeKind::I1,
          .result_value_id = prepare::PreparedValueId{54},
          .result_value_name = c4c::ValueNameId{55},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(2),
          .source = aarch64_codegen::make_register_operand(wreg(3)),
          .supported_simple_integer_cast = true,
      }));
  const auto trunc = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::Truncate,
          .source_cast_opcode = bir::CastOpcode::Trunc,
          .source_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{56},
          .result_value_name = c4c::ValueNameId{57},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(4),
          .source = aarch64_codegen::make_register_operand(xreg(5)),
          .supported_simple_integer_cast = true,
      }));

  const auto result = print_common_instruction_nodes({sext, zext, trunc});
  if (!result.ok) {
    return fail("expected simple integer casts to print from structured operands: " +
                result.diagnostic);
  }
  const std::string expected =
      "    sxtw x0, w1\n"
      "    ubfx w2, w3, #0, #1\n"
      "    mov w4, w5\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "simple integer cast common-printer drift guard");
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

  const auto result = print_common_instruction_nodes({ret});
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
                                        "return common-printer drift guard");
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

int common_mir_printer_can_delegate_to_aarch64_target_spelling_adapter() {
  const auto ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{
          .value = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::SignedInteger,
                  .type = bir::TypeKind::I32,
                  .signed_value = 7,
              }),
          .value_type = bir::TypeKind::I32,
      });

  mir::MachineFunction<aarch64_codegen::InstructionRecord> function;
  function.function_name = c4c::FunctionNameId{11};
  function.blocks.push_back(mir::MachineBlock<aarch64_codegen::InstructionRecord>{
      .block_label = c4c::BlockLabelId{13},
      .index = 0,
      .instructions =
          {
              mir::MachineInstruction<aarch64_codegen::InstructionRecord>{
                  .target = ret,
              },
          },
  });

  const auto target_printer = aarch64_codegen::MachineInstructionPrinter{};
  const auto result = mir::print_machine_function(function, target_printer);
  if (!result.ok) {
    return fail("expected common MIR printer to delegate AArch64 target spelling: " +
                result.diagnostic);
  }
  const auto move_mnemonic = aarch64_codegen::machine_instruction_auxiliary_printer_mnemonic(ret);
  const auto return_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(ret);
  const auto line_payloads = aarch64_codegen::print_machine_instruction_line_payloads(ret);
  if (!line_payloads.ok || line_payloads.instruction_lines.size() != 2 ||
      line_payloads.instruction_lines[0] != "mov w0, #7" ||
      line_payloads.instruction_lines[1] != "ret") {
    return fail("expected AArch64 target spelling hook to return unindented line payloads");
  }
  const std::string expected =
      "    " + std::string(move_mnemonic) + " w0, #7\n" +
      "    " + std::string(return_mnemonic) + "\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    mov w0, #7\n    ret\n",
                         "common MIR printer AArch64 adapter drift guard");
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

  const auto result = print_common_instruction_nodes({add, ret});
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
                                        "scalar immediate add common-printer drift guard");
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
      aarch64_codegen::print_machine_instruction_line_payloads(add_out_of_range);
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
  const auto negative_result =
      aarch64_codegen::print_machine_instruction_line_payloads(sub_negative);
  if (negative_result.ok ||
      negative_result.diagnostic.find("plain #imm encoding range 0..4095") ==
          std::string::npos) {
    return fail("expected scalar sub with negative immediate to fail closed");
  }

  return 0;
}

int selected_simple_integer_casts_reject_missing_or_unsupported_facts() {
  const auto missing_source = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::SignExtend,
          .source_cast_opcode = bir::CastOpcode::SExt,
          .source_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{58},
          .result_value_name = c4c::ValueNameId{59},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(0),
          .source = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = 1,
          }),
          .supported_simple_integer_cast = true,
      }));
  const auto missing_source_result =
      aarch64_codegen::print_machine_instruction_line_payloads(missing_source);
  if (missing_source_result.ok ||
      missing_source_result.diagnostic.find("structured register source operand") ==
          std::string::npos) {
    return fail("expected simple integer cast without source register to fail closed");
  }

  const auto unsupported_type = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::SignExtend,
          .source_cast_opcode = bir::CastOpcode::SExt,
          .source_type = bir::TypeKind::F128,
          .result_value_id = prepare::PreparedValueId{60},
          .result_value_name = c4c::ValueNameId{61},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(0),
          .source = aarch64_codegen::make_register_operand(xreg(1)),
          .supported_simple_integer_cast = true,
      }));
  const auto unsupported_type_result =
      aarch64_codegen::print_machine_instruction_line_payloads(unsupported_type);
  if (unsupported_type_result.ok ||
      unsupported_type_result.diagnostic.find("supported integer source/result width") ==
          std::string::npos) {
    return fail("expected simple integer cast with unsupported type to fail closed");
  }

  return 0;
}

int selected_direct_call_prints_from_prepared_call_provenance() {
  const prepare::PreparedCallPlan prepared_call{
      .block_index = 0,
      .instruction_index = 1,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"actual_function"},
      .preserved_values = {prepare::PreparedCallPreservedValue{
          .value_id = prepare::PreparedValueId{77},
          .value_name = c4c::ValueNameId{14},
          .route = prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
          .callee_saved_save_index = std::size_t{0},
          .contiguous_width = 1,
          .register_name = std::string{"x19"},
          .register_bank = prepare::PreparedRegisterBank::Gpr,
          .occupied_register_names = {"x19"},
      }},
      .clobbered_registers = {prepare::PreparedClobberedRegister{
          .bank = prepare::PreparedRegisterBank::Gpr,
          .register_name = "x13",
          .contiguous_width = 1,
          .occupied_register_names = {"x13"},
      }},
  };
  const auto call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{9},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "actual_function",
          .wrapper_kind = prepared_call.wrapper_kind,
          .preserved_values = prepared_call.preserved_values,
          .clobbered_registers = prepared_call.clobbered_registers,
          .source_call = &prepared_call,
          .calling_convention = bir::CallingConv::C,
      });

  if (call.preserves.size() != 1 ||
      call.preserves.front().reg != aarch64_abi::x_register(19) ||
      call.preserves.front().value_id != prepare::PreparedValueId{77}) {
    return fail("expected direct-call printer fixture to carry prepared preserved-value effect");
  }
  if (call.clobbers.size() != 1 ||
      call.clobbers.front().reg != aarch64_abi::x_register(13)) {
    return fail("expected direct-call printer fixture to carry prepared clobber effect");
  }
  const auto result = print_common_instruction_nodes({call});
  if (!result.ok) {
    return fail("expected direct call node to print from prepared provenance: " +
                result.diagnostic);
  }
  const auto call_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(call);
  const std::string expected = "    " + std::string(call_mnemonic) + " actual_function\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    bl actual_function\n",
                         "direct-call common-printer drift guard");
}

int selected_memory_return_call_prints_call_and_preserves_storage_effect() {
  const prepare::PreparedMemoryReturnPlan memory_return{
      .sret_arg_index = std::size_t{0},
      .storage_slot_name = c4c::SlotNameId{8},
      .encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
      .slot_id = prepare::PreparedFrameSlotId{9},
      .stack_offset_bytes = std::size_t{32},
      .size_bytes = 16,
      .align_bytes = 8,
  };
  const prepare::PreparedCallPlan prepared_call{
      .block_index = 0,
      .instruction_index = 1,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"make_large"},
      .memory_return = memory_return,
  };
  const auto call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{9},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "make_large",
          .wrapper_kind = prepared_call.wrapper_kind,
          .memory_return = prepared_call.memory_return,
          .memory_return_storage =
              aarch64_codegen::MemoryOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::MachineInstructionNode,
                  .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
                  .function_name = c4c::FunctionNameId{2},
                  .block_label = c4c::BlockLabelId{3},
                  .instruction_index = 1,
                  .base_kind = aarch64_codegen::MemoryBaseKind::FrameSlot,
                  .frame_slot_id = prepare::PreparedFrameSlotId{9},
                  .byte_offset = 32,
                  .byte_offset_is_prepared_snapshot = true,
                  .size_bytes = 16,
                  .align_bytes = 8,
                  .can_use_base_plus_offset = true,
              },
          .source_call = &prepared_call,
          .calling_convention = bir::CallingConv::C,
      });

  if (call.defs.size() != 1 ||
      call.defs.front().kind != aarch64_codegen::MachineEffectResourceKind::Memory ||
      call.defs.front().frame_slot_id != prepare::PreparedFrameSlotId{9} ||
      call.side_effects.size() != 2 ||
      call.side_effects.back() != aarch64_codegen::MachineSideEffectKind::MemoryWrite) {
    return fail("expected memory-return call printer fixture to carry prepared storage effect");
  }
  const auto result = print_common_instruction_nodes({call});
  if (!result.ok) {
    return fail("expected memory-return call node to print the call itself: " +
                result.diagnostic);
  }
  const auto call_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(call);
  const std::string expected = "    " + std::string(call_mnemonic) + " make_large\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    bl make_large\n",
                         "memory-return call common-printer drift guard");
}

int selected_indirect_call_prints_from_prepared_register_callee() {
  const prepare::PreparedCallPlan prepared_call{
      .block_index = 0,
      .instruction_index = 1,
      .wrapper_kind = prepare::PreparedCallWrapperKind::Indirect,
      .is_indirect = true,
      .indirect_callee =
          prepare::PreparedIndirectCalleePlan{
              .value_name = c4c::ValueNameId{52},
              .value_id = prepare::PreparedValueId{51},
              .encoding = prepare::PreparedStorageEncodingKind::Register,
              .bank = prepare::PreparedRegisterBank::Gpr,
              .register_name = std::string{"x9"},
          },
  };
  const auto call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .indirect_callee =
              aarch64_codegen::make_register_operand(aarch64_codegen::RegisterOperand{
                  .reg = aarch64_abi::x_register(9),
                  .role = aarch64_codegen::RegisterOperandRole::CallAbi,
                  .value_id = prepare::PreparedValueId{51},
                  .value_name = c4c::ValueNameId{52},
                  .prepared_class = prepare::PreparedRegisterClass::General,
                  .prepared_bank = prepare::PreparedRegisterBank::Gpr,
                  .expected_view = aarch64_abi::RegisterView::X,
                  .contiguous_width = 1,
                  .occupied_registers = {"x9"},
              }),
          .wrapper_kind = prepared_call.wrapper_kind,
          .prepared_indirect_callee = prepared_call.indirect_callee,
          .source_call = &prepared_call,
          .calling_convention = bir::CallingConv::C,
          .is_indirect = true,
      });

  const auto result = print_common_instruction_nodes({call});
  if (!result.ok) {
    return fail("expected indirect call node to print from prepared register callee: " +
                result.diagnostic);
  }
  const auto call_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(call);
  const std::string expected = "    " + std::string(call_mnemonic) + " x9\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    blr x9\n",
                         "indirect-call common-printer drift guard");
}

int selected_call_boundary_register_move_prints_prepared_mov() {
  const prepare::PreparedMoveBundle call_boundary_bundle{
      .function_name = c4c::FunctionNameId{2},
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = 0,
      .instruction_index = 4,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = prepare::PreparedValueId{70},
                  .to_value_id = prepare::PreparedValueId{70},
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{0},
                  .destination_register_name = std::string{"x0"},
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
              },
          },
      .abi_bindings =
          {
              prepare::PreparedAbiBinding{
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{0},
                  .destination_register_name = std::string{"x0"},
              },
          },
  };
  const auto move = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = call_boundary_bundle.function_name,
          .phase = call_boundary_bundle.phase,
          .block_index = call_boundary_bundle.block_index,
          .instruction_index = call_boundary_bundle.instruction_index,
          .move = call_boundary_bundle.moves.front(),
          .source_register = xreg(2),
          .destination_register = xreg(0),
          .source_bundle = &call_boundary_bundle,
          .source_move = &call_boundary_bundle.moves.front(),
      });

  const auto result = print_common_instruction_nodes({move});
  if (!result.ok) {
    return fail("expected call-boundary register move to print: " + result.diagnostic);
  }
  const auto move_mnemonic =
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(move);
  const std::string expected = "    " + std::string(move_mnemonic) + " x0, x2\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    mov x0, x2\n",
                         "call-boundary register-move drift guard");
}

int selected_after_call_result_register_move_prints_prepared_mov() {
  const prepare::PreparedMoveBundle call_boundary_bundle{
      .function_name = c4c::FunctionNameId{2},
      .phase = prepare::PreparedMovePhase::AfterCall,
      .block_index = 0,
      .instruction_index = 4,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = prepare::PreparedValueId{71},
                  .to_value_id = prepare::PreparedValueId{71},
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallResultAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"x0"},
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
              },
          },
      .abi_bindings =
          {
              prepare::PreparedAbiBinding{
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallResultAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"x0"},
              },
          },
  };
  const auto move = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = call_boundary_bundle.function_name,
          .phase = call_boundary_bundle.phase,
          .block_index = call_boundary_bundle.block_index,
          .instruction_index = call_boundary_bundle.instruction_index,
          .move = call_boundary_bundle.moves.front(),
          .source_register = xreg(0),
          .destination_register = xreg(3),
          .source_bundle = &call_boundary_bundle,
          .source_move = &call_boundary_bundle.moves.front(),
      });

  const auto result = print_common_instruction_nodes({move});
  if (!result.ok) {
    return fail("expected after-call result register move to print: " + result.diagnostic);
  }
  const auto move_mnemonic =
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(move);
  const std::string expected = "    " + std::string(move_mnemonic) + " x3, x0\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    mov x3, x0\n",
                         "after-call result register-move drift guard");
}

int selected_simple_frame_setup_and_teardown_print_from_prepared_frame_facts() {
  const prepare::PreparedFramePlanFunction prepared_frame{
      .function_name = c4c::FunctionNameId{2},
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
  };
  const auto setup = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::PrologueSetup,
          .function_name = prepared_frame.function_name,
          .frame_size_bytes = prepared_frame.frame_size_bytes,
          .frame_alignment_bytes = prepared_frame.frame_alignment_bytes,
          .source_frame = &prepared_frame,
      });
  const auto teardown = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::EpilogueTeardown,
          .function_name = prepared_frame.function_name,
          .frame_size_bytes = prepared_frame.frame_size_bytes,
          .frame_alignment_bytes = prepared_frame.frame_alignment_bytes,
          .source_frame = &prepared_frame,
      });

  const auto result = print_common_instruction_nodes({setup, teardown});
  if (!result.ok) {
    return fail("expected simple fixed frame nodes to print from prepared frame facts: " +
                result.diagnostic);
  }
  const auto setup_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(setup);
  const auto teardown_mnemonic =
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(teardown);
  const std::string expected =
      "    " + std::string(setup_mnemonic) + " sp, sp, #32\n" +
      "    " + std::string(teardown_mnemonic) + " sp, sp, #32\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    sub sp, sp, #32\n"
                         "    add sp, sp, #32\n",
                         "simple frame common-printer drift guard");
}

int selected_direct_address_materialization_prints_page_low12_sequence() {
  prepare::PreparedAddressMaterialization source;
  const auto address = aarch64_codegen::make_address_materialization_instruction(
      aarch64_codegen::AddressMaterializationRecord{
          .kind = aarch64_codegen::AddressMaterializationKind::DirectPageLow12,
          .prepared_kind = prepare::PreparedAddressMaterializationKind::DirectGlobal,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 4,
          .result_value_id = prepare::PreparedValueId{40},
          .result_value_name = c4c::ValueNameId{41},
          .result_home_kind = prepare::PreparedValueHomeKind::Register,
          .result_register = xreg(9),
          .symbol_name = c4c::LinkNameId{42},
          .symbol_label = "g.direct",
          .byte_offset = 16,
          .source_materialization = &source,
      });
  const auto result = aarch64_codegen::print_machine_instruction_line_payloads(address);
  if (!result.ok || !result.diagnostic.empty()) {
    return fail("expected direct global address materialization to print ADRP/ADD");
  }
  if (result.instruction_lines.size() != 2) {
    return fail("expected direct global address materialization to print two lines");
  }
  if (const int check = expect_equal(
          result.instruction_lines[0], "adrp x9, g.direct+16", "direct address ADRP");
      check != 0) {
    return check;
  }
  return expect_equal(
      result.instruction_lines[1], "add x9, x9, :lo12:g.direct+16", "direct address low12 ADD");
}

int selected_string_address_materialization_prints_page_low12_sequence() {
  prepare::PreparedAddressMaterialization source;
  const auto address = aarch64_codegen::make_address_materialization_instruction(
      aarch64_codegen::AddressMaterializationRecord{
          .kind = aarch64_codegen::AddressMaterializationKind::StringConstant,
          .prepared_kind = prepare::PreparedAddressMaterializationKind::StringConstant,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 5,
          .result_value_id = prepare::PreparedValueId{42},
          .result_value_name = c4c::ValueNameId{43},
          .result_home_kind = prepare::PreparedValueHomeKind::Register,
          .result_register = xreg(10),
          .text_name = c4c::TextId{44},
          .text_label = ".L.str0",
          .source_materialization = &source,
      });
  const auto result = aarch64_codegen::print_machine_instruction_line_payloads(address);
  if (!result.ok || !result.diagnostic.empty()) {
    return fail("expected string address materialization to print ADRP/ADD");
  }
  if (result.instruction_lines.size() != 2) {
    return fail("expected string address materialization to print two lines");
  }
  if (const int check =
          expect_equal(result.instruction_lines[0], "adrp x10, .L.str0", "string address ADRP");
      check != 0) {
    return check;
  }
  return expect_equal(
      result.instruction_lines[1], "add x10, x10, :lo12:.L.str0", "string address low12 ADD");
}

int remaining_address_materialization_printer_paths_use_structured_facts() {
  prepare::PreparedAddressMaterialization source;
  const auto tls = aarch64_codegen::make_address_materialization_instruction(
      aarch64_codegen::AddressMaterializationRecord{
          .kind = aarch64_codegen::AddressMaterializationKind::TlsRelative,
          .prepared_kind = prepare::PreparedAddressMaterializationKind::TlsGlobal,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 6,
          .result_value_id = prepare::PreparedValueId{44},
          .result_value_name = c4c::ValueNameId{45},
          .result_home_kind = prepare::PreparedValueHomeKind::Register,
          .result_register = xreg(11),
          .symbol_name = c4c::LinkNameId{46},
          .symbol_label = "g.tls",
          .address_space = bir::AddressSpace::Tls,
          .is_thread_local = true,
          .has_tls_address_space = true,
          .tls_model = prepare::PreparedTlsMaterializationModel::LocalExecThreadPointerRelative,
          .tls_thread_pointer_register =
              prepare::PreparedTlsThreadPointerRegister::Aarch64TpidrEl0,
          .tls_high_relocation = prepare::PreparedTlsRelocationKind::Aarch64TprelHi12,
          .tls_low_relocation = prepare::PreparedTlsRelocationKind::Aarch64TprelLo12Nc,
          .source_materialization = &source,
      });
  const auto tls_result = aarch64_codegen::print_machine_instruction_line_payloads(tls);
  if (!tls_result.ok || !tls_result.diagnostic.empty()) {
    return fail("expected TLS address materialization to print local-exec sequence");
  }
  if (tls_result.instruction_lines.size() != 3) {
    return fail("expected TLS address materialization to print three lines");
  }
  if (const int check =
          expect_equal(tls_result.instruction_lines[0], "mrs x11, tpidr_el0", "TLS MRS");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(tls_result.instruction_lines[1],
                                     "add x11, x11, :tprel_hi12:g.tls",
                                     "TLS high relocation ADD");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(tls_result.instruction_lines[2],
                                     "add x11, x11, :tprel_lo12_nc:g.tls",
                                     "TLS low relocation ADD");
      check != 0) {
    return check;
  }

  const auto got = aarch64_codegen::make_address_materialization_instruction(
      aarch64_codegen::AddressMaterializationRecord{
          .kind = aarch64_codegen::AddressMaterializationKind::GotPageLow12,
          .prepared_kind = prepare::PreparedAddressMaterializationKind::GotGlobal,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 7,
          .result_value_id = prepare::PreparedValueId{46},
          .result_value_name = c4c::ValueNameId{47},
          .result_home_kind = prepare::PreparedValueHomeKind::Register,
          .result_register = xreg(12),
          .symbol_name = c4c::LinkNameId{48},
          .symbol_label = "g.got",
          .address_materialization_policy =
              bir::GlobalAddressMaterializationPolicy::GotRequired,
          .source_materialization = &source,
      });
  const auto got_result = aarch64_codegen::print_machine_instruction_line_payloads(got);
  if (!got_result.ok || !got_result.diagnostic.empty()) {
    return fail("expected GOT address materialization to print GOT load sequence");
  }
  if (got_result.instruction_lines.size() != 2) {
    return fail("expected GOT address materialization to print two lines");
  }
  if (const int check =
          expect_equal(got_result.instruction_lines[0], "adrp x12, :got:g.got", "GOT ADRP");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(got_result.instruction_lines[1],
                                     "ldr x12, [x12, :got_lo12:g.got]",
                                     "GOT low12 LDR");
      check != 0) {
    return check;
  }

  const auto label = aarch64_codegen::make_address_materialization_instruction(
      aarch64_codegen::AddressMaterializationRecord{
          .kind = aarch64_codegen::AddressMaterializationKind::LabelPageLow12,
          .prepared_kind = prepare::PreparedAddressMaterializationKind::Label,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 8,
          .result_value_id = prepare::PreparedValueId{48},
          .result_value_name = c4c::ValueNameId{49},
          .result_home_kind = prepare::PreparedValueHomeKind::Register,
          .result_register = xreg(13),
          .target_label = c4c::BlockLabelId{50},
          .target_label_name = "target",
          .source_materialization = &source,
      });
  const auto label_result = aarch64_codegen::print_machine_instruction_line_payloads(label);
  if (!label_result.ok || !label_result.diagnostic.empty()) {
    return fail("expected label address materialization to print ADRP/ADD");
  }
  if (label_result.instruction_lines.size() != 2) {
    return fail("expected label address materialization to print two lines");
  }
  if (const int check =
          expect_equal(label_result.instruction_lines[0], "adrp x13, target", "label ADRP");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(label_result.instruction_lines[1],
                                     "add x13, x13, :lo12:target",
                                     "label low12 ADD");
      check != 0) {
    return check;
  }

  const auto missing_got_policy = aarch64_codegen::make_address_materialization_instruction(
      aarch64_codegen::AddressMaterializationRecord{
          .kind = aarch64_codegen::AddressMaterializationKind::GotPageLow12,
          .prepared_kind = prepare::PreparedAddressMaterializationKind::GotGlobal,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 9,
          .result_value_id = prepare::PreparedValueId{50},
          .result_value_name = c4c::ValueNameId{51},
          .result_home_kind = prepare::PreparedValueHomeKind::Register,
          .result_register = xreg(14),
          .symbol_name = c4c::LinkNameId{52},
          .symbol_label = "g.got",
          .source_materialization = &source,
      });
  const auto missing_got_policy_result =
      aarch64_codegen::print_machine_instruction_line_payloads(missing_got_policy);
  if (missing_got_policy_result.ok ||
      missing_got_policy_result.diagnostic.find("GOT-required policy") == std::string::npos) {
    return fail("expected GOT address materialization without policy to fail closed");
  }

  const auto missing_label_text = aarch64_codegen::make_address_materialization_instruction(
      aarch64_codegen::AddressMaterializationRecord{
          .kind = aarch64_codegen::AddressMaterializationKind::LabelPageLow12,
          .prepared_kind = prepare::PreparedAddressMaterializationKind::Label,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 10,
          .result_value_id = prepare::PreparedValueId{52},
          .result_value_name = c4c::ValueNameId{53},
          .result_home_kind = prepare::PreparedValueHomeKind::Register,
          .result_register = xreg(15),
          .target_label = c4c::BlockLabelId{54},
          .source_materialization = &source,
      });
  const auto missing_label_text_result =
      aarch64_codegen::print_machine_instruction_line_payloads(missing_label_text);
  if (missing_label_text_result.ok ||
      missing_label_text_result.diagnostic.find("target label text") == std::string::npos) {
    return fail("expected label address materialization without label text to fail closed");
  }
  return 0;
}

int unsupported_surfaces_statuses_and_missing_operands_fail_closed() {
  const auto assembler = aarch64_codegen::make_assembler_instruction(
      aarch64_codegen::AssemblerInstructionRecord{});
  const auto assembler_result =
      aarch64_codegen::print_machine_instruction_line_payloads(assembler);
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
      aarch64_codegen::print_machine_instruction_line_payloads(unsupported);
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
  const auto scalar_result =
      aarch64_codegen::print_machine_instruction_line_payloads(scalar);
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
      aarch64_codegen::print_machine_instruction_line_payloads(
          scalar_with_unprintable_sub_operands);
  if (scalar_with_unprintable_sub_operands_result.ok ||
      scalar_with_unprintable_sub_operands_result.diagnostic.find(
          "scalar sub with an immediate lhs and register rhs is not printable") ==
          std::string::npos) {
    return fail("expected selected scalar with unprintable sub operands to fail closed");
  }

  auto load_address = frame_slot(48);
  load_address.result_value_id = prepare::PreparedValueId{34};
  load_address.result_value_name = c4c::ValueNameId{35};
  const auto load_missing_destination = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Load,
          .address = load_address,
          .result_value_id = prepare::PreparedValueId{34},
          .result_value_name = c4c::ValueNameId{35},
          .value_type = bir::TypeKind::I32,
      });
  const auto load_missing_destination_result =
      aarch64_codegen::print_machine_instruction_line_payloads(load_missing_destination);
  if (load_missing_destination_result.ok ||
      load_missing_destination_result.diagnostic.find(
          "load node is missing a structured destination register operand") ==
          std::string::npos) {
    return fail("expected selected load without destination register to fail closed");
  }

  const auto pointer_store_missing_base = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address =
              aarch64_codegen::MemoryOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
                  .function_name = c4c::FunctionNameId{2},
                  .block_label = c4c::BlockLabelId{3},
                  .stored_value_id = prepare::PreparedValueId{36},
                  .stored_value_name = c4c::ValueNameId{37},
                  .base_kind = aarch64_codegen::MemoryBaseKind::PointerValue,
                  .byte_offset = 8,
                  .size_bytes = 4,
                  .align_bytes = 4,
                  .can_use_base_plus_offset = true,
              },
          .value = aarch64_codegen::make_register_operand(wreg(4)),
          .value_type = bir::TypeKind::I32,
      });
  const auto pointer_store_missing_base_result =
      aarch64_codegen::print_machine_instruction_line_payloads(pointer_store_missing_base);
  if (pointer_store_missing_base_result.ok ||
      pointer_store_missing_base_result.diagnostic.find(
          "memory address is not printable") == std::string::npos) {
    return fail("expected pointer-value store without base register to fail closed");
  }

  auto unprepared_store_address = frame_slot(56);
  unprepared_store_address.support =
      aarch64_codegen::MemoryOperandSupportKind::DeferredUnsupported;
  unprepared_store_address.stored_value_id = prepare::PreparedValueId{38};
  unprepared_store_address.stored_value_name = c4c::ValueNameId{39};
  const auto unprepared_store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address = unprepared_store_address,
          .value = aarch64_codegen::make_register_operand(wreg(5)),
          .value_type = bir::TypeKind::I32,
      });
  const auto unprepared_store_result =
      aarch64_codegen::print_machine_instruction_line_payloads(unprepared_store);
  if (unprepared_store_result.ok ||
      unprepared_store_result.diagnostic.find("memory operand is outside the selected subset") ==
          std::string::npos) {
    return fail("expected unprepared memory base to fail closed before printing");
  }

  const auto call_missing_provenance = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{10},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "actual_function",
          .calling_convention = bir::CallingConv::C,
      });
  const auto call_missing_provenance_result =
      aarch64_codegen::print_machine_instruction_line_payloads(call_missing_provenance);
  if (call_missing_provenance_result.ok ||
      call_missing_provenance_result.diagnostic.find("missing prepared callee provenance") ==
          std::string::npos) {
    return fail("expected selected direct call without prepared provenance to fail closed");
  }

  const auto indirect_call_missing_provenance = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .indirect_callee =
              aarch64_codegen::make_register_operand(aarch64_codegen::RegisterOperand{
                  .reg = aarch64_abi::x_register(9),
                  .role = aarch64_codegen::RegisterOperandRole::CallAbi,
              }),
          .calling_convention = bir::CallingConv::C,
          .is_indirect = true,
      });
  const auto indirect_call_missing_provenance_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          indirect_call_missing_provenance);
  if (indirect_call_missing_provenance_result.ok ||
      indirect_call_missing_provenance_result.diagnostic.find(
          "missing prepared callee provenance") == std::string::npos) {
    return fail("expected selected indirect call without prepared provenance to fail closed");
  }

  const prepare::PreparedCallPlan prepared_va_start_call{
      .block_index = 0,
      .instruction_index = 2,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"llvm.va_start.p0"},
  };
  const prepare::PreparedVariadicEntryHelperOperandHomes va_start_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaStart,
      .block_index = 0,
      .instruction_index = 2,
      .destination_va_list =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{14},
              .function_name = c4c::FunctionNameId{2},
              .value_name = c4c::ValueNameId{4},
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x2"},
          },
  };
  const prepare::PreparedVariadicEntryPlanFunction variadic_entry{
      .function_name = c4c::FunctionNameId{2},
      .named_parameter_count = 1,
      .named_register_counts =
          prepare::PreparedVariadicEntryNamedRegisterCounts{
              .gp = std::size_t{1},
              .fp = std::size_t{0},
          },
      .register_save_area =
          prepare::PreparedVariadicEntryRegisterSaveArea{
              .required = true,
              .size_bytes = std::size_t{192},
              .align_bytes = std::size_t{16},
              .slot_id = prepare::PreparedFrameSlotId{5},
              .stack_offset_bytes = std::size_t{16},
              .gp_offset_bytes = std::size_t{0},
              .fp_offset_bytes = std::size_t{64},
              .gp_slot_size_bytes = std::size_t{8},
              .fp_slot_size_bytes = std::size_t{16},
              .saved_gp_register_count = std::size_t{7},
              .saved_fp_register_count = std::size_t{8},
              .initial_gp_offset_bytes = std::ptrdiff_t{-56},
              .initial_fp_offset_bytes = std::ptrdiff_t{-128},
          },
      .overflow_area =
          prepare::PreparedVariadicEntryOverflowArea{
              .required = true,
              .base_slot_id = prepare::PreparedFrameSlotId{6},
              .base_stack_offset_bytes = std::size_t{208},
              .align_bytes = std::size_t{8},
          },
      .va_list_layout =
          prepare::PreparedVariadicVaListLayout{
              .required = true,
              .size_bytes = std::size_t{32},
              .align_bytes = std::size_t{8},
              .fields =
                  {
                      prepare::PreparedVariadicVaListField{
                          .kind = prepare::PreparedVariadicVaListFieldKind::GpOffset,
                          .offset_bytes = 0,
                          .size_bytes = 4,
                      },
                      prepare::PreparedVariadicVaListField{
                          .kind =
                              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
                          .offset_bytes = 8,
                          .size_bytes = 8,
                      },
                  },
          },
      .helper_resources =
          prepare::PreparedVariadicEntryHelperResources{
              .required_helpers = {prepare::PreparedVariadicEntryHelperKind::VaStart},
              .scratch_register_count = std::size_t{1},
              .scratch_stack_bytes = std::size_t{0},
          },
      .helper_operand_homes = {va_start_homes},
  };
  const auto va_start_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{11},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_start.p0",
          .wrapper_kind = prepared_va_start_call.wrapper_kind,
          .source_call = &prepared_va_start_call,
          .source_variadic_entry = &variadic_entry,
          .source_variadic_helper_operand_homes = &variadic_entry.helper_operand_homes.front(),
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaStart,
          .calling_convention = bir::CallingConv::C,
      });
  const auto va_start_result =
      aarch64_codegen::print_machine_instruction_line_payloads(va_start_call);
  if (!va_start_result.ok || va_start_result.instruction_lines.size() != 5 ||
      va_start_result.instruction_lines[0].find("va.start dest=value#14:register:x2") ==
          std::string::npos ||
      va_start_result.instruction_lines[1].find("slot#5 stack+16") == std::string::npos ||
      va_start_result.instruction_lines[2].find("overflow_slot#6 overflow_stack+208") ==
          std::string::npos ||
      va_start_result.instruction_lines[3] !=
          "va.start.field kind=gp_offset offset=0 size=4" ||
      va_start_result.instruction_lines[4] !=
          "va.start.field kind=overflow_arg_area offset=8 size=8") {
    return fail("expected variadic entry helper call to print prepared va_start records");
  }

  const prepare::PreparedCallPlan prepared_va_arg_call{
      .block_index = 0,
      .instruction_index = 3,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"llvm.va_arg.i32"},
  };
  const prepare::PreparedVariadicEntryHelperOperandHomes va_arg_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaArg,
      .block_index = 0,
      .instruction_index = 3,
      .source_va_list =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{15},
              .function_name = c4c::FunctionNameId{2},
              .value_name = c4c::ValueNameId{5},
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x3"},
          },
      .scalar_result =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{16},
              .function_name = c4c::FunctionNameId{2},
              .value_name = c4c::ValueNameId{6},
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"w0"},
          },
      .scalar_access_plan =
          prepare::PreparedVariadicScalarVaArgAccessPlan{
              .source_class =
                  prepare::PreparedVariadicScalarVaArgSourceClass::OverflowArgArea,
              .value_type = bir::TypeKind::I32,
              .value_size_bytes = 4,
              .value_align_bytes = 4,
              .result_home =
                  prepare::PreparedValueHome{
                      .value_id = prepare::PreparedValueId{16},
                      .function_name = c4c::FunctionNameId{2},
                      .value_name = c4c::ValueNameId{6},
                      .kind = prepare::PreparedValueHomeKind::Register,
                      .register_name = std::string{"w0"},
                  },
              .source_field =
                  prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
              .source_field_offset_bytes = std::size_t{8},
              .source_slot_size_bytes = std::size_t{8},
              .progression_field =
                  prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
              .progression_field_offset_bytes = std::size_t{8},
              .progression_stride_bytes = std::size_t{8},
              .overflow_source_field =
                  prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
              .overflow_source_field_offset_bytes = std::size_t{8},
              .overflow_stride_bytes = std::size_t{8},
          },
  };
  auto scalar_va_arg_entry = variadic_entry;
  scalar_va_arg_entry.helper_resources.required_helpers =
      {prepare::PreparedVariadicEntryHelperKind::VaArg};
  scalar_va_arg_entry.helper_resources.scratch_register_count = std::size_t{2};
  scalar_va_arg_entry.helper_operand_homes = {va_arg_homes};
  const auto va_arg_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{12},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_arg.i32",
          .wrapper_kind = prepared_va_arg_call.wrapper_kind,
          .source_call = &prepared_va_arg_call,
          .source_variadic_entry = &scalar_va_arg_entry,
          .source_variadic_helper_operand_homes =
              &scalar_va_arg_entry.helper_operand_homes.front(),
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaArg,
          .calling_convention = bir::CallingConv::C,
      });
  const auto va_arg_result =
      aarch64_codegen::print_machine_instruction_line_payloads(va_arg_call);
  if (!va_arg_result.ok || va_arg_result.instruction_lines.size() != 3 ||
      va_arg_result.instruction_lines[0].find(
          "va.arg.scalar source=overflow_arg_area va_list=value#15:register:x3 result=value#16:register:w0") ==
          std::string::npos ||
      va_arg_result.instruction_lines[1].find(
          "field=overflow_arg_area field_offset=8 slot_size=8") ==
          std::string::npos ||
      va_arg_result.instruction_lines[2].find(
          "progress field=overflow_arg_area field_offset=8 stride=8") ==
          std::string::npos ||
      va_arg_result.instruction_lines[2].find(
          "overflow_slot#6 overflow_stack+208") == std::string::npos) {
    return fail("expected scalar va_arg helper call to print prepared access-plan records");
  }

  auto scalar_fp_va_arg_entry = scalar_va_arg_entry;
  auto fp_homes = scalar_fp_va_arg_entry.helper_operand_homes.front();
  fp_homes.scalar_result =
      prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{17},
          .function_name = c4c::FunctionNameId{2},
          .value_name = c4c::ValueNameId{7},
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"d0"},
      };
  fp_homes.scalar_access_plan =
      prepare::PreparedVariadicScalarVaArgAccessPlan{
          .source_class =
              prepare::PreparedVariadicScalarVaArgSourceClass::FpRegisterSaveArea,
          .value_type = bir::TypeKind::F64,
          .value_size_bytes = 8,
          .value_align_bytes = 8,
          .result_home = fp_homes.scalar_result,
          .source_field =
              prepare::PreparedVariadicVaListFieldKind::FpRegisterSaveArea,
          .source_field_offset_bytes = std::size_t{24},
          .source_slot_size_bytes = std::size_t{16},
          .progression_field =
              prepare::PreparedVariadicVaListFieldKind::FpOffset,
          .progression_field_offset_bytes = std::size_t{4},
          .progression_stride_bytes = std::size_t{16},
          .overflow_source_field =
              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
          .overflow_source_field_offset_bytes = std::size_t{8},
          .overflow_stride_bytes = std::size_t{8},
      };
  scalar_fp_va_arg_entry.helper_operand_homes = {fp_homes};
  const auto fp_va_arg_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{13},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_arg.f64",
          .wrapper_kind = prepared_va_arg_call.wrapper_kind,
          .source_call = &prepared_va_arg_call,
          .source_variadic_entry = &scalar_fp_va_arg_entry,
          .source_variadic_helper_operand_homes =
              &scalar_fp_va_arg_entry.helper_operand_homes.front(),
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaArg,
          .calling_convention = bir::CallingConv::C,
      });
  const auto fp_va_arg_result =
      aarch64_codegen::print_machine_instruction_line_payloads(fp_va_arg_call);
  if (!fp_va_arg_result.ok || fp_va_arg_result.instruction_lines.size() != 3 ||
      fp_va_arg_result.instruction_lines[0].find(
          "va.arg.scalar source=fp_register_save_area va_list=value#15:register:x3 result=value#17:register:d0") ==
          std::string::npos ||
      fp_va_arg_result.instruction_lines[1].find(
          "field=fp_register_save_area field_offset=24 slot_size=16") ==
          std::string::npos ||
      fp_va_arg_result.instruction_lines[1].find(
          "fp_base=64 gp_slot=8 fp_slot=16") == std::string::npos ||
      fp_va_arg_result.instruction_lines[2].find(
          "progress field=fp_offset field_offset=4 stride=16") ==
          std::string::npos) {
    return fail("expected scalar fp va_arg helper call to print prepared access-plan records");
  }

  const prepare::PreparedCallPlan prepared_aggregate_va_arg_call{
      .block_index = 0,
      .instruction_index = 4,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"llvm.va_arg.aggregate"},
  };
  const prepare::PreparedVariadicEntryHelperOperandHomes aggregate_va_arg_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaArgAggregate,
      .block_index = 0,
      .instruction_index = 4,
      .source_va_list =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{18},
              .function_name = c4c::FunctionNameId{2},
              .value_name = c4c::ValueNameId{8},
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x4"},
          },
      .aggregate_destination_payload =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{19},
              .function_name = c4c::FunctionNameId{2},
              .value_name = c4c::ValueNameId{9},
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = prepare::PreparedFrameSlotId{9},
              .offset_bytes = std::size_t{32},
          },
      .aggregate_access_plan =
          prepare::PreparedVariadicAggregateVaArgAccessPlan{
              .source_class =
                  prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea,
              .payload_size_bytes = 24,
              .payload_align_bytes = 8,
              .destination_payload_home =
                  prepare::PreparedValueHome{
                      .value_id = prepare::PreparedValueId{19},
                      .function_name = c4c::FunctionNameId{2},
                      .value_name = c4c::ValueNameId{9},
                      .kind = prepare::PreparedValueHomeKind::StackSlot,
                      .slot_id = prepare::PreparedFrameSlotId{9},
                      .offset_bytes = std::size_t{32},
                  },
              .source_field =
                  prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
              .source_field_offset_bytes = std::size_t{8},
              .source_payload_offset_bytes = std::size_t{0},
              .source_slot_size_bytes = std::size_t{24},
              .copy_size_bytes = std::size_t{24},
              .copy_align_bytes = std::size_t{8},
              .progression_field =
                  prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
              .progression_field_offset_bytes = std::size_t{8},
              .progression_stride_bytes = std::size_t{24},
          },
  };
  auto aggregate_va_arg_entry = variadic_entry;
  aggregate_va_arg_entry.helper_resources.required_helpers =
      {prepare::PreparedVariadicEntryHelperKind::VaArgAggregate};
  aggregate_va_arg_entry.helper_resources.scratch_register_count = std::size_t{2};
  aggregate_va_arg_entry.helper_operand_homes = {aggregate_va_arg_homes};
  const auto aggregate_va_arg_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{14},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_arg.aggregate",
          .wrapper_kind = prepared_aggregate_va_arg_call.wrapper_kind,
          .source_call = &prepared_aggregate_va_arg_call,
          .source_variadic_entry = &aggregate_va_arg_entry,
          .source_variadic_helper_operand_homes =
              &aggregate_va_arg_entry.helper_operand_homes.front(),
          .variadic_entry_helper =
              prepare::PreparedVariadicEntryHelperKind::VaArgAggregate,
          .calling_convention = bir::CallingConv::C,
      });
  const auto aggregate_va_arg_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          aggregate_va_arg_call);
  if (!aggregate_va_arg_result.ok ||
      aggregate_va_arg_result.instruction_lines.size() != 3 ||
      aggregate_va_arg_result.instruction_lines[0].find(
          "va.arg.aggregate source=overflow_arg_area va_list=value#18:register:x4 destination_payload=value#19:stack_slot:slot#9:offset+32") ==
          std::string::npos ||
      aggregate_va_arg_result.instruction_lines[0].find(
          "payload_size=24 payload_align=8 copy_size=24 copy_align=8") ==
          std::string::npos ||
      aggregate_va_arg_result.instruction_lines[1].find(
          "field=overflow_arg_area field_offset=8 payload_offset=0 slot_size=24") ==
          std::string::npos ||
      aggregate_va_arg_result.instruction_lines[2].find(
          "progress field=overflow_arg_area field_offset=8 stride=24") ==
          std::string::npos ||
      aggregate_va_arg_result.instruction_lines[2].find(
          "overflow_slot#6 overflow_stack+208") == std::string::npos) {
    return fail(
        "expected aggregate va_arg helper call to print prepared access-plan records");
  }

  const prepare::PreparedCallPlan prepared_va_copy_call{
      .block_index = 0,
      .instruction_index = 5,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"llvm.va_copy.p0.p0"},
  };
  const prepare::PreparedVariadicEntryHelperOperandHomes va_copy_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaCopy,
      .block_index = 0,
      .instruction_index = 5,
      .destination_va_list =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{20},
              .function_name = c4c::FunctionNameId{2},
              .value_name = c4c::ValueNameId{10},
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = prepare::PreparedFrameSlotId{10},
              .offset_bytes = std::size_t{64},
          },
      .source_va_list =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{21},
              .function_name = c4c::FunctionNameId{2},
              .value_name = c4c::ValueNameId{11},
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x5"},
          },
  };
  auto va_copy_entry = variadic_entry;
  va_copy_entry.helper_resources.required_helpers =
      {prepare::PreparedVariadicEntryHelperKind::VaCopy};
  va_copy_entry.helper_resources.scratch_register_count = std::size_t{1};
  va_copy_entry.helper_operand_homes = {va_copy_homes};
  const auto va_copy_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{15},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_copy.p0.p0",
          .wrapper_kind = prepared_va_copy_call.wrapper_kind,
          .source_call = &prepared_va_copy_call,
          .source_variadic_entry = &va_copy_entry,
          .source_variadic_helper_operand_homes =
              &va_copy_entry.helper_operand_homes.front(),
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaCopy,
          .calling_convention = bir::CallingConv::C,
      });
  const auto va_copy_result =
      aarch64_codegen::print_machine_instruction_line_payloads(va_copy_call);
  if (!va_copy_result.ok ||
      va_copy_result.instruction_lines.size() != 3 ||
      va_copy_result.instruction_lines[0].find(
          "va.copy dest=value#20:stack_slot:slot#10:offset+64 source=value#21:register:x5") ==
          std::string::npos ||
      va_copy_result.instruction_lines[0].find(
          "va_list_size=32 va_list_align=8 scratch_registers=1") ==
          std::string::npos ||
      va_copy_result.instruction_lines[1] !=
          "va.copy.field kind=gp_offset source_offset=0 destination_offset=0 size=4" ||
      va_copy_result.instruction_lines[2] !=
          "va.copy.field kind=overflow_arg_area source_offset=8 destination_offset=8 size=8") {
    return fail("expected va_copy helper call to print prepared layout field copies");
  }

  const auto frame_missing_provenance = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::PrologueSetup,
          .function_name = c4c::FunctionNameId{2},
          .frame_size_bytes = 32,
          .frame_alignment_bytes = 16,
      });
  const auto frame_missing_provenance_result =
      aarch64_codegen::print_machine_instruction_line_payloads(frame_missing_provenance);
  if (frame_missing_provenance_result.ok ||
      frame_missing_provenance_result.diagnostic.find("prepared frame facts") ==
          std::string::npos) {
    return fail("expected selected frame without prepared provenance to fail closed");
  }

  const prepare::PreparedFramePlanFunction saved_frame{
      .function_name = c4c::FunctionNameId{2},
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
      .saved_callee_registers =
          {
              prepare::PreparedSavedRegister{
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .register_name = "x19",
                  .contiguous_width = 1,
                  .occupied_register_names = {"x19"},
                  .save_index = 0,
              },
          },
  };
  const auto frame_with_saved_register = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::PrologueSetup,
          .function_name = saved_frame.function_name,
          .frame_size_bytes = saved_frame.frame_size_bytes,
          .frame_alignment_bytes = saved_frame.frame_alignment_bytes,
          .saved_callee_registers = saved_frame.saved_callee_registers,
          .source_frame = &saved_frame,
      });
  const auto saved_result =
      aarch64_codegen::print_machine_instruction_line_payloads(frame_with_saved_register);
  if (saved_result.ok ||
      saved_result.diagnostic.find("callee-save frame node is outside the printable subset") ==
          std::string::npos) {
    return fail("expected frame with callee-save facts to fail closed");
  }

  const prepare::PreparedFramePlanFunction dynamic_frame{
      .function_name = c4c::FunctionNameId{2},
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
      .has_dynamic_stack = true,
  };
  const auto dynamic_stack_frame = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::PrologueSetup,
          .function_name = dynamic_frame.function_name,
          .frame_size_bytes = dynamic_frame.frame_size_bytes,
          .frame_alignment_bytes = dynamic_frame.frame_alignment_bytes,
          .has_dynamic_stack = dynamic_frame.has_dynamic_stack,
          .source_frame = &dynamic_frame,
      });
  const auto dynamic_result =
      aarch64_codegen::print_machine_instruction_line_payloads(dynamic_stack_frame);
  if (dynamic_result.ok ||
      dynamic_result.diagnostic.find("dynamic-stack frame node is outside the printable subset") ==
          std::string::npos) {
    return fail("expected dynamic-stack frame to fail closed");
  }

  const prepare::PreparedMoveBundle call_boundary_bundle{
      .function_name = c4c::FunctionNameId{2},
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = 0,
      .instruction_index = 4,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = prepare::PreparedValueId{70},
                  .to_value_id = prepare::PreparedValueId{71},
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{0},
                  .destination_register_name = std::string{"x0"},
              },
          },
      .abi_bindings =
          {
              prepare::PreparedAbiBinding{
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{0},
                  .destination_register_name = std::string{"x0"},
              },
          },
  };
  const auto call_boundary_move = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = call_boundary_bundle.function_name,
          .phase = call_boundary_bundle.phase,
          .block_index = call_boundary_bundle.block_index,
          .instruction_index = call_boundary_bundle.instruction_index,
          .move = call_boundary_bundle.moves.front(),
          .source_bundle = &call_boundary_bundle,
          .source_move = &call_boundary_bundle.moves.front(),
      });
  const auto move_result =
      aarch64_codegen::print_machine_instruction_line_payloads(call_boundary_move);
  if (move_result.ok ||
      move_result.diagnostic.find(
          "call-boundary move node requires prepared register source and destination") ==
          std::string::npos) {
    return fail("expected call-boundary move record to fail closed in printer");
  }

  const auto call_boundary_binding =
      aarch64_codegen::make_call_boundary_abi_binding_instruction(
          aarch64_codegen::CallBoundaryAbiBindingInstructionRecord{
              .function_name = call_boundary_bundle.function_name,
              .phase = call_boundary_bundle.phase,
              .block_index = call_boundary_bundle.block_index,
              .instruction_index = call_boundary_bundle.instruction_index,
              .binding = call_boundary_bundle.abi_bindings.front(),
              .source_bundle = &call_boundary_bundle,
              .source_binding = &call_boundary_bundle.abi_bindings.front(),
          });
  const auto binding_result =
      aarch64_codegen::print_machine_instruction_line_payloads(call_boundary_binding);
  if (binding_result.ok ||
      binding_result.diagnostic.find(
          "call-boundary ABI binding node requires later AArch64 move lowering") ==
          std::string::npos) {
    return fail("expected call-boundary ABI binding record to fail closed in printer");
  }

  const auto missing_move = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = c4c::FunctionNameId{2},
          .move = call_boundary_bundle.moves.front(),
      });
  const auto missing_move_result =
      aarch64_codegen::print_machine_instruction_line_payloads(missing_move);
  if (missing_move_result.ok ||
      missing_move_result.diagnostic.find("missing prepared move provenance") ==
          std::string::npos) {
    return fail("expected call-boundary move without provenance to fail closed");
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
  if (const int result = selected_structured_memory_subset_prints_loads_and_stores();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_scalar_add_sub_and_register_return_print_from_structured_operands();
      result != 0) {
    return result;
  }
  if (const int result = selected_simple_integer_casts_print_from_structured_operands();
      result != 0) {
    return result;
  }
  if (const int result = selected_immediate_return_node_prints_callable_epilogue();
      result != 0) {
    return result;
  }
  if (const int result = common_mir_printer_can_delegate_to_aarch64_target_spelling_adapter();
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
  if (const int result = selected_simple_integer_casts_reject_missing_or_unsupported_facts();
      result != 0) {
    return result;
  }
  if (const int result = selected_direct_call_prints_from_prepared_call_provenance();
      result != 0) {
    return result;
  }
  if (const int result = selected_memory_return_call_prints_call_and_preserves_storage_effect();
      result != 0) {
    return result;
  }
  if (const int result = selected_indirect_call_prints_from_prepared_register_callee();
      result != 0) {
    return result;
  }
  if (const int result = selected_call_boundary_register_move_prints_prepared_mov();
      result != 0) {
    return result;
  }
  if (const int result = selected_after_call_result_register_move_prints_prepared_mov();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_simple_frame_setup_and_teardown_print_from_prepared_frame_facts();
      result != 0) {
    return result;
  }
  if (const int result = selected_direct_address_materialization_prints_page_low12_sequence();
      result != 0) {
    return result;
  }
  if (const int result = selected_string_address_materialization_prints_page_low12_sequence();
      result != 0) {
    return result;
  }
  if (const int result = remaining_address_materialization_printer_paths_use_structured_facts();
      result != 0) {
    return result;
  }
  if (const int result = unsupported_surfaces_statuses_and_missing_operands_fail_closed();
      result != 0) {
    return result;
  }
  return 0;
}
