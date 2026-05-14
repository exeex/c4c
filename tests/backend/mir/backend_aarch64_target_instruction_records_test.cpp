#include "src/backend/mir/aarch64/codegen/instruction.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

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
      .preserved_values =
          {prepare::PreparedCallPreservedValue{
               .value_id = prepare::PreparedValueId{40},
               .value_name = c4c::ValueNameId{12},
               .route = prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
               .callee_saved_save_index = std::size_t{0},
               .contiguous_width = 1,
               .register_name = std::string{"x19"},
               .register_bank = prepare::PreparedRegisterBank::Gpr,
               .occupied_register_names = {"x19"},
           },
           prepare::PreparedCallPreservedValue{
               .value_id = prepare::PreparedValueId{41},
               .value_name = c4c::ValueNameId{13},
               .route = prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
               .contiguous_width = 1,
               .register_bank = prepare::PreparedRegisterBank::Gpr,
           }},
      .clobbered_registers = {prepare::PreparedClobberedRegister{
                                  .bank = prepare::PreparedRegisterBank::Gpr,
                                  .register_name = "x0",
                              },
                              prepare::PreparedClobberedRegister{
                                  .bank = prepare::PreparedRegisterBank::Fpr,
                                  .register_name = "d13",
                                  .contiguous_width = 1,
                                  .occupied_register_names = {"d13"},
                              },
                              prepare::PreparedClobberedRegister{
                                  .bank = prepare::PreparedRegisterBank::Gpr,
                                  .register_name = "not_a_register",
                                  .contiguous_width = 1,
                                  .occupied_register_names = {"not_a_register"},
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
          .preserved_values = prepared_call.preserved_values,
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
      call_payload->preserved_values.size() != 2 ||
      call_payload->clobbered_registers.size() != 3 ||
      call_payload->arguments.size() != 1 || !call_payload->result.has_value()) {
    return fail("expected call record to preserve explicit callee, argument, and result operands");
  }
  if (call.preserves.size() != 1 ||
      call.preserves.front().kind != aarch64_codegen::MachineEffectResourceKind::Register ||
      call.preserves.front().reg != aarch64_abi::x_register(19) ||
      call.preserves.front().value_id != prepare::PreparedValueId{40} ||
      call.preserves.front().value_name != c4c::ValueNameId{12} ||
      !call.preserves.front().operand.has_value()) {
    return fail("expected call node to expose only explicit prepared preserved-value effects");
  }
  const auto* preserved_effect =
      std::get_if<aarch64_codegen::RegisterOperand>(&call.preserves.front().operand->payload);
  if (preserved_effect == nullptr ||
      preserved_effect->role != aarch64_codegen::RegisterOperandRole::CallAbi ||
      preserved_effect->prepared_bank != prepare::PreparedRegisterBank::Gpr ||
      preserved_effect->reg != aarch64_abi::x_register(19) ||
      preserved_effect->value_id != prepare::PreparedValueId{40} ||
      preserved_effect->value_name != c4c::ValueNameId{12}) {
    return fail("expected preserved-value effect operand to retain prepared register carrier");
  }
  if (call.clobbers.size() != 2 ||
      call.clobbers[0].kind != aarch64_codegen::MachineEffectResourceKind::Register ||
      call.clobbers[0].reg != aarch64_abi::x_register(0) ||
      call.clobbers[1].kind != aarch64_codegen::MachineEffectResourceKind::Register ||
      call.clobbers[1].reg != aarch64_abi::d_register(13)) {
    return fail("expected call node to expose convertible prepared clobbers as register effects");
  }
  if (!call.clobbers[0].operand.has_value() || !call.clobbers[1].operand.has_value()) {
    return fail("expected call clobber effects to carry register operands");
  }
  const auto* gpr_clobber =
      std::get_if<aarch64_codegen::RegisterOperand>(&call.clobbers[0].operand->payload);
  const auto* fpr_clobber =
      std::get_if<aarch64_codegen::RegisterOperand>(&call.clobbers[1].operand->payload);
  if (gpr_clobber == nullptr || fpr_clobber == nullptr ||
      gpr_clobber->role != aarch64_codegen::RegisterOperandRole::CallAbi ||
      gpr_clobber->prepared_bank != prepare::PreparedRegisterBank::Gpr ||
      gpr_clobber->occupied_register_references !=
          std::vector<aarch64_abi::RegisterReference>{aarch64_abi::x_register(0)} ||
      fpr_clobber->role != aarch64_codegen::RegisterOperandRole::CallAbi ||
      fpr_clobber->prepared_bank != prepare::PreparedRegisterBank::Fpr ||
      fpr_clobber->occupied_register_references !=
          std::vector<aarch64_abi::RegisterReference>{aarch64_abi::d_register(13)}) {
    return fail("expected prepared clobber operands to preserve call ABI register metadata");
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

int memory_return_call_record_exposes_prepared_frame_slot_storage() {
  const prepare::PreparedMemoryReturnPlan memory_return{
      .sret_arg_index = std::size_t{0},
      .storage_slot_name = c4c::SlotNameId{8},
      .encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
      .slot_id = prepare::PreparedFrameSlotId{12},
      .stack_offset_bytes = std::size_t{48},
      .size_bytes = 24,
      .align_bytes = 8,
  };
  const prepare::PreparedCallPlan prepared_call{
      .block_index = 2,
      .instruction_index = 5,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"make_large"},
      .memory_return = memory_return,
  };
  const auto call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{4},
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
                  .instruction_index = 5,
                  .base_kind = aarch64_codegen::MemoryBaseKind::FrameSlot,
                  .frame_slot_id = prepare::PreparedFrameSlotId{12},
                  .byte_offset = 48,
                  .byte_offset_is_prepared_snapshot = true,
                  .size_bytes = 24,
                  .align_bytes = 8,
                  .can_use_base_plus_offset = true,
              },
          .source_call = &prepared_call,
          .calling_convention = bir::CallingConv::C,
      });
  const auto* call_payload = std::get_if<aarch64_codegen::CallInstructionRecord>(&call.payload);

  if (call_payload == nullptr || !call_payload->memory_return.has_value() ||
      !call_payload->memory_return_storage.has_value() ||
      call_payload->memory_return->slot_id != std::optional<prepare::PreparedFrameSlotId>{12} ||
      call_payload->memory_return->stack_offset_bytes != std::optional<std::size_t>{48} ||
      call_payload->memory_return_storage->base_kind != aarch64_codegen::MemoryBaseKind::FrameSlot ||
      call_payload->memory_return_storage->frame_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{12} ||
      call_payload->memory_return_storage->byte_offset != 48 ||
      call_payload->memory_return_storage->size_bytes != 24 ||
      call_payload->memory_return_storage->align_bytes != 8) {
    return fail("expected memory-return call payload to preserve prepared frame-slot storage");
  }
  if (call.operands.size() != 1 || call.defs.size() != 1 ||
      call.defs.front().kind != aarch64_codegen::MachineEffectResourceKind::Memory ||
      call.defs.front().frame_slot_id != std::optional<prepare::PreparedFrameSlotId>{12} ||
      !call.defs.front().operand.has_value() || call.side_effects.size() != 2 ||
      call.side_effects.front() != aarch64_codegen::MachineSideEffectKind::Call ||
      call.side_effects.back() != aarch64_codegen::MachineSideEffectKind::MemoryWrite) {
    return fail("expected memory-return call to expose prepared storage as a memory def");
  }
  const auto* storage =
      std::get_if<aarch64_codegen::MemoryOperand>(&call.defs.front().operand->payload);
  if (storage == nullptr || storage->base_kind != aarch64_codegen::MemoryBaseKind::FrameSlot ||
      storage->frame_slot_id != std::optional<prepare::PreparedFrameSlotId>{12} ||
      storage->byte_offset != 48) {
    return fail("expected memory-return def operand to carry prepared slot and offset");
  }

  return 0;
}

int frame_instruction_records_preserve_prepared_frame_facts() {
  const prepare::PreparedFramePlanFunction prepared_frame{
      .function_name = c4c::FunctionNameId{6},
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
      .frame_slot_order = {prepare::PreparedFrameSlotId{12}},
      .has_dynamic_stack = true,
      .uses_frame_pointer_for_fixed_slots = true,
  };
  const auto prologue = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::PrologueSetup,
          .function_name = prepared_frame.function_name,
          .frame_size_bytes = prepared_frame.frame_size_bytes,
          .frame_alignment_bytes = prepared_frame.frame_alignment_bytes,
          .frame_slot_order = prepared_frame.frame_slot_order,
          .saved_callee_registers = prepared_frame.saved_callee_registers,
          .has_dynamic_stack = prepared_frame.has_dynamic_stack,
          .uses_frame_pointer_for_fixed_slots =
              prepared_frame.uses_frame_pointer_for_fixed_slots,
          .source_frame = &prepared_frame,
      });
  const auto epilogue = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::EpilogueTeardown,
          .function_name = prepared_frame.function_name,
          .frame_size_bytes = prepared_frame.frame_size_bytes,
          .frame_alignment_bytes = prepared_frame.frame_alignment_bytes,
          .source_frame = &prepared_frame,
      });
  const auto callee_save = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::CalleeSaveStore,
          .function_name = prepared_frame.function_name,
          .frame_size_bytes = prepared_frame.frame_size_bytes,
          .frame_alignment_bytes = prepared_frame.frame_alignment_bytes,
          .saved_callee_registers = prepared_frame.saved_callee_registers,
          .callee_save =
              aarch64_codegen::CalleeSaveInstructionRecord{
                  .saved_register = prepared_frame.saved_callee_registers.front(),
                  .register_operand = value_register(prepare::PreparedValueId{50},
                                                     c4c::ValueNameId{51},
                                                     19),
                  .slot_id = prepare::PreparedFrameSlotId{12},
                  .stack_offset_bytes = std::size_t{16},
                  .stack_offset_is_prepared_snapshot = true,
              },
          .source_frame = &prepared_frame,
      });

  const auto* prologue_payload =
      std::get_if<aarch64_codegen::FrameInstructionRecord>(&prologue.payload);
  const auto* epilogue_payload =
      std::get_if<aarch64_codegen::FrameInstructionRecord>(&epilogue.payload);
  const auto* callee_save_payload =
      std::get_if<aarch64_codegen::FrameInstructionRecord>(&callee_save.payload);

  if (prologue_payload == nullptr ||
      aarch64_codegen::instruction_family_name(prologue.family) != "frame" ||
      prologue.opcode != aarch64_codegen::MachineOpcode::FrameSetup ||
      aarch64_codegen::machine_opcode_name(prologue.opcode) != "frame_setup" ||
      prologue.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      prologue.side_effects.size() != 1 ||
      prologue.side_effects.front() != aarch64_codegen::MachineSideEffectKind::FrameSetup ||
      aarch64_codegen::machine_side_effect_kind_name(prologue.side_effects.front()) !=
          "frame_setup" ||
      prologue_payload->source_frame != &prepared_frame ||
      prologue_payload->frame_size_bytes != 32 ||
      prologue_payload->frame_alignment_bytes != 16 ||
      !prologue_payload->has_dynamic_stack ||
      !prologue_payload->uses_frame_pointer_for_fixed_slots ||
      prologue_payload->frame_slot_order.size() != 1 ||
      prologue_payload->frame_slot_order.front() != prepare::PreparedFrameSlotId{12} ||
      prologue_payload->saved_callee_registers.size() != 1 ||
      prologue_payload->saved_callee_registers.front().register_name != "x19" ||
      aarch64_codegen::frame_instruction_kind_name(prologue_payload->frame_kind) !=
          "prologue_setup") {
    return fail("expected frame prologue record to preserve prepared frame facts");
  }
  if (epilogue_payload == nullptr ||
      epilogue.opcode != aarch64_codegen::MachineOpcode::FrameTeardown ||
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(epilogue) != "add" ||
      epilogue.side_effects.size() != 1 ||
      epilogue.side_effects.front() != aarch64_codegen::MachineSideEffectKind::FrameTeardown ||
      aarch64_codegen::frame_instruction_kind_name(epilogue_payload->frame_kind) !=
          "epilogue_teardown") {
    return fail("expected frame epilogue record to preserve teardown identity");
  }
  if (callee_save_payload == nullptr ||
      callee_save.opcode != aarch64_codegen::MachineOpcode::CalleeSaveStore ||
      aarch64_codegen::machine_opcode_name(callee_save.opcode) != "callee_save_store" ||
      aarch64_codegen::frame_instruction_kind_name(callee_save_payload->frame_kind) !=
          "callee_save_store" ||
      !callee_save_payload->callee_save.has_value() ||
      callee_save_payload->callee_save->saved_register.register_name != "x19" ||
      callee_save_payload->callee_save->slot_id != prepare::PreparedFrameSlotId{12} ||
      !callee_save_payload->callee_save->stack_offset_is_prepared_snapshot ||
      callee_save.uses.empty() ||
      callee_save.side_effects.size() != 1 ||
      callee_save.side_effects.front() != aarch64_codegen::MachineSideEffectKind::MemoryWrite) {
    return fail("expected callee-save frame record to carry prepared save provenance");
  }

  return 0;
}

int call_boundary_records_preserve_prepared_move_and_abi_binding_facts() {
  const prepare::PreparedMoveBundle bundle{
      .function_name = c4c::FunctionNameId{9},
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
      .block_index = 3,
      .instruction_index = 7,
      .source_parallel_copy_predecessor_label = c4c::BlockLabelId{11},
      .source_parallel_copy_successor_label = c4c::BlockLabelId{12},
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = prepare::PreparedValueId{60},
                  .to_value_id = prepare::PreparedValueId{61},
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{1},
                  .destination_register_name = std::string{"x1"},
                  .destination_contiguous_width = 1,
                  .destination_occupied_register_names = {"x1"},
                  .block_index = 3,
                  .instruction_index = 7,
                  .uses_cycle_temp_source = true,
                  .source_parallel_copy_step_index = std::size_t{2},
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  .source_parallel_copy_predecessor_label = c4c::BlockLabelId{11},
                  .source_parallel_copy_successor_label = c4c::BlockLabelId{12},
                  .reason = "before-call arg move",
              },
          },
      .abi_bindings =
          {
              prepare::PreparedAbiBinding{
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{1},
                  .destination_register_name = std::string{"x1"},
                  .destination_contiguous_width = 1,
                  .destination_occupied_register_names = {"x1"},
              },
          },
  };
  const auto move = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = bundle.function_name,
          .phase = bundle.phase,
          .authority_kind = bundle.authority_kind,
          .block_index = bundle.block_index,
          .instruction_index = bundle.instruction_index,
          .source_parallel_copy_predecessor_label =
              bundle.source_parallel_copy_predecessor_label,
          .source_parallel_copy_successor_label =
              bundle.source_parallel_copy_successor_label,
          .move = bundle.moves.front(),
          .source_register =
              aarch64_codegen::RegisterOperand{
                  .reg = aarch64_abi::x_register(2),
                  .role = aarch64_codegen::RegisterOperandRole::CallAbi,
                  .value_id = prepare::PreparedValueId{60},
                  .value_name = c4c::ValueNameId{14},
                  .prepared_class = prepare::PreparedRegisterClass::General,
                  .prepared_bank = prepare::PreparedRegisterBank::Gpr,
                  .expected_view = aarch64_abi::RegisterView::X,
                  .contiguous_width = 1,
                  .occupied_registers = {"x2"},
              },
          .destination_register =
              aarch64_codegen::RegisterOperand{
                  .reg = aarch64_abi::x_register(1),
                  .role = aarch64_codegen::RegisterOperandRole::CallAbi,
                  .value_id = prepare::PreparedValueId{61},
                  .value_name = c4c::ValueNameId{14},
                  .prepared_class = prepare::PreparedRegisterClass::General,
                  .prepared_bank = prepare::PreparedRegisterBank::Gpr,
                  .expected_view = aarch64_abi::RegisterView::X,
                  .contiguous_width = 1,
                  .occupied_registers = {"x1"},
              },
          .source_bundle = &bundle,
          .source_move = &bundle.moves.front(),
      });
  const auto binding = aarch64_codegen::make_call_boundary_abi_binding_instruction(
      aarch64_codegen::CallBoundaryAbiBindingInstructionRecord{
          .function_name = bundle.function_name,
          .phase = bundle.phase,
          .authority_kind = bundle.authority_kind,
          .block_index = bundle.block_index,
          .instruction_index = bundle.instruction_index,
          .binding_index = 0,
          .source_parallel_copy_predecessor_label =
              bundle.source_parallel_copy_predecessor_label,
          .source_parallel_copy_successor_label =
              bundle.source_parallel_copy_successor_label,
          .binding = bundle.abi_bindings.front(),
          .source_bundle = &bundle,
          .source_binding = &bundle.abi_bindings.front(),
      });

  const auto* move_payload =
      std::get_if<aarch64_codegen::CallBoundaryMoveInstructionRecord>(&move.payload);
  const auto* binding_payload =
      std::get_if<aarch64_codegen::CallBoundaryAbiBindingInstructionRecord>(&binding.payload);

  if (move_payload == nullptr ||
      aarch64_codegen::instruction_family_name(move.family) != "call_boundary" ||
      move.opcode != aarch64_codegen::MachineOpcode::CallBoundaryMove ||
      aarch64_codegen::machine_opcode_name(move.opcode) != "call_boundary_move" ||
      move.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      move.function_name != c4c::FunctionNameId{9} ||
      move.block_index != 3 || move.instruction_index != 7 ||
      move.operands.size() != 2 || move.uses.size() != 1 || move.defs.size() != 1 ||
      move.uses.front().reg != aarch64_abi::x_register(2) ||
      move.defs.front().reg != aarch64_abi::x_register(1) ||
      move_payload->source_bundle != &bundle ||
      move_payload->source_move != &bundle.moves.front() ||
      !move_payload->source_register.has_value() ||
      !move_payload->destination_register.has_value() ||
      move_payload->source_register->reg != aarch64_abi::x_register(2) ||
      move_payload->destination_register->reg != aarch64_abi::x_register(1) ||
      move_payload->phase != prepare::PreparedMovePhase::BeforeCall ||
      prepare::prepared_move_phase_name(move_payload->phase) != "before_call" ||
      move_payload->authority_kind !=
          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
      prepare::prepared_move_authority_kind_name(move_payload->authority_kind) !=
          "out_of_ssa_parallel_copy" ||
      move_payload->source_parallel_copy_predecessor_label != c4c::BlockLabelId{11} ||
      move_payload->source_parallel_copy_successor_label != c4c::BlockLabelId{12} ||
      move_payload->move.from_value_id != prepare::PreparedValueId{60} ||
      move_payload->move.to_value_id != prepare::PreparedValueId{61} ||
      move_payload->move.destination_kind !=
          prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      move_payload->move.destination_storage_kind !=
          prepare::PreparedMoveStorageKind::Register ||
      move_payload->move.destination_abi_index != std::size_t{1} ||
      move_payload->move.destination_register_name != std::string{"x1"} ||
      move_payload->move.destination_occupied_register_names.size() != 1 ||
      move_payload->move.destination_occupied_register_names.front() != "x1" ||
      !move_payload->move.uses_cycle_temp_source ||
      move_payload->move.source_parallel_copy_step_index != std::size_t{2} ||
      move_payload->move.reason != "before-call arg move") {
    return fail("expected call-boundary move record to preserve prepared move facts");
  }
  if (binding_payload == nullptr ||
      binding.family != aarch64_codegen::InstructionFamily::CallBoundary ||
      binding.opcode != aarch64_codegen::MachineOpcode::CallBoundaryAbiBinding ||
      aarch64_codegen::machine_opcode_name(binding.opcode) !=
          "call_boundary_abi_binding" ||
      binding.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      binding.function_name != c4c::FunctionNameId{9} ||
      binding_payload->source_bundle != &bundle ||
      binding_payload->source_binding != &bundle.abi_bindings.front() ||
      binding_payload->binding_index != 0 ||
      binding_payload->phase != prepare::PreparedMovePhase::BeforeCall ||
      binding_payload->binding.destination_kind !=
          prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      binding_payload->binding.destination_storage_kind !=
          prepare::PreparedMoveStorageKind::Register ||
      binding_payload->binding.destination_abi_index != std::size_t{1} ||
      binding_payload->binding.destination_register_name != std::string{"x1"} ||
      binding_payload->binding.destination_occupied_register_names.size() != 1 ||
      binding_payload->binding.destination_occupied_register_names.front() != "x1") {
    return fail("expected call-boundary ABI binding record to preserve prepared binding facts");
  }

  return 0;
}

int after_call_result_move_record_selects_register_to_register_subset() {
  const prepare::PreparedMoveBundle bundle{
      .function_name = c4c::FunctionNameId{10},
      .phase = prepare::PreparedMovePhase::AfterCall,
      .block_index = 2,
      .instruction_index = 5,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = prepare::PreparedValueId{71},
                  .to_value_id = prepare::PreparedValueId{71},
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallResultAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"x0"},
                  .destination_contiguous_width = 1,
                  .destination_occupied_register_names = {"x0"},
                  .block_index = 2,
                  .instruction_index = 5,
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .reason = "after-call result move",
              },
          },
  };
  const auto move = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = bundle.function_name,
          .phase = bundle.phase,
          .block_index = bundle.block_index,
          .instruction_index = bundle.instruction_index,
          .move = bundle.moves.front(),
          .source_register =
              aarch64_codegen::RegisterOperand{
                  .reg = aarch64_abi::x_register(0),
                  .role = aarch64_codegen::RegisterOperandRole::CallAbi,
                  .value_id = prepare::PreparedValueId{71},
                  .value_name = c4c::ValueNameId{17},
                  .prepared_class = prepare::PreparedRegisterClass::General,
                  .prepared_bank = prepare::PreparedRegisterBank::Gpr,
                  .expected_view = aarch64_abi::RegisterView::X,
                  .contiguous_width = 1,
                  .occupied_registers = {"x0"},
              },
          .destination_register =
              aarch64_codegen::RegisterOperand{
                  .reg = aarch64_abi::x_register(3),
                  .role = aarch64_codegen::RegisterOperandRole::CallAbi,
                  .value_id = prepare::PreparedValueId{71},
                  .value_name = c4c::ValueNameId{17},
                  .prepared_class = prepare::PreparedRegisterClass::General,
                  .prepared_bank = prepare::PreparedRegisterBank::Gpr,
                  .expected_view = aarch64_abi::RegisterView::X,
                  .contiguous_width = 1,
                  .occupied_registers = {"x3"},
              },
          .source_bundle = &bundle,
          .source_move = &bundle.moves.front(),
      });
  const auto* payload =
      std::get_if<aarch64_codegen::CallBoundaryMoveInstructionRecord>(&move.payload);

  if (payload == nullptr ||
      move.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      move.operands.size() != 2 || move.uses.size() != 1 || move.defs.size() != 1 ||
      move.uses.front().reg != aarch64_abi::x_register(0) ||
      move.defs.front().reg != aarch64_abi::x_register(3) ||
      payload->phase != prepare::PreparedMovePhase::AfterCall ||
      payload->move.destination_kind !=
          prepare::PreparedMoveDestinationKind::CallResultAbi ||
      payload->move.destination_register_name != std::optional<std::string>{"x0"} ||
      payload->source_register->reg != aarch64_abi::x_register(0) ||
      payload->destination_register->reg != aarch64_abi::x_register(3)) {
    return fail("expected after-call result move record to select prepared x0 -> x3");
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
  const prepare::PreparedFramePlanFunction prepared_frame{
      .function_name = c4c::FunctionNameId{2},
      .frame_size_bytes = 16,
      .frame_alignment_bytes = 16,
  };
  const auto frame_setup = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::PrologueSetup,
          .function_name = prepared_frame.function_name,
          .frame_size_bytes = prepared_frame.frame_size_bytes,
          .frame_alignment_bytes = prepared_frame.frame_alignment_bytes,
          .source_frame = &prepared_frame,
      });
  const auto frame_teardown = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::EpilogueTeardown,
          .function_name = prepared_frame.function_name,
          .frame_size_bytes = prepared_frame.frame_size_bytes,
          .frame_alignment_bytes = prepared_frame.frame_alignment_bytes,
          .source_frame = &prepared_frame,
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
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(sub_scalar) != "sub" ||
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(frame_setup) != "sub" ||
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(frame_teardown) != "add") {
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
  if (const int status = memory_return_call_record_exposes_prepared_frame_slot_storage();
      status != 0) {
    return status;
  }
  if (const int status = frame_instruction_records_preserve_prepared_frame_facts();
      status != 0) {
    return status;
  }
  if (const int status = call_boundary_records_preserve_prepared_move_and_abi_binding_facts();
      status != 0) {
    return status;
  }
  if (const int status = after_call_result_move_record_selects_register_to_register_subset();
      status != 0) {
    return status;
  }
  if (const int status = machine_node_printer_mnemonics_have_one_supported_spelling_source();
      status != 0) {
    return status;
  }
  return 0;
}
