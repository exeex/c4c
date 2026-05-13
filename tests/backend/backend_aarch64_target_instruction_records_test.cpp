#include "src/backend/mir/aarch64/codegen/records.hpp"

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

aarch64_codegen::OperandRecord make_value_register(prepare::PreparedValueId value_id,
                                                   c4c::ValueNameId value_name,
                                                   unsigned index) {
  return aarch64_codegen::make_register_operand(aarch64_codegen::RegisterOperand{
      .reg = aarch64_abi::x_register(index),
      .role = aarch64_codegen::RegisterOperandRole::PreparedAssignment,
      .value_id = value_id,
      .value_name = value_name,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = aarch64_abi::RegisterView::X,
      .contiguous_width = 1,
  });
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
      scalar.defs.front().value_id != prepare::PreparedValueId{11}) {
    return fail("expected scalar instruction record to retain BIR opcode and operand records");
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
  const auto call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{4},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .arguments =
              {
                  make_value_register(prepare::PreparedValueId{22}, c4c::ValueNameId{9}, 3),
              },
          .result = result,
          .calling_convention = bir::CallingConv::C,
      });
  const auto ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{
          .value = result,
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
  return 0;
}
