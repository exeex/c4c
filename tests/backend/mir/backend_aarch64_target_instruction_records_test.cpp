#include "src/backend/mir/aarch64/codegen/instruction.hpp"

#include <iostream>
#include <string>
#include <string_view>
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

int expect_equal(std::string_view actual, std::string_view expected, const char* context) {
  if (actual != expected) {
    std::cerr << context << " expected '" << expected << "', got '" << actual << "'\n";
    return 1;
  }
  return 0;
}

aarch64_codegen::RegisterOperand value_register(prepare::PreparedValueId value_id,
                                                c4c::ValueNameId value_name,
                                                unsigned index) {
  return aarch64_codegen::RegisterOperand{
      .reg = aarch64_abi::x_register(index),
      .role = aarch64_codegen::RegisterOperandRole::PreparedAssignment,
      .value_id = value_id,
      .value_name = value_name,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = aarch64_abi::RegisterView::X,
      .contiguous_width = 1,
  };
}

aarch64_codegen::OperandRecord make_value_register(prepare::PreparedValueId value_id,
                                                   c4c::ValueNameId value_name,
                                                   unsigned index) {
  return aarch64_codegen::make_register_operand(value_register(value_id, value_name, index));
}

int branch_scalar_and_memory_instruction_records_preserve_typed_operands() {
  const auto condition = make_value_register(prepare::PreparedValueId{10},
                                             c4c::ValueNameId{3},
                                             1);
  const auto branch = aarch64_codegen::make_branch_instruction(
      aarch64_codegen::BranchInstructionRecord{
          .target =
              aarch64_codegen::BranchTargetOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .block_label = c4c::BlockLabelId{7},
                  .function_name = c4c::FunctionNameId{2},
                  .condition_value_id = prepare::PreparedValueId{10},
              },
          .target_pair =
              aarch64_codegen::BranchTargetPairRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .true_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{7},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{10},
                      },
                  .false_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{8},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{10},
                      },
              },
          .condition = condition,
          .condition_record =
              aarch64_codegen::BranchConditionRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .form = aarch64_codegen::BranchConditionForm::MaterializedBool,
                  .condition_value_id = prepare::PreparedValueId{10},
                  .condition_value_name = c4c::ValueNameId{3},
                  .condition_type = bir::TypeKind::I1,
              },
          .conditional = true,
      });

  const auto scalar = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{11},
          .result_value_name = c4c::ValueNameId{4},
          .result_type = bir::TypeKind::I64,
          .result_register =
              value_register(prepare::PreparedValueId{11}, c4c::ValueNameId{4}, 0),
          .lhs = make_value_register(prepare::PreparedValueId{12}, c4c::ValueNameId{5}, 2),
          .rhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I64,
              .signed_value = 8,
              .source_value_id = prepare::PreparedValueId{13},
              .source_value_name = c4c::ValueNameId{6},
          }),
          .supported_integer_operation = true,
      }));

  const auto memory = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Load,
          .address =
              aarch64_codegen::MemoryOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
                  .function_name = c4c::FunctionNameId{2},
                  .block_label = c4c::BlockLabelId{7},
                  .instruction_index = 9,
                  .result_value_id = prepare::PreparedValueId{15},
                  .result_value_name = c4c::ValueNameId{7},
                  .base_kind = aarch64_codegen::MemoryBaseKind::FrameSlot,
                  .frame_slot_id = prepare::PreparedFrameSlotId{9},
                  .pointer_value_id = prepare::PreparedValueId{14},
                  .byte_offset = 16,
                  .size_bytes = 8,
                  .align_bytes = 8,
                  .address_space = bir::AddressSpace::Default,
                  .can_use_base_plus_offset = true,
              },
          .result_value_id = prepare::PreparedValueId{15},
          .result_value_name = c4c::ValueNameId{7},
          .value_type = bir::TypeKind::I64,
      });

  const auto* branch_payload =
      std::get_if<aarch64_codegen::BranchInstructionRecord>(&branch.payload);
  const auto* scalar_payload =
      std::get_if<aarch64_codegen::ScalarInstructionRecord>(&scalar.payload);
  const auto* memory_payload =
      std::get_if<aarch64_codegen::MemoryInstructionRecord>(&memory.payload);

  if (branch.family != aarch64_codegen::InstructionFamily::Branch ||
      aarch64_codegen::instruction_family_name(branch.family) != "branch" ||
      branch.surface != aarch64_codegen::RecordSurfaceKind::MachineInstructionNode ||
      branch.opcode != aarch64_codegen::MachineOpcode::ConditionalBranch ||
      aarch64_codegen::machine_opcode_name(branch.opcode) != "conditional_branch" ||
      branch.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      branch.operands.size() != 3 || branch.uses.size() != 3 || !branch.defs.empty() ||
      branch.side_effects.size() != 1 ||
      branch.side_effects.front() != aarch64_codegen::MachineSideEffectKind::ControlFlowTransfer ||
      !aarch64_codegen::is_structured_downstream_surface(branch.surface) ||
      branch_payload == nullptr || !branch_payload->conditional ||
      branch_payload->target.block_label != c4c::BlockLabelId{7} ||
      branch_payload->target.condition_value_id != prepare::PreparedValueId{10} ||
      !branch_payload->target_pair.has_value() ||
      branch_payload->target_pair->false_target.block_label != c4c::BlockLabelId{8}) {
    return fail("expected branch instruction to be a typed machine-node handoff");
  }

  if (scalar_payload == nullptr || scalar_payload->result_value_id != prepare::PreparedValueId{11} ||
      scalar_payload->inputs.size() != 2 ||
      scalar_payload->source_binary_opcode != bir::BinaryOpcode::Add ||
      aarch64_codegen::instruction_family_name(scalar.family) != "scalar" ||
      scalar.opcode != aarch64_codegen::MachineOpcode::Add ||
      scalar.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      scalar.operands.size() != 2 || scalar.uses.size() != 2 ||
      scalar.defs.size() != 1 ||
      scalar.defs.front().kind != aarch64_codegen::MachineEffectResourceKind::Register ||
      scalar.defs.front().value_id != prepare::PreparedValueId{11} ||
      scalar.defs.front().reg != aarch64_abi::x_register(0) ||
      !scalar_payload->result_register.has_value() ||
      scalar_payload->result_register->value_name != c4c::ValueNameId{4}) {
    return fail("expected scalar instruction record to retain BIR opcode and destination register");
  }

  if (memory_payload == nullptr ||
      aarch64_codegen::memory_instruction_kind_name(memory_payload->memory_kind) != "load" ||
      memory_payload->address.frame_slot_id != prepare::PreparedFrameSlotId{9} ||
      memory_payload->address.pointer_value_id != prepare::PreparedValueId{14} ||
      memory_payload->result_value_id != prepare::PreparedValueId{15} ||
      memory.opcode != aarch64_codegen::MachineOpcode::Load ||
      memory.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      memory.operands.size() != 1 || memory.uses.size() != 1 || memory.defs.size() != 1 ||
      memory.side_effects.size() != 1 ||
      memory.side_effects.front() != aarch64_codegen::MachineSideEffectKind::MemoryRead) {
    return fail("expected memory instruction record to preserve prepared address facts");
  }

  return 0;
}

int call_return_assembler_and_object_families_are_explicit_placeholders() {
  const auto result = make_value_register(prepare::PreparedValueId{21},
                                          c4c::ValueNameId{8},
                                          0);
  const prepare::PreparedCallPlan prepared_call{
      .block_index = 2,
      .instruction_index = 5,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"actual_function"},
      .arguments = {prepare::PreparedCallArgumentPlan{
          .instruction_index = 5,
          .arg_index = 0,
          .value_bank = prepare::PreparedRegisterBank::Gpr,
          .destination_register_name = std::string{"x0"},
      }},
      .result = prepare::PreparedCallResultPlan{
          .instruction_index = 5,
          .value_bank = prepare::PreparedRegisterBank::Gpr,
          .destination_value_id = prepare::PreparedValueId{21},
          .source_register_name = std::string{"x0"},
      },
      .clobbered_registers = {prepare::PreparedClobberedRegister{
          .bank = prepare::PreparedRegisterBank::Gpr,
          .register_name = "x0",
      }},
  };
  const auto call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{4},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "actual_function",
          .arguments =
              {
                  make_value_register(prepare::PreparedValueId{22}, c4c::ValueNameId{9}, 3),
              },
          .result = result,
          .wrapper_kind = prepared_call.wrapper_kind,
          .prepared_arguments = prepared_call.arguments,
          .prepared_result = prepared_call.result,
          .clobbered_registers = prepared_call.clobbered_registers,
          .source_call = &prepared_call,
          .calling_convention = bir::CallingConv::C,
      });
  const auto ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{
          .value = result,
          .value_type = bir::TypeKind::I64,
      });
  const auto prepared_scalar_result = value_register(prepare::PreparedValueId{24},
                                                     c4c::ValueNameId{11},
                                                     0);
  const auto prepared_result_ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{
          .value = aarch64_codegen::make_register_operand(prepared_scalar_result),
          .value_type = bir::TypeKind::I64,
      });
  const auto assembler = aarch64_codegen::make_assembler_instruction(
      aarch64_codegen::AssemblerInstructionRecord{
          .operands = {result},
          .has_inline_asm_payload = true,
          .side_effects = true,
      });
  const auto object = aarch64_codegen::make_object_instruction(
      aarch64_codegen::ObjectInstructionRecord{
          .symbol =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{5},
                  .type = bir::TypeKind::I64,
              },
          .value = result,
          .object_type = bir::TypeKind::I64,
      });

  const auto* call_payload = std::get_if<aarch64_codegen::CallInstructionRecord>(&call.payload);
  const auto* return_payload =
      std::get_if<aarch64_codegen::ReturnInstructionRecord>(&ret.payload);
  const auto* prepared_result_return_payload =
      std::get_if<aarch64_codegen::ReturnInstructionRecord>(&prepared_result_ret.payload);
  const auto* assembler_payload =
      std::get_if<aarch64_codegen::AssemblerInstructionRecord>(&assembler.payload);
  const auto* object_payload =
      std::get_if<aarch64_codegen::ObjectInstructionRecord>(&object.payload);

  if (call_payload == nullptr ||
      aarch64_codegen::instruction_family_name(call.family) != "call" ||
      call.surface != aarch64_codegen::RecordSurfaceKind::MachineInstructionNode ||
      call.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      call.operands.size() != 2 || call.defs.size() != 1 || call.side_effects.size() != 1 ||
      call.side_effects.front() != aarch64_codegen::MachineSideEffectKind::Call ||
      !call_payload->direct_callee.has_value() ||
      call_payload->direct_callee->link_name != c4c::LinkNameId{4} ||
      call_payload->direct_callee_label != "actual_function" ||
      call_payload->wrapper_kind !=
          prepare::PreparedCallWrapperKind::DirectExternFixedArity ||
      call_payload->source_call != &prepared_call ||
      call_payload->prepared_arguments.size() != 1 ||
      !call_payload->prepared_result.has_value() ||
      call_payload->clobbered_registers.size() != 1 ||
      call_payload->arguments.size() != 1 || !call_payload->result.has_value()) {
    return fail("expected call record to preserve explicit callee, argument, and result operands");
  }
  if (return_payload == nullptr ||
      aarch64_codegen::instruction_family_name(ret.family) != "return" ||
      ret.side_effects.size() != 2 ||
      ret.side_effects.front() != aarch64_codegen::MachineSideEffectKind::Return ||
      !return_payload->value.has_value() || return_payload->value_type != bir::TypeKind::I64) {
    return fail("expected return family to be an explicit machine-node placeholder");
  }
  if (prepared_result_return_payload == nullptr ||
      !prepared_result_return_payload->value.has_value() ||
      prepared_result_ret.operands.size() != 1 || prepared_result_ret.uses.size() != 1 ||
      prepared_result_ret.uses.front().kind !=
          aarch64_codegen::MachineEffectResourceKind::Register ||
      prepared_result_ret.uses.front().reg != aarch64_abi::x_register(0) ||
      prepared_result_ret.uses.front().value_id != prepare::PreparedValueId{24}) {
    return fail("expected return record to reference a prepared scalar result register");
  }
  if (assembler_payload == nullptr ||
      aarch64_codegen::instruction_family_name(assembler.family) != "assembler" ||
      assembler.surface != aarch64_codegen::RecordSurfaceKind::ExternalAssemblerInput ||
      assembler.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::DeferredUnsupported ||
      aarch64_codegen::machine_node_selection_status_name(assembler.selection.status) !=
          "deferred_unsupported" ||
      !aarch64_codegen::is_text_first_external_input_surface(assembler.surface) ||
      assembler_payload->operands.size() != 1 || !assembler_payload->has_inline_asm_payload ||
      !assembler_payload->side_effects) {
    return fail("expected assembler family to stay external-input, not a semantic handoff");
  }
  if (object_payload == nullptr ||
      aarch64_codegen::instruction_family_name(object.family) != "object" ||
      object.surface != aarch64_codegen::RecordSurfaceKind::EncoderInput ||
      object.selection.status ==
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      !aarch64_codegen::is_structured_downstream_surface(object.surface) ||
      !object_payload->symbol.has_value() ||
      object_payload->symbol->link_name != c4c::LinkNameId{5} ||
      !object_payload->value.has_value()) {
    return fail("expected object family to preserve symbol and value records");
  }

  return 0;
}

int machine_node_printer_mnemonics_have_one_supported_spelling_source() {
  if (const int status = expect_equal(
          aarch64_codegen::machine_printer_mnemonic_kind_name(
              aarch64_codegen::MachinePrinterMnemonicKind::Store),
          "str",
          "store mnemonic kind");
      status != 0) {
    return status;
  }
  if (const int status = expect_equal(
          aarch64_codegen::machine_printer_mnemonic_kind_name(
              aarch64_codegen::MachinePrinterMnemonicKind::Load),
          "ldr",
          "load mnemonic kind");
      status != 0) {
    return status;
  }
  if (const int status = expect_equal(
          aarch64_codegen::machine_printer_mnemonic_kind_name(
              aarch64_codegen::MachinePrinterMnemonicKind::ConditionalBranchNonZero),
          "cbnz",
          "conditional branch mnemonic kind");
      status != 0) {
    return status;
  }
  if (const int status = expect_equal(
          aarch64_codegen::machine_printer_mnemonic_kind_name(
              aarch64_codegen::MachinePrinterMnemonicKind::Add),
          "add",
          "add mnemonic kind");
      status != 0) {
    return status;
  }
  if (const int status = expect_equal(
          aarch64_codegen::machine_printer_mnemonic_kind_name(
              aarch64_codegen::MachinePrinterMnemonicKind::Sub),
          "sub",
          "sub mnemonic kind");
      status != 0) {
    return status;
  }
  if (const int status = expect_equal(
          aarch64_codegen::machine_printer_mnemonic_kind_name(
              aarch64_codegen::MachinePrinterMnemonicKind::Branch),
          "b",
          "branch mnemonic kind");
      status != 0) {
    return status;
  }
  if (const int status = expect_equal(
          aarch64_codegen::machine_printer_mnemonic_kind_name(
              aarch64_codegen::MachinePrinterMnemonicKind::Move),
          "mov",
          "move mnemonic kind");
      status != 0) {
    return status;
  }
  if (const int status = expect_equal(
          aarch64_codegen::machine_printer_mnemonic_kind_name(
              aarch64_codegen::MachinePrinterMnemonicKind::Return),
          "ret",
          "return mnemonic kind");
      status != 0) {
    return status;
  }

  const auto condition = make_value_register(prepare::PreparedValueId{30},
                                             c4c::ValueNameId{12},
                                             1);
  const auto branch = aarch64_codegen::make_branch_instruction(
      aarch64_codegen::BranchInstructionRecord{
          .target =
              aarch64_codegen::BranchTargetOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .block_label = c4c::BlockLabelId{7},
                  .function_name = c4c::FunctionNameId{2},
                  .condition_value_id = prepare::PreparedValueId{30},
              },
      });
  const auto conditional_branch = aarch64_codegen::make_branch_instruction(
      aarch64_codegen::BranchInstructionRecord{
          .target =
              aarch64_codegen::BranchTargetOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .block_label = c4c::BlockLabelId{7},
                  .function_name = c4c::FunctionNameId{2},
                  .condition_value_id = prepare::PreparedValueId{30},
              },
          .target_pair =
              aarch64_codegen::BranchTargetPairRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .true_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{7},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{30},
                      },
                  .false_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{8},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{30},
                      },
              },
          .condition = condition,
          .condition_record =
              aarch64_codegen::BranchConditionRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .form = aarch64_codegen::BranchConditionForm::MaterializedBool,
                  .condition_value_id = prepare::PreparedValueId{30},
                  .condition_value_name = c4c::ValueNameId{12},
                  .condition_type = bir::TypeKind::I1,
              },
          .conditional = true,
      });

  const auto memory_address = aarch64_codegen::MemoryOperand{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
      .function_name = c4c::FunctionNameId{2},
      .block_label = c4c::BlockLabelId{7},
      .base_kind = aarch64_codegen::MemoryBaseKind::FrameSlot,
      .frame_slot_id = prepare::PreparedFrameSlotId{9},
      .size_bytes = 8,
      .align_bytes = 8,
      .address_space = bir::AddressSpace::Default,
      .can_use_base_plus_offset = true,
  };
  const auto load = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Load,
          .address = memory_address,
          .result_value_id = prepare::PreparedValueId{31},
          .result_value_name = c4c::ValueNameId{13},
          .value_type = bir::TypeKind::I64,
      });

  auto store_address = memory_address;
  store_address.stored_value_id = prepare::PreparedValueId{32};
  store_address.stored_value_name = c4c::ValueNameId{14};
  const auto store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address = store_address,
          .value = make_value_register(prepare::PreparedValueId{32}, c4c::ValueNameId{14}, 2),
          .value_type = bir::TypeKind::I64,
      });
  const auto ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{});
  const auto immediate_ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{
          .value = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::SignedInteger,
                  .type = bir::TypeKind::I32,
                  .signed_value = 0,
              }),
          .value_type = bir::TypeKind::I32,
      });
  const auto scalar = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{33},
          .result_value_name = c4c::ValueNameId{15},
          .result_type = bir::TypeKind::I64,
          .result_register =
              value_register(prepare::PreparedValueId{33}, c4c::ValueNameId{15}, 0),
          .lhs = make_value_register(prepare::PreparedValueId{34}, c4c::ValueNameId{16}, 3),
          .rhs = make_value_register(prepare::PreparedValueId{35}, c4c::ValueNameId{17}, 4),
          .supported_integer_operation = true,
      }));
  const auto sub_scalar = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Sub,
          .source_binary_opcode = bir::BinaryOpcode::Sub,
          .operand_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{36},
          .result_value_name = c4c::ValueNameId{18},
          .result_type = bir::TypeKind::I64,
          .result_register =
              value_register(prepare::PreparedValueId{36}, c4c::ValueNameId{18}, 0),
          .lhs = make_value_register(prepare::PreparedValueId{37}, c4c::ValueNameId{19}, 3),
          .rhs = make_value_register(prepare::PreparedValueId{38}, c4c::ValueNameId{20}, 4),
          .supported_integer_operation = true,
      }));
  const auto call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{8},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "actual_function",
          .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
          .calling_convention = bir::CallingConv::C,
      });

  if (aarch64_codegen::machine_printer_mnemonic_kind_name(
          aarch64_codegen::MachinePrinterMnemonicKind::None) != "" ||
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(branch) != "b" ||
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(conditional_branch) !=
          "cbnz" ||
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(load) != "ldr" ||
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(store) != "str" ||
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(call) != "bl" ||
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(ret) != "ret" ||
      aarch64_codegen::machine_instruction_auxiliary_printer_mnemonic(ret) != "" ||
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(immediate_ret) != "ret" ||
      aarch64_codegen::machine_instruction_auxiliary_printer_mnemonic(immediate_ret) != "mov" ||
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(scalar) != "add" ||
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(sub_scalar) != "sub") {
    return fail("expected supported printer mnemonics to come from the central helper");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status = branch_scalar_and_memory_instruction_records_preserve_typed_operands();
      status != 0) {
    return status;
  }
  if (const int status = call_return_assembler_and_object_families_are_explicit_placeholders();
      status != 0) {
    return status;
  }
  if (const int status = machine_node_printer_mnemonics_have_one_supported_spelling_source();
      status != 0) {
    return status;
  }
  return 0;
}
