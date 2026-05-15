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

aarch64_codegen::MemoryOperand f128_frame_slot_memory_operand(
    c4c::FunctionNameId function_name,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index) {
  return aarch64_codegen::MemoryOperand{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .function_name = function_name,
      .block_label = block_label,
      .instruction_index = instruction_index,
      .base_kind = aarch64_codegen::MemoryBaseKind::FrameSlot,
      .frame_slot_id = prepare::PreparedFrameSlotId{40},
      .byte_offset = 32,
      .size_bytes = 16,
      .align_bytes = 16,
      .address_space = bir::AddressSpace::Default,
  };
}

prepare::PreparedF128Carrier f128_full_width_register_carrier(
    c4c::FunctionNameId function_name,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    const char* register_name) {
  return prepare::PreparedF128Carrier{
      .function_name = function_name,
      .value_id = value_id,
      .value_name = value_name,
      .source_type = bir::TypeKind::F128,
      .kind = prepare::PreparedF128CarrierKind::FullWidthRegister,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
      .register_bank = prepare::PreparedRegisterBank::Vreg,
      .register_class = prepare::PreparedRegisterClass::Vector,
      .contiguous_width = 1,
      .register_name = std::string{register_name},
      .occupied_register_names = {register_name},
  };
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

int stack_slot_preserved_value_consumes_prepared_size_alignment_extent() {
  const prepare::PreparedCallPlan prepared_call{
      .block_index = 0,
      .instruction_index = 1,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"actual_function"},
      .preserved_values = {prepare::PreparedCallPreservedValue{
          .value_id = prepare::PreparedValueId{70},
          .value_name = c4c::ValueNameId{31},
          .route = prepare::PreparedCallPreservationRoute::StackSlot,
          .contiguous_width = 1,
          .slot_id = prepare::PreparedFrameSlotId{15},
          .stack_offset_bytes = std::size_t{88},
          .stack_size_bytes = std::size_t{16},
          .stack_align_bytes = std::size_t{8},
          .spill_slot_placement =
              prepare::PreparedSpillSlotPlacement{
                  .slot_id = prepare::PreparedFrameSlotId{15},
                  .offset_bytes = 88,
              },
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
          .wrapper_kind = prepared_call.wrapper_kind,
          .preserved_values = prepared_call.preserved_values,
          .source_call = &prepared_call,
          .calling_convention = bir::CallingConv::C,
      });

  const auto* call_payload = std::get_if<aarch64_codegen::CallInstructionRecord>(&call.payload);
  if (call_payload == nullptr || call_payload->preserved_values.size() != 1 ||
      call_payload->preserved_values.front().route !=
          prepare::PreparedCallPreservationRoute::StackSlot ||
      call_payload->preserved_values.front().slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{15} ||
      call_payload->preserved_values.front().stack_offset_bytes !=
          std::optional<std::size_t>{88} ||
      call_payload->preserved_values.front().stack_size_bytes !=
          std::optional<std::size_t>{16} ||
      call_payload->preserved_values.front().stack_align_bytes !=
          std::optional<std::size_t>{8}) {
    return fail("expected stack-slot preserved value to remain raw prepared provenance");
  }
  if (call.preserves.size() != 1 ||
      call.preserves.front().kind != aarch64_codegen::MachineEffectResourceKind::Memory ||
      call.preserves.front().value_id != prepare::PreparedValueId{70} ||
      call.preserves.front().value_name != c4c::ValueNameId{31} ||
      call.preserves.front().frame_slot_id != prepare::PreparedFrameSlotId{15} ||
      !call.preserves.front().operand.has_value()) {
    return fail("expected stack-slot preserved value to expose prepared memory-preserve effect");
  }
  const auto* memory =
      std::get_if<aarch64_codegen::MemoryOperand>(&call.preserves.front().operand->payload);
  if (memory == nullptr ||
      memory->support != aarch64_codegen::MemoryOperandSupportKind::Prepared ||
      memory->base_kind != aarch64_codegen::MemoryBaseKind::FrameSlot ||
      memory->frame_slot_id != prepare::PreparedFrameSlotId{15} ||
      memory->byte_offset != 88 ||
      !memory->byte_offset_is_prepared_snapshot ||
      memory->size_bytes != 16 ||
      memory->align_bytes != 8 ||
      !memory->can_use_base_plus_offset) {
    return fail("expected stack-slot memory-preserve effect to use prepared slot extent facts");
  }

  return 0;
}

int stack_slot_preserved_value_fails_closed_without_prepared_extent() {
  auto preserved = [](prepare::PreparedValueId value_id,
                      c4c::ValueNameId value_name,
                      std::optional<std::size_t> size,
                      std::optional<std::size_t> align) {
    return prepare::PreparedCallPreservedValue{
        .value_id = value_id,
        .value_name = value_name,
        .route = prepare::PreparedCallPreservationRoute::StackSlot,
        .contiguous_width = 1,
        .slot_id = prepare::PreparedFrameSlotId{15},
        .stack_offset_bytes = std::size_t{88},
        .stack_size_bytes = size,
        .stack_align_bytes = align,
        .spill_slot_placement =
            prepare::PreparedSpillSlotPlacement{
                .slot_id = prepare::PreparedFrameSlotId{15},
                .offset_bytes = 88,
            },
    };
  };
  const prepare::PreparedCallPlan missing_size_call{
      .block_index = 0,
      .instruction_index = 1,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"actual_function"},
      .preserved_values =
          {preserved(prepare::PreparedValueId{71}, c4c::ValueNameId{32}, std::nullopt, 8)},
  };
  const prepare::PreparedCallPlan missing_align_call{
      .block_index = 0,
      .instruction_index = 1,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"actual_function"},
      .preserved_values =
          {preserved(prepare::PreparedValueId{72}, c4c::ValueNameId{33}, 16, std::nullopt)},
  };

  const auto make_call = [](const prepare::PreparedCallPlan& prepared_call) {
    return aarch64_codegen::make_call_instruction(
        aarch64_codegen::CallInstructionRecord{
            .direct_callee =
                aarch64_codegen::SymbolOperand{
                    .link_name = c4c::LinkNameId{4},
                    .type = bir::TypeKind::Ptr,
                    .is_extern = true,
                },
            .direct_callee_label = "actual_function",
            .wrapper_kind = prepared_call.wrapper_kind,
            .preserved_values = prepared_call.preserved_values,
            .source_call = &prepared_call,
            .calling_convention = bir::CallingConv::C,
        });
  };
  const auto missing_size = make_call(missing_size_call);
  const auto missing_align = make_call(missing_align_call);
  if (!missing_size.preserves.empty()) {
    return fail("expected stack-slot preserved value without prepared size to fail closed");
  }
  if (!missing_align.preserves.empty()) {
    return fail("expected stack-slot preserved value without prepared alignment to fail closed");
  }
  const auto* missing_size_payload =
      std::get_if<aarch64_codegen::CallInstructionRecord>(&missing_size.payload);
  const auto* missing_align_payload =
      std::get_if<aarch64_codegen::CallInstructionRecord>(&missing_align.payload);
  if (missing_size_payload == nullptr || missing_align_payload == nullptr ||
      missing_size_payload->preserved_values.size() != 1 ||
      missing_align_payload->preserved_values.size() != 1 ||
      missing_size_payload->preserved_values.front().stack_align_bytes !=
          std::optional<std::size_t>{8} ||
      missing_align_payload->preserved_values.front().stack_size_bytes !=
          std::optional<std::size_t>{16}) {
    return fail("expected incomplete stack-slot preserved values to retain raw provenance");
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

int variadic_entry_helper_call_records_select_prepared_va_start() {
  const prepare::PreparedCallPlan prepared_call{
      .block_index = 0,
      .instruction_index = 3,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"llvm.va_start.p0"},
  };
  const prepare::PreparedVariadicEntryHelperOperandHomes va_start_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaStart,
      .block_index = 0,
      .instruction_index = 3,
      .destination_va_list =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{44},
              .function_name = c4c::FunctionNameId{9},
              .value_name = c4c::ValueNameId{7},
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x3"},
          },
  };
  prepare::PreparedVariadicEntryPlanFunction variadic_entry{
      .function_name = c4c::FunctionNameId{9},
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

  const auto helper_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{23},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_start.p0",
          .wrapper_kind = prepared_call.wrapper_kind,
          .source_call = &prepared_call,
          .source_variadic_entry = &variadic_entry,
          .source_variadic_helper_operand_homes = &variadic_entry.helper_operand_homes.front(),
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaStart,
          .calling_convention = bir::CallingConv::C,
      });
  const auto missing_entry_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{23},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_start.p0",
          .wrapper_kind = prepared_call.wrapper_kind,
          .source_call = &prepared_call,
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaStart,
          .calling_convention = bir::CallingConv::C,
      });

  const auto* helper_payload =
      std::get_if<aarch64_codegen::CallInstructionRecord>(&helper_call.payload);
  const auto* missing_payload =
      std::get_if<aarch64_codegen::CallInstructionRecord>(&missing_entry_call.payload);
  if (helper_payload == nullptr ||
      helper_call.family != aarch64_codegen::InstructionFamily::Call ||
      helper_call.opcode != aarch64_codegen::MachineOpcode::VariadicVaStart ||
      helper_call.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      helper_payload->source_call != &prepared_call ||
      helper_payload->source_variadic_entry != &variadic_entry ||
      helper_payload->source_variadic_helper_operand_homes !=
          &variadic_entry.helper_operand_homes.front() ||
      !helper_payload->source_variadic_helper_operand_homes->destination_va_list.has_value() ||
      helper_payload->source_variadic_entry->helper_resources.scratch_register_count !=
          std::optional<std::size_t>{1} ||
      helper_payload->source_variadic_entry->helper_resources.scratch_stack_bytes !=
          std::optional<std::size_t>{0} ||
      helper_payload->variadic_entry_helper !=
          std::optional<prepare::PreparedVariadicEntryHelperKind>{
              prepare::PreparedVariadicEntryHelperKind::VaStart} ||
      !helper_payload->variadic_va_start.has_value() ||
      helper_payload->variadic_va_start->register_save_area_slot_id !=
          prepare::PreparedFrameSlotId{5} ||
      helper_payload->variadic_va_start->overflow_area_base_slot_id !=
          prepare::PreparedFrameSlotId{6} ||
      helper_payload->variadic_va_start->va_list_fields.size() != 2 ||
      helper_call.defs.empty() ||
      !helper_payload->direct_callee.has_value() ||
      helper_payload->direct_callee_label != "llvm.va_start.p0") {
    return fail("expected va_start call record to select prepared machine-node facts");
  }
  if (missing_payload == nullptr ||
      missing_entry_call.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::MissingRequiredFacts ||
      missing_entry_call.selection.diagnostic !=
          "variadic entry helper node is missing prepared entry provenance" ||
      missing_payload->source_variadic_entry != nullptr) {
    return fail("expected variadic helper call without prepared entry to fail closed");
  }

  return 0;
}

int scalar_va_arg_call_record_requires_prepared_access_plan() {
  const prepare::PreparedCallPlan prepared_call{
      .block_index = 0,
      .instruction_index = 4,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"llvm.va_arg.i32"},
  };
  const prepare::PreparedVariadicEntryHelperOperandHomes va_arg_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaArg,
      .block_index = 0,
      .instruction_index = 4,
      .source_va_list =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{50},
              .function_name = c4c::FunctionNameId{9},
              .value_name = c4c::ValueNameId{10},
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x2"},
          },
      .scalar_result =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{51},
              .function_name = c4c::FunctionNameId{9},
              .value_name = c4c::ValueNameId{11},
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"w0"},
          },
  };
  prepare::PreparedVariadicEntryPlanFunction variadic_entry{
      .function_name = c4c::FunctionNameId{9},
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
                  },
          },
      .helper_resources =
          prepare::PreparedVariadicEntryHelperResources{
              .required_helpers = {prepare::PreparedVariadicEntryHelperKind::VaArg},
              .scratch_register_count = std::size_t{2},
              .scratch_stack_bytes = std::size_t{0},
          },
      .helper_operand_homes = {va_arg_homes},
  };

  const auto helper_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{24},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_arg.i32",
          .wrapper_kind = prepared_call.wrapper_kind,
          .source_call = &prepared_call,
          .source_variadic_entry = &variadic_entry,
          .source_variadic_helper_operand_homes = &variadic_entry.helper_operand_homes.front(),
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaArg,
          .calling_convention = bir::CallingConv::C,
      });
  const auto* helper_payload =
      std::get_if<aarch64_codegen::CallInstructionRecord>(&helper_call.payload);
  if (helper_payload == nullptr ||
      helper_call.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::MissingRequiredFacts ||
      helper_call.selection.diagnostic.find(
          "helper_operand_homes.va_arg.scalar_access_plan") == std::string::npos ||
      helper_payload->source_variadic_entry != &variadic_entry ||
      helper_payload->source_variadic_helper_operand_homes !=
          &variadic_entry.helper_operand_homes.front() ||
      !helper_payload->source_variadic_helper_operand_homes->source_va_list.has_value() ||
      !helper_payload->source_variadic_helper_operand_homes->scalar_result.has_value()) {
    return fail("expected scalar va_arg call record to stop on missing prepared access plan");
  }

  variadic_entry.helper_operand_homes.front().scalar_access_plan =
      prepare::PreparedVariadicScalarVaArgAccessPlan{
          .source_class =
              prepare::PreparedVariadicScalarVaArgSourceClass::GpRegisterSaveArea,
          .value_type = bir::TypeKind::I32,
          .value_size_bytes = 4,
          .value_align_bytes = 4,
          .result_home = variadic_entry.helper_operand_homes.front().scalar_result,
          .source_field =
              prepare::PreparedVariadicVaListFieldKind::GpRegisterSaveArea,
          .source_field_offset_bytes = std::size_t{16},
          .source_slot_size_bytes = std::size_t{8},
          .progression_field = prepare::PreparedVariadicVaListFieldKind::GpOffset,
          .progression_field_offset_bytes = std::size_t{0},
          .progression_stride_bytes = std::size_t{8},
          .overflow_source_field =
              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
          .overflow_source_field_offset_bytes = std::size_t{8},
          .overflow_stride_bytes = std::size_t{8},
      };
  const auto selected_gp_helper_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{24},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_arg.i32",
          .wrapper_kind = prepared_call.wrapper_kind,
          .source_call = &prepared_call,
          .source_variadic_entry = &variadic_entry,
          .source_variadic_helper_operand_homes = &variadic_entry.helper_operand_homes.front(),
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaArg,
          .calling_convention = bir::CallingConv::C,
      });
  const auto* selected_gp_payload =
      std::get_if<aarch64_codegen::CallInstructionRecord>(
          &selected_gp_helper_call.payload);
  if (selected_gp_payload == nullptr ||
      selected_gp_helper_call.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      selected_gp_helper_call.opcode !=
          aarch64_codegen::MachineOpcode::VariadicVaArgScalar ||
      !selected_gp_payload->variadic_scalar_va_arg.has_value() ||
      selected_gp_payload->variadic_scalar_va_arg->source_class !=
          prepare::PreparedVariadicScalarVaArgSourceClass::GpRegisterSaveArea ||
      selected_gp_payload->variadic_scalar_va_arg->source_field !=
          prepare::PreparedVariadicVaListFieldKind::GpRegisterSaveArea ||
      selected_gp_payload->variadic_scalar_va_arg->result_home.register_name !=
          std::optional<std::string>{"w0"}) {
    return fail("expected scalar gp va_arg call record to select prepared access plan");
  }

  auto fp_plan =
      *variadic_entry.helper_operand_homes.front().scalar_access_plan;
  fp_plan.source_class =
      prepare::PreparedVariadicScalarVaArgSourceClass::FpRegisterSaveArea;
  fp_plan.value_type = bir::TypeKind::F64;
  fp_plan.value_size_bytes = 8;
  fp_plan.value_align_bytes = 8;
  fp_plan.result_home =
      prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{52},
          .function_name = c4c::FunctionNameId{9},
          .value_name = c4c::ValueNameId{12},
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"d0"},
      };
  fp_plan.source_field =
      prepare::PreparedVariadicVaListFieldKind::FpRegisterSaveArea;
  fp_plan.source_field_offset_bytes = std::size_t{24};
  fp_plan.source_slot_size_bytes = std::size_t{16};
  fp_plan.progression_field =
      prepare::PreparedVariadicVaListFieldKind::FpOffset;
  fp_plan.progression_field_offset_bytes = std::size_t{4};
  fp_plan.progression_stride_bytes = std::size_t{16};
  variadic_entry.helper_operand_homes.front().scalar_access_plan = fp_plan;
  const auto selected_fp_helper_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{26},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_arg.f64",
          .wrapper_kind = prepared_call.wrapper_kind,
          .source_call = &prepared_call,
          .source_variadic_entry = &variadic_entry,
          .source_variadic_helper_operand_homes = &variadic_entry.helper_operand_homes.front(),
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaArg,
          .calling_convention = bir::CallingConv::C,
      });
  const auto* selected_fp_payload =
      std::get_if<aarch64_codegen::CallInstructionRecord>(
          &selected_fp_helper_call.payload);
  if (selected_fp_payload == nullptr ||
      selected_fp_helper_call.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      selected_fp_helper_call.opcode !=
          aarch64_codegen::MachineOpcode::VariadicVaArgScalar ||
      !selected_fp_payload->variadic_scalar_va_arg.has_value() ||
      selected_fp_payload->variadic_scalar_va_arg->source_class !=
          prepare::PreparedVariadicScalarVaArgSourceClass::FpRegisterSaveArea ||
      selected_fp_payload->variadic_scalar_va_arg->source_field !=
          prepare::PreparedVariadicVaListFieldKind::FpRegisterSaveArea ||
      selected_fp_payload->variadic_scalar_va_arg->progression_field !=
          prepare::PreparedVariadicVaListFieldKind::FpOffset ||
      selected_fp_payload->variadic_scalar_va_arg->source_slot_size_bytes != 16 ||
      selected_fp_payload->variadic_scalar_va_arg->result_home.register_name !=
          std::optional<std::string>{"d0"}) {
    return fail("expected scalar fp va_arg call record to select prepared access plan");
  }

  auto overflow_plan =
      *variadic_entry.helper_operand_homes.front().scalar_access_plan;
  overflow_plan.source_class =
      prepare::PreparedVariadicScalarVaArgSourceClass::OverflowArgArea;
  overflow_plan.source_field =
      prepare::PreparedVariadicVaListFieldKind::OverflowArgArea;
  overflow_plan.progression_field =
      prepare::PreparedVariadicVaListFieldKind::OverflowArgArea;
  overflow_plan.source_field_offset_bytes = std::size_t{8};
  overflow_plan.progression_field_offset_bytes = std::size_t{8};
  variadic_entry.helper_operand_homes.front().scalar_access_plan = overflow_plan;
  const auto selected_overflow_helper_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{25},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_arg.i64",
          .wrapper_kind = prepared_call.wrapper_kind,
          .source_call = &prepared_call,
          .source_variadic_entry = &variadic_entry,
          .source_variadic_helper_operand_homes = &variadic_entry.helper_operand_homes.front(),
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaArg,
          .calling_convention = bir::CallingConv::C,
      });
  const auto* selected_overflow_payload =
      std::get_if<aarch64_codegen::CallInstructionRecord>(
          &selected_overflow_helper_call.payload);
  if (selected_overflow_payload == nullptr ||
      selected_overflow_helper_call.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      !selected_overflow_payload->variadic_scalar_va_arg.has_value() ||
      selected_overflow_payload->variadic_scalar_va_arg->source_class !=
          prepare::PreparedVariadicScalarVaArgSourceClass::OverflowArgArea ||
      selected_overflow_payload->variadic_scalar_va_arg->progression_field !=
          prepare::PreparedVariadicVaListFieldKind::OverflowArgArea) {
    return fail("expected scalar overflow va_arg call record to select prepared access plan");
  }

  return 0;
}

int aggregate_va_arg_call_record_requires_prepared_access_plan() {
  const prepare::PreparedCallPlan prepared_call{
      .block_index = 0,
      .instruction_index = 5,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"llvm.va_arg.aggregate"},
  };
  const prepare::PreparedVariadicEntryHelperOperandHomes va_arg_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaArgAggregate,
      .block_index = 0,
      .instruction_index = 5,
      .source_va_list =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{53},
              .function_name = c4c::FunctionNameId{9},
              .value_name = c4c::ValueNameId{13},
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x3"},
          },
      .aggregate_destination_payload =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{54},
              .function_name = c4c::FunctionNameId{9},
              .value_name = c4c::ValueNameId{14},
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = prepare::PreparedFrameSlotId{9},
              .offset_bytes = std::size_t{32},
          },
  };
  prepare::PreparedVariadicEntryPlanFunction variadic_entry{
      .function_name = c4c::FunctionNameId{9},
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
                          .kind =
                              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
                          .offset_bytes = 8,
                          .size_bytes = 8,
                      },
                  },
          },
      .helper_resources =
          prepare::PreparedVariadicEntryHelperResources{
              .required_helpers =
                  {prepare::PreparedVariadicEntryHelperKind::VaArgAggregate},
              .scratch_register_count = std::size_t{2},
              .scratch_stack_bytes = std::size_t{0},
          },
      .helper_operand_homes = {va_arg_homes},
  };

  const auto missing_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{27},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_arg.aggregate",
          .wrapper_kind = prepared_call.wrapper_kind,
          .source_call = &prepared_call,
          .source_variadic_entry = &variadic_entry,
          .source_variadic_helper_operand_homes = &variadic_entry.helper_operand_homes.front(),
          .variadic_entry_helper =
              prepare::PreparedVariadicEntryHelperKind::VaArgAggregate,
          .calling_convention = bir::CallingConv::C,
      });
  const auto* missing_payload =
      std::get_if<aarch64_codegen::CallInstructionRecord>(&missing_call.payload);
  if (missing_payload == nullptr ||
      missing_call.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::MissingRequiredFacts ||
      missing_call.selection.diagnostic.find(
          "helper_operand_homes.va_arg_aggregate.aggregate_access_plan") ==
          std::string::npos ||
      !missing_payload->source_variadic_helper_operand_homes
           ->aggregate_destination_payload.has_value()) {
    return fail("expected aggregate va_arg call record to stop on missing access plan");
  }

  variadic_entry.helper_operand_homes.front().aggregate_access_plan =
      prepare::PreparedVariadicAggregateVaArgAccessPlan{
          .source_class =
              prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea,
          .payload_size_bytes = 24,
          .payload_align_bytes = 8,
          .destination_payload_home =
              variadic_entry.helper_operand_homes.front()
                  .aggregate_destination_payload,
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
      };
  const auto selected_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{27},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_arg.aggregate",
          .wrapper_kind = prepared_call.wrapper_kind,
          .source_call = &prepared_call,
          .source_variadic_entry = &variadic_entry,
          .source_variadic_helper_operand_homes = &variadic_entry.helper_operand_homes.front(),
          .variadic_entry_helper =
              prepare::PreparedVariadicEntryHelperKind::VaArgAggregate,
          .calling_convention = bir::CallingConv::C,
      });
  const auto* selected_payload =
      std::get_if<aarch64_codegen::CallInstructionRecord>(&selected_call.payload);
  if (selected_payload == nullptr ||
      selected_call.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      selected_call.opcode !=
          aarch64_codegen::MachineOpcode::VariadicVaArgAggregate ||
      !selected_payload->variadic_aggregate_va_arg.has_value() ||
      selected_payload->variadic_aggregate_va_arg->source_class !=
          prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea ||
      selected_payload->variadic_aggregate_va_arg->source_va_list.register_name !=
          std::optional<std::string>{"x3"} ||
      selected_payload->variadic_aggregate_va_arg->destination_payload_home.slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{prepare::PreparedFrameSlotId{9}} ||
      selected_payload->variadic_aggregate_va_arg->copy_size_bytes != 24 ||
      selected_payload->variadic_aggregate_va_arg->progression_stride_bytes != 24 ||
      selected_payload->variadic_aggregate_va_arg->overflow_area_base_slot_id !=
          prepare::PreparedFrameSlotId{6}) {
    return fail("expected aggregate va_arg call record to select prepared access plan");
  }

  return 0;
}

int va_copy_call_record_selects_prepared_layout_field_copies() {
  const prepare::PreparedCallPlan prepared_call{
      .block_index = 0,
      .instruction_index = 6,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"llvm.va_copy.p0.p0"},
  };
  const prepare::PreparedVariadicEntryHelperOperandHomes va_copy_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaCopy,
      .block_index = 0,
      .instruction_index = 6,
      .destination_va_list =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{55},
              .function_name = c4c::FunctionNameId{9},
              .value_name = c4c::ValueNameId{15},
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = prepare::PreparedFrameSlotId{10},
              .offset_bytes = std::size_t{64},
          },
      .source_va_list =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{56},
              .function_name = c4c::FunctionNameId{9},
              .value_name = c4c::ValueNameId{16},
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x4"},
          },
  };
  prepare::PreparedVariadicEntryPlanFunction variadic_entry{
      .function_name = c4c::FunctionNameId{9},
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
              .required_helpers = {prepare::PreparedVariadicEntryHelperKind::VaCopy},
              .scratch_register_count = std::size_t{1},
              .scratch_stack_bytes = std::size_t{0},
          },
      .helper_operand_homes = {va_copy_homes},
  };

  const auto selected_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{28},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_copy.p0.p0",
          .wrapper_kind = prepared_call.wrapper_kind,
          .source_call = &prepared_call,
          .source_variadic_entry = &variadic_entry,
          .source_variadic_helper_operand_homes = &variadic_entry.helper_operand_homes.front(),
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaCopy,
          .calling_convention = bir::CallingConv::C,
      });
  const auto* selected_payload =
      std::get_if<aarch64_codegen::CallInstructionRecord>(&selected_call.payload);
  if (selected_payload == nullptr ||
      selected_call.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      selected_call.opcode != aarch64_codegen::MachineOpcode::VariadicVaCopy ||
      !selected_payload->variadic_va_copy.has_value() ||
      selected_payload->variadic_va_copy->destination_va_list.slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{prepare::PreparedFrameSlotId{10}} ||
      selected_payload->variadic_va_copy->source_va_list.register_name !=
          std::optional<std::string>{"x4"} ||
      selected_payload->variadic_va_copy->va_list_size_bytes != 32 ||
      selected_payload->variadic_va_copy->field_copies.size() != 2 ||
      selected_payload->variadic_va_copy->field_copies[1].kind !=
          prepare::PreparedVariadicVaListFieldKind::OverflowArgArea ||
      selected_payload->variadic_va_copy->field_copies[1].source_offset_bytes != 8 ||
      selected_payload->variadic_va_copy->field_copies[1].destination_offset_bytes != 8 ||
      selected_payload->variadic_va_copy->scratch_register_count != 1) {
    return fail("expected va_copy call record to select prepared layout field copies");
  }

  variadic_entry.va_list_layout.fields.front().size_bytes = 0;
  const auto missing_layout_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{28},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_copy.p0.p0",
          .wrapper_kind = prepared_call.wrapper_kind,
          .source_call = &prepared_call,
          .source_variadic_entry = &variadic_entry,
          .source_variadic_helper_operand_homes = &variadic_entry.helper_operand_homes.front(),
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaCopy,
          .calling_convention = bir::CallingConv::C,
      });
  if (missing_layout_call.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::MissingRequiredFacts ||
      missing_layout_call.selection.diagnostic.find("va_list_layout field facts") ==
          std::string::npos) {
    return fail("expected va_copy call record to fail closed on incomplete layout facts");
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

int f128_call_boundary_move_record_selects_structured_q_register_subset() {
  const prepare::PreparedMoveBundle bundle{
      .function_name = c4c::FunctionNameId{12},
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = 4,
      .instruction_index = 8,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = prepare::PreparedValueId{81},
                  .to_value_id = prepare::PreparedValueId{81},
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{0},
                  .destination_register_name = std::string{"q0"},
                  .destination_contiguous_width = 1,
                  .destination_occupied_register_names = {"q0"},
                  .block_index = 4,
                  .instruction_index = 8,
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .reason = "f128 before-call arg move",
              },
          },
  };
  const auto value_name = c4c::ValueNameId{19};
  const auto carrier = f128_full_width_register_carrier(
      bundle.function_name, prepare::PreparedValueId{81}, value_name, "q2");
  const auto move = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = bundle.function_name,
          .phase = bundle.phase,
          .block_index = bundle.block_index,
          .instruction_index = bundle.instruction_index,
          .move = bundle.moves.front(),
          .source_register =
              aarch64_codegen::RegisterOperand{
                  .reg = aarch64_abi::q_register(2),
                  .role = aarch64_codegen::RegisterOperandRole::CallAbi,
                  .value_id = prepare::PreparedValueId{81},
                  .value_name = value_name,
                  .prepared_class = prepare::PreparedRegisterClass::Vector,
                  .prepared_bank = prepare::PreparedRegisterBank::Vreg,
                  .expected_view = aarch64_abi::RegisterView::Q,
                  .contiguous_width = 1,
                  .occupied_registers = {"q2"},
              },
          .destination_register =
              aarch64_codegen::RegisterOperand{
                  .reg = aarch64_abi::q_register(0),
                  .role = aarch64_codegen::RegisterOperandRole::CallAbi,
                  .value_id = prepare::PreparedValueId{81},
                  .value_name = value_name,
                  .prepared_class = prepare::PreparedRegisterClass::Vector,
                  .prepared_bank = prepare::PreparedRegisterBank::Vreg,
                  .expected_view = aarch64_abi::RegisterView::Q,
                  .contiguous_width = 1,
                  .occupied_registers = {"q0"},
              },
          .source_f128_carrier = &carrier,
          .source_bundle = &bundle,
          .source_move = &bundle.moves.front(),
      });
  const auto* payload =
      std::get_if<aarch64_codegen::CallBoundaryMoveInstructionRecord>(&move.payload);

  if (payload == nullptr ||
      move.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      move.operands.size() != 2 || move.uses.size() != 1 || move.defs.size() != 1 ||
      move.uses.front().reg != aarch64_abi::q_register(2) ||
      move.defs.front().reg != aarch64_abi::q_register(0) ||
      payload->source_f128_carrier != &carrier ||
      payload->source_register->expected_view != aarch64_abi::RegisterView::Q ||
      payload->destination_register->expected_view != aarch64_abi::RegisterView::Q ||
      payload->source_f128_carrier->total_size_bytes != 16 ||
      payload->source_f128_carrier->total_align_bytes != 16) {
    return fail("expected f128 call-boundary move record to select structured q registers");
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

prepare::PreparedI128Carrier make_i128_register_pair_carrier(
    c4c::FunctionNameId function_name,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    int low_register) {
  const std::string low = "x" + std::to_string(low_register);
  const std::string high = "x" + std::to_string(low_register + 1);
  return prepare::PreparedI128Carrier{
      .function_name = function_name,
      .value_id = value_id,
      .value_name = value_name,
      .source_type = bir::TypeKind::I128,
      .kind = prepare::PreparedI128CarrierKind::RegisterPair,
      .lane_width_bytes = 8,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
      .register_bank = prepare::PreparedRegisterBank::Gpr,
      .register_class = prepare::PreparedRegisterClass::General,
      .contiguous_width = 2,
      .occupied_register_names = {low, high},
      .register_placement =
          prepare::PreparedRegisterPlacement{
              .bank = prepare::PreparedRegisterBank::Gpr,
              .pool = prepare::PreparedRegisterSlotPool::CallerSaved,
              .slot_index = static_cast<std::size_t>(low_register),
              .contiguous_width = 2,
          },
      .low_lane =
          prepare::PreparedI128LaneCarrier{
              .role = prepare::PreparedI128LaneRole::Low,
              .lane_index = 0,
              .width_bytes = 8,
              .register_name = low,
          },
      .high_lane =
          prepare::PreparedI128LaneCarrier{
              .role = prepare::PreparedI128LaneRole::High,
              .lane_index = 1,
              .width_bytes = 8,
              .register_name = high,
          },
  };
}

prepare::PreparedStoragePlanValue make_gpr_storage(prepare::PreparedValueId value_id,
                                                   c4c::ValueNameId value_name,
                                                   const char* register_name) {
  return prepare::PreparedStoragePlanValue{
      .value_id = value_id,
      .value_name = value_name,
      .encoding = prepare::PreparedStorageEncodingKind::Register,
      .bank = prepare::PreparedRegisterBank::Gpr,
      .contiguous_width = 1,
      .register_name = register_name,
      .occupied_register_names = {register_name},
  };
}

prepare::PreparedI128RuntimeHelper::LaneBinding make_i128_helper_lane(
    const prepare::PreparedI128Carrier& carrier,
    const prepare::PreparedI128LaneCarrier& lane) {
  return prepare::PreparedI128RuntimeHelper::LaneBinding{
      .value_id = carrier.value_id,
      .value_name = carrier.value_name,
      .carrier_kind = carrier.kind,
      .role = lane.role,
      .lane_index = lane.lane_index,
      .width_bytes = lane.width_bytes,
      .register_name = lane.register_name,
      .slot_id = lane.slot_id,
      .stack_offset_bytes = lane.stack_offset_bytes,
  };
}

prepare::PreparedI128RuntimeHelper::AbiRegisterBinding make_i128_helper_abi_binding(
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    prepare::PreparedI128LaneRole role,
    std::size_t lane_index,
    std::optional<std::size_t> argument_index,
    std::size_t abi_index,
    std::string register_name,
    prepare::PreparedRegisterSlotPool pool) {
  const std::vector<std::string> occupied_register_names{register_name};
  return prepare::PreparedI128RuntimeHelper::AbiRegisterBinding{
      .value_id = value_id,
      .value_name = value_name,
      .role = role,
      .lane_index = lane_index,
      .width_bytes = 8,
      .helper_argument_index = argument_index,
      .abi_register_index = abi_index,
      .register_bank = prepare::PreparedRegisterBank::Gpr,
      .register_class = prepare::PreparedRegisterClass::General,
      .register_name = std::move(register_name),
      .contiguous_width = 1,
      .occupied_register_names = occupied_register_names,
      .register_placement =
          prepare::PreparedRegisterPlacement{
              .bank = prepare::PreparedRegisterBank::Gpr,
              .pool = pool,
              .slot_index = abi_index,
              .contiguous_width = 1,
          },
  };
}

prepare::PreparedI128RuntimeHelper::MarshalingMove make_i128_helper_marshaling_move(
    const prepare::PreparedI128RuntimeHelper::LaneBinding& lane,
    const prepare::PreparedI128RuntimeHelper::AbiRegisterBinding& binding,
    prepare::PreparedI128RuntimeHelperMarshalDirection direction) {
  return prepare::PreparedI128RuntimeHelper::MarshalingMove{
      .direction = direction,
      .phase =
          direction ==
                  prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument
              ? prepare::PreparedMovePhase::BeforeCall
              : prepare::PreparedMovePhase::AfterCall,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .carrier_lane = lane,
      .abi_register = binding,
  };
}

prepare::PreparedI128RuntimeHelper make_i128_runtime_helper(
    c4c::FunctionNameId function_name,
    std::size_t instruction_index,
    bir::BinaryOpcode opcode,
    prepare::PreparedI128RuntimeHelperKind kind,
    std::string callee,
    const prepare::PreparedI128Carrier& result,
    const prepare::PreparedI128Carrier& lhs,
    const prepare::PreparedI128Carrier& rhs) {
  auto helper = prepare::PreparedI128RuntimeHelper{
      .function_name = function_name,
      .block_index = 0,
      .instruction_index = instruction_index,
      .source_binary_opcode = opcode,
      .source_type = bir::TypeKind::I128,
      .result_type = bir::TypeKind::I128,
      .result_value_id = result.value_id,
      .result_value_name = result.value_name,
      .lhs_value_id = lhs.value_id,
      .lhs_value_name = lhs.value_name,
      .rhs_value_id = rhs.value_id,
      .rhs_value_name = rhs.value_name,
      .helper_family = prepare::PreparedI128RuntimeHelperFamily::DivRem,
      .helper_kind = kind,
      .callee_name = std::move(callee),
      .result_ownership =
          prepare::PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes,
      .lhs_low_lane = make_i128_helper_lane(lhs, lhs.low_lane),
      .lhs_high_lane = make_i128_helper_lane(lhs, lhs.high_lane),
      .rhs_low_lane = make_i128_helper_lane(rhs, rhs.low_lane),
      .rhs_high_lane = make_i128_helper_lane(rhs, rhs.high_lane),
      .result_low_lane = make_i128_helper_lane(result, result.low_lane),
      .result_high_lane = make_i128_helper_lane(result, result.high_lane),
      .resource_policy =
          prepare::PreparedI128RuntimeHelper::ResourcePolicy{
              .call_boundary = true,
              .runtime_helper_callee = true,
              .caller_saved_clobbers = true,
              .preserves_source_operation_identity = true,
          },
      .abi_policy =
          prepare::PreparedI128RuntimeHelper::AbiPolicy{
              .transition =
                  prepare::PreparedI128RuntimeHelperAbiTransition::
                      DirectRegisterPairArgumentsAndResult,
              .argument_bank = prepare::PreparedRegisterBank::Gpr,
              .result_bank = prepare::PreparedRegisterBank::Gpr,
              .argument_count = 2,
              .lanes_per_argument = 2,
              .result_lane_count = 2,
              .lane_width_bytes = 8,
          },
      .clobbered_registers =
          {prepare::PreparedClobberedRegister{
              .bank = prepare::PreparedRegisterBank::Gpr,
              .register_name = "x13",
              .contiguous_width = 1,
              .occupied_register_names = {"x13"},
          }},
  };
  helper.lhs_low_abi_argument = make_i128_helper_abi_binding(
      helper.lhs_value_id, helper.lhs_value_name, prepare::PreparedI128LaneRole::Low, 0,
      std::size_t{0}, 0, "x0", prepare::PreparedRegisterSlotPool::CallArgument);
  helper.lhs_high_abi_argument = make_i128_helper_abi_binding(
      helper.lhs_value_id, helper.lhs_value_name, prepare::PreparedI128LaneRole::High, 1,
      std::size_t{0}, 1, "x1", prepare::PreparedRegisterSlotPool::CallArgument);
  helper.rhs_low_abi_argument = make_i128_helper_abi_binding(
      helper.rhs_value_id, helper.rhs_value_name, prepare::PreparedI128LaneRole::Low, 0,
      std::size_t{1}, 2, "x2", prepare::PreparedRegisterSlotPool::CallArgument);
  helper.rhs_high_abi_argument = make_i128_helper_abi_binding(
      helper.rhs_value_id, helper.rhs_value_name, prepare::PreparedI128LaneRole::High, 1,
      std::size_t{1}, 3, "x3", prepare::PreparedRegisterSlotPool::CallArgument);
  helper.result_low_abi_result = make_i128_helper_abi_binding(
      helper.result_value_id, helper.result_value_name, prepare::PreparedI128LaneRole::Low, 0,
      std::nullopt, 0, "x0", prepare::PreparedRegisterSlotPool::CallResult);
  helper.result_high_abi_result = make_i128_helper_abi_binding(
      helper.result_value_id, helper.result_value_name, prepare::PreparedI128LaneRole::High, 1,
      std::nullopt, 1, "x1", prepare::PreparedRegisterSlotPool::CallResult);
  helper.lhs_low_argument_move = make_i128_helper_marshaling_move(
      *helper.lhs_low_lane, *helper.lhs_low_abi_argument,
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument);
  helper.lhs_high_argument_move = make_i128_helper_marshaling_move(
      *helper.lhs_high_lane, *helper.lhs_high_abi_argument,
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument);
  helper.rhs_low_argument_move = make_i128_helper_marshaling_move(
      *helper.rhs_low_lane, *helper.rhs_low_abi_argument,
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument);
  helper.rhs_high_argument_move = make_i128_helper_marshaling_move(
      *helper.rhs_high_lane, *helper.rhs_high_abi_argument,
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument);
  helper.result_low_unmarshal_move = make_i128_helper_marshaling_move(
      *helper.result_low_lane, *helper.result_low_abi_result,
      prepare::PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane);
  helper.result_high_unmarshal_move = make_i128_helper_marshaling_move(
      *helper.result_high_lane, *helper.result_high_abi_result,
      prepare::PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane);
  helper.live_preservation_policy =
      prepare::PreparedI128RuntimeHelper::LivePreservationPolicy{
          .evaluated = true,
          .caller_saved_clobbers_modeled = true,
          .no_additional_live_preservation_required = true,
          .preserved_values =
              {prepare::PreparedCallPreservedValue{
                  .value_id = lhs.value_id,
                  .value_name = lhs.value_name,
                  .route = prepare::PreparedCallPreservationRoute::StackSlot,
                  .slot_id = prepare::PreparedFrameSlotId{99},
                  .stack_offset_bytes = std::size_t{128},
                  .stack_size_bytes = std::size_t{16},
                  .stack_align_bytes = std::size_t{16},
                  .spill_slot_placement =
                      prepare::PreparedSpillSlotPlacement{
                          .slot_id = prepare::PreparedFrameSlotId{99},
                          .offset_bytes = 128,
                      },
              }},
      };
  helper.selected_call_ownership =
      prepare::PreparedI128RuntimeHelper::SelectedCallOwnershipPolicy{
          .owns_terminal_call = true,
          .has_callee_identity = true,
          .has_resource_policy = true,
          .has_clobber_policy = true,
          .has_abi_bindings = true,
          .has_marshaling = true,
          .has_live_preservation = true,
  };
  return helper;
}

prepare::PreparedF128RuntimeHelper::AbiRegisterBinding make_f128_helper_abi_binding(
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    std::optional<std::size_t> argument_index,
    std::size_t abi_index,
    std::string register_name,
    prepare::PreparedRegisterSlotPool pool) {
  const std::vector<std::string> occupied_register_names{register_name};
  return prepare::PreparedF128RuntimeHelper::AbiRegisterBinding{
      .value_id = value_id,
      .value_name = value_name,
      .helper_argument_index = argument_index,
      .abi_register_index = abi_index,
      .width_bytes = 16,
      .register_bank = prepare::PreparedRegisterBank::Vreg,
      .register_class = prepare::PreparedRegisterClass::Vector,
      .register_name = std::move(register_name),
      .contiguous_width = 1,
      .occupied_register_names = occupied_register_names,
      .register_placement =
          prepare::PreparedRegisterPlacement{
              .bank = prepare::PreparedRegisterBank::Vreg,
              .pool = pool,
              .slot_index = abi_index,
              .contiguous_width = 1,
          },
  };
}

prepare::PreparedF128RuntimeHelper::CarrierBinding make_f128_helper_carrier_binding(
    const prepare::PreparedF128Carrier& carrier) {
  return prepare::PreparedF128RuntimeHelper::CarrierBinding{
      .value_id = carrier.value_id,
      .value_name = carrier.value_name,
      .carrier_kind = carrier.kind,
      .width_bytes = carrier.total_size_bytes,
      .align_bytes = carrier.total_align_bytes,
      .register_bank = carrier.register_bank,
      .register_class = carrier.register_class,
      .register_name = carrier.register_name,
      .slot_id = carrier.slot_id,
      .stack_offset_bytes = carrier.stack_offset_bytes,
  };
}

prepare::PreparedF128RuntimeHelper::MarshalingMove make_f128_helper_marshaling_move(
    const prepare::PreparedF128RuntimeHelper::CarrierBinding& carrier,
    const prepare::PreparedF128RuntimeHelper::AbiRegisterBinding& binding,
    prepare::PreparedF128RuntimeHelperMarshalDirection direction) {
  return prepare::PreparedF128RuntimeHelper::MarshalingMove{
      .direction = direction,
      .carrier = carrier,
      .abi_register = binding,
  };
}

prepare::PreparedF128RuntimeHelper::AbiRegisterBinding make_f128_cmp_result_abi_binding(
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name) {
  return prepare::PreparedF128RuntimeHelper::AbiRegisterBinding{
      .value_id = value_id,
      .value_name = value_name,
      .helper_argument_index = std::nullopt,
      .abi_register_index = 0,
      .width_bytes = 4,
      .register_bank = prepare::PreparedRegisterBank::Gpr,
      .register_class = prepare::PreparedRegisterClass::General,
      .register_name = "w0",
      .contiguous_width = 1,
      .occupied_register_names = {"w0"},
      .register_placement =
          prepare::PreparedRegisterPlacement{
              .bank = prepare::PreparedRegisterBank::Gpr,
              .pool = prepare::PreparedRegisterSlotPool::CallResult,
              .slot_index = 0,
              .contiguous_width = 1,
          },
  };
}

prepare::PreparedF128RuntimeHelper::AbiRegisterBinding make_f128_scalar_fp_abi_binding(
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    bir::TypeKind type,
    std::optional<std::size_t> argument_index,
    std::string register_name,
    prepare::PreparedRegisterSlotPool pool) {
  const std::size_t width = type == bir::TypeKind::F64 ? std::size_t{8} : std::size_t{4};
  return prepare::PreparedF128RuntimeHelper::AbiRegisterBinding{
      .value_id = value_id,
      .value_name = value_name,
      .helper_argument_index = argument_index,
      .abi_register_index = 0,
      .width_bytes = width,
      .register_bank = prepare::PreparedRegisterBank::Fpr,
      .register_class = prepare::PreparedRegisterClass::Float,
      .register_name = std::move(register_name),
      .contiguous_width = 1,
      .occupied_register_names = {type == bir::TypeKind::F64 ? "d0" : "s0"},
      .register_placement =
          prepare::PreparedRegisterPlacement{
              .bank = prepare::PreparedRegisterBank::Fpr,
              .pool = pool,
              .slot_index = 0,
              .contiguous_width = 1,
          },
  };
}

void retarget_f128_helper_as_compare(
    prepare::PreparedF128RuntimeHelper& helper,
    bir::BinaryOpcode opcode,
    prepare::PreparedF128RuntimeHelperKind kind,
    std::string callee) {
  helper.source_binary_opcode = opcode;
  helper.result_type = bir::TypeKind::I32;
  helper.helper_family = prepare::PreparedF128RuntimeHelperFamily::Comparison;
  helper.helper_kind = kind;
  helper.callee_name = std::move(callee);
  helper.result_ownership =
      prepare::PreparedF128RuntimeHelperResultOwnership::ScalarCmpResult;
  helper.result_carrier.reset();
  helper.result_unmarshal_move.reset();
  helper.scalar_result =
      prepare::PreparedF128RuntimeHelper::ScalarResultOwnership{
          .value_id = helper.result_value_id,
          .value_name = helper.result_value_name,
          .type = bir::TypeKind::I32,
          .width_bytes = 4,
          .register_bank = prepare::PreparedRegisterBank::Gpr,
          .home_kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"w9"},
      };
  helper.result_abi_result =
      make_f128_cmp_result_abi_binding(helper.result_value_id, helper.result_value_name);
  helper.scalar_result_unmarshal_move =
      prepare::PreparedF128RuntimeHelper::ScalarMarshalingMove{
          .direction =
              prepare::PreparedF128RuntimeHelperMarshalDirection::AbiCmpResultToScalar,
          .scalar_result = *helper.scalar_result,
          .abi_register = *helper.result_abi_result,
      };
  helper.scalar_cmp_result_consumption =
      prepare::PreparedF128RuntimeHelper::ScalarCmpResultConsumption{
          .cmp_type = bir::TypeKind::I32,
          .bir_result_type = bir::TypeKind::I1,
          .zero_test = prepare::PreparedF128CmpResultZeroTest::EqualZero,
          .consumes_helper_cmp_result = true,
          .owns_bir_i1_result = true,
      };
  helper.abi_policy = prepare::PreparedF128RuntimeHelper::AbiPolicy{
      .transition =
          prepare::PreparedF128RuntimeHelperAbiTransition::
              DirectF128ArgumentsAndCmpResult,
      .argument_bank = prepare::PreparedRegisterBank::Vreg,
      .result_bank = prepare::PreparedRegisterBank::Gpr,
      .argument_count = 2,
      .result_count = 1,
      .width_bytes = 4,
  };
}

void retarget_f128_helper_as_f64_to_f128_cast(
    prepare::PreparedF128RuntimeHelper& helper,
    const prepare::PreparedF128Carrier& result,
    prepare::PreparedValueId scalar_value_id,
    c4c::ValueNameId scalar_value_name) {
  helper.source_cast_opcode = bir::CastOpcode::FPExt;
  helper.source_type = bir::TypeKind::F64;
  helper.result_type = bir::TypeKind::F128;
  helper.operand_value_id = scalar_value_id;
  helper.operand_value_name = scalar_value_name;
  helper.lhs_value_id = result.value_id;
  helper.lhs_value_name = result.value_name;
  helper.rhs_value_id = 0;
  helper.rhs_value_name = c4c::kInvalidValueName;
  helper.helper_family = prepare::PreparedF128RuntimeHelperFamily::Cast;
  helper.helper_kind = prepare::PreparedF128RuntimeHelperKind::F64ToF128;
  helper.callee_name = "__extenddftf2";
  helper.result_ownership =
      prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier;
  helper.lhs_carrier.reset();
  helper.rhs_carrier.reset();
  helper.rhs_abi_argument.reset();
  helper.rhs_argument_move.reset();
  helper.result_carrier = make_f128_helper_carrier_binding(result);
  helper.scalar_operand =
      prepare::PreparedF128RuntimeHelper::ScalarResultOwnership{
          .value_id = scalar_value_id,
          .value_name = scalar_value_name,
          .type = bir::TypeKind::F64,
          .width_bytes = 8,
          .register_bank = prepare::PreparedRegisterBank::Fpr,
          .home_kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"d9"},
      };
  helper.scalar_operand_abi_argument =
      make_f128_scalar_fp_abi_binding(
          scalar_value_id, scalar_value_name, bir::TypeKind::F64, std::size_t{0}, "d0",
          prepare::PreparedRegisterSlotPool::CallArgument);
  helper.scalar_operand_argument_move =
      prepare::PreparedF128RuntimeHelper::ScalarMarshalingMove{
          .direction = prepare::PreparedF128RuntimeHelperMarshalDirection::ScalarToAbiArgument,
          .scalar_result = *helper.scalar_operand,
          .abi_register = *helper.scalar_operand_abi_argument,
      };
  helper.result_abi_result = make_f128_helper_abi_binding(
      helper.result_value_id, helper.result_value_name, std::nullopt, 0, "q0",
      prepare::PreparedRegisterSlotPool::CallResult);
  helper.result_unmarshal_move = make_f128_helper_marshaling_move(
      *helper.result_carrier, *helper.result_abi_result,
      prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier);
  helper.abi_policy = prepare::PreparedF128RuntimeHelper::AbiPolicy{
      .transition =
          prepare::PreparedF128RuntimeHelperAbiTransition::DirectScalarArgumentAndF128Result,
      .argument_bank = prepare::PreparedRegisterBank::Fpr,
      .result_bank = prepare::PreparedRegisterBank::Vreg,
      .argument_count = 1,
      .result_count = 1,
      .width_bytes = 16,
  };
}

void retarget_f128_helper_as_f128_to_f32_cast(
    prepare::PreparedF128RuntimeHelper& helper,
    const prepare::PreparedF128Carrier& source,
    prepare::PreparedValueId scalar_value_id,
    c4c::ValueNameId scalar_value_name) {
  helper.source_cast_opcode = bir::CastOpcode::FPTrunc;
  helper.source_type = bir::TypeKind::F128;
  helper.result_type = bir::TypeKind::F32;
  helper.result_value_id = scalar_value_id;
  helper.result_value_name = scalar_value_name;
  helper.operand_value_id = source.value_id;
  helper.operand_value_name = source.value_name;
  helper.lhs_value_id = source.value_id;
  helper.lhs_value_name = source.value_name;
  helper.rhs_value_id = 0;
  helper.rhs_value_name = c4c::kInvalidValueName;
  helper.helper_family = prepare::PreparedF128RuntimeHelperFamily::Cast;
  helper.helper_kind = prepare::PreparedF128RuntimeHelperKind::F128ToF32;
  helper.callee_name = "__trunctfsf2";
  helper.result_ownership =
      prepare::PreparedF128RuntimeHelperResultOwnership::ScalarValue;
  helper.result_carrier.reset();
  helper.rhs_carrier.reset();
  helper.rhs_abi_argument.reset();
  helper.rhs_argument_move.reset();
  helper.scalar_operand.reset();
  helper.scalar_operand_abi_argument.reset();
  helper.scalar_operand_argument_move.reset();
  helper.lhs_carrier = make_f128_helper_carrier_binding(source);
  helper.lhs_abi_argument = make_f128_helper_abi_binding(
      source.value_id, source.value_name, std::size_t{0}, 0, "q0",
      prepare::PreparedRegisterSlotPool::CallArgument);
  helper.lhs_argument_move = make_f128_helper_marshaling_move(
      *helper.lhs_carrier, *helper.lhs_abi_argument,
      prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument);
  helper.scalar_result =
      prepare::PreparedF128RuntimeHelper::ScalarResultOwnership{
          .value_id = scalar_value_id,
          .value_name = scalar_value_name,
          .type = bir::TypeKind::F32,
          .width_bytes = 4,
          .register_bank = prepare::PreparedRegisterBank::Fpr,
          .home_kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"s9"},
      };
  helper.result_abi_result =
      make_f128_scalar_fp_abi_binding(
          scalar_value_id, scalar_value_name, bir::TypeKind::F32, std::nullopt, "s0",
          prepare::PreparedRegisterSlotPool::CallResult);
  helper.scalar_result_unmarshal_move =
      prepare::PreparedF128RuntimeHelper::ScalarMarshalingMove{
          .direction = prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToScalar,
          .scalar_result = *helper.scalar_result,
          .abi_register = *helper.result_abi_result,
      };
  helper.abi_policy = prepare::PreparedF128RuntimeHelper::AbiPolicy{
      .transition =
          prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentAndScalarResult,
      .argument_bank = prepare::PreparedRegisterBank::Vreg,
      .result_bank = prepare::PreparedRegisterBank::Fpr,
      .argument_count = 1,
      .result_count = 1,
      .width_bytes = 4,
  };
}

prepare::PreparedF128RuntimeHelper make_f128_runtime_helper(
    c4c::FunctionNameId function_name,
    std::size_t instruction_index,
    bir::BinaryOpcode opcode,
    prepare::PreparedF128RuntimeHelperKind kind,
    std::string callee,
    const prepare::PreparedF128Carrier& result,
    const prepare::PreparedF128Carrier& lhs,
    const prepare::PreparedF128Carrier& rhs) {
  auto helper = prepare::PreparedF128RuntimeHelper{
      .function_name = function_name,
      .block_index = 0,
      .instruction_index = instruction_index,
      .source_binary_opcode = opcode,
      .source_type = bir::TypeKind::F128,
      .result_type = bir::TypeKind::F128,
      .result_value_id = result.value_id,
      .result_value_name = result.value_name,
      .lhs_value_id = lhs.value_id,
      .lhs_value_name = lhs.value_name,
      .rhs_value_id = rhs.value_id,
      .rhs_value_name = rhs.value_name,
      .helper_family = prepare::PreparedF128RuntimeHelperFamily::Arithmetic,
      .helper_kind = kind,
      .callee_name = std::move(callee),
      .result_ownership =
          prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier,
      .lhs_carrier = make_f128_helper_carrier_binding(lhs),
      .rhs_carrier = make_f128_helper_carrier_binding(rhs),
      .result_carrier = make_f128_helper_carrier_binding(result),
      .resource_policy =
          prepare::PreparedF128RuntimeHelper::ResourcePolicy{
              .call_boundary = true,
              .runtime_helper_callee = true,
              .caller_saved_clobbers = true,
              .preserves_source_operation_identity = true,
          },
      .abi_policy =
          prepare::PreparedF128RuntimeHelper::AbiPolicy{
              .transition =
                  prepare::PreparedF128RuntimeHelperAbiTransition::
                      DirectF128ArgumentsAndResult,
              .argument_bank = prepare::PreparedRegisterBank::Vreg,
              .result_bank = prepare::PreparedRegisterBank::Vreg,
              .argument_count = 2,
              .result_count = 1,
              .width_bytes = 16,
          },
      .live_preservation_policy =
          prepare::PreparedF128RuntimeHelper::LivePreservationPolicy{
              .evaluated = true,
              .caller_saved_clobbers_modeled = true,
              .no_additional_live_preservation_required = true,
              .preserved_values =
                  {prepare::PreparedCallPreservedValue{
                      .value_id = lhs.value_id,
                      .value_name = lhs.value_name,
                      .route = prepare::PreparedCallPreservationRoute::StackSlot,
                      .slot_id = prepare::PreparedFrameSlotId{101},
                      .stack_offset_bytes = std::size_t{144},
                      .stack_size_bytes = std::size_t{16},
                      .stack_align_bytes = std::size_t{16},
                      .spill_slot_placement =
                          prepare::PreparedSpillSlotPlacement{
                              .slot_id = prepare::PreparedFrameSlotId{101},
                              .offset_bytes = 144,
                          },
                  }},
          },
      .selected_call_ownership =
          prepare::PreparedF128RuntimeHelper::SelectedCallOwnershipPolicy{
              .owns_terminal_call = true,
              .has_callee_identity = true,
              .has_resource_policy = true,
              .has_clobber_policy = true,
              .has_abi_bindings = true,
              .has_marshaling = true,
              .has_live_preservation = true,
          },
      .clobbered_registers =
          {prepare::PreparedClobberedRegister{
              .bank = prepare::PreparedRegisterBank::Vreg,
              .register_name = "q16",
              .contiguous_width = 1,
              .occupied_register_names = {"q16"},
          }},
  };
  helper.lhs_abi_argument = make_f128_helper_abi_binding(
      helper.lhs_value_id, helper.lhs_value_name, std::size_t{0}, 0, "q0",
      prepare::PreparedRegisterSlotPool::CallArgument);
  helper.rhs_abi_argument = make_f128_helper_abi_binding(
      helper.rhs_value_id, helper.rhs_value_name, std::size_t{1}, 1, "q1",
      prepare::PreparedRegisterSlotPool::CallArgument);
  helper.result_abi_result = make_f128_helper_abi_binding(
      helper.result_value_id, helper.result_value_name, std::nullopt, 0, "q0",
      prepare::PreparedRegisterSlotPool::CallResult);
  helper.lhs_argument_move = make_f128_helper_marshaling_move(
      *helper.lhs_carrier, *helper.lhs_abi_argument,
      prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument);
  helper.rhs_argument_move = make_f128_helper_marshaling_move(
      *helper.rhs_carrier, *helper.rhs_abi_argument,
      prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument);
  helper.result_unmarshal_move = make_f128_helper_marshaling_move(
      *helper.result_carrier, *helper.result_abi_result,
      prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier);
  return helper;
}

int i128_transport_records_preserve_prepared_carrier_lanes() {
  prepare::PreparedI128CarrierFunction carriers{
      .function_name = c4c::FunctionNameId{2},
      .carriers =
          {make_i128_register_pair_carrier(c4c::FunctionNameId{2},
                                           prepare::PreparedValueId{60},
                                           c4c::ValueNameId{30},
                                           10)},
  };
  auto prepared = aarch64_codegen::make_prepared_i128_carrier_transport_record(
      carriers,
      c4c::ValueNameId{30},
      aarch64_codegen::I128TransportKind::CarrierSnapshot);
  if (!prepared.record.has_value() ||
      prepared.error != aarch64_codegen::PreparedI128TransportRecordError::None) {
    return fail("expected complete i128 register-pair carrier to select transport record");
  }
  const auto instruction =
      aarch64_codegen::make_i128_transport_instruction(*prepared.record);
  const auto* payload =
      std::get_if<aarch64_codegen::I128TransportRecord>(&instruction.payload);
  if (payload == nullptr ||
      instruction.family != aarch64_codegen::InstructionFamily::I128Transport ||
      aarch64_codegen::instruction_family_name(instruction.family) != "i128_transport" ||
      instruction.opcode != aarch64_codegen::MachineOpcode::I128Transport ||
      aarch64_codegen::machine_opcode_name(instruction.opcode) != "i128_transport" ||
      instruction.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      payload->value_id != prepare::PreparedValueId{60} ||
      payload->carrier_kind != prepare::PreparedI128CarrierKind::RegisterPair ||
      payload->low_lane.role != prepare::PreparedI128LaneRole::Low ||
      payload->high_lane.role != prepare::PreparedI128LaneRole::High ||
      !payload->low_lane.reg.has_value() ||
      !payload->high_lane.reg.has_value() ||
      payload->low_lane.reg->reg != aarch64_abi::x_register(10) ||
      payload->high_lane.reg->reg != aarch64_abi::x_register(11) ||
      payload->total_size_bytes != 16 ||
      payload->total_align_bytes != 16 ||
      instruction.operands.size() != 2 ||
      instruction.defs.size() != 2 ||
      instruction.uses.size() != 2) {
    return fail("expected i128 transport record to preserve low/high register lanes");
  }

  carriers.carriers.front().kind = prepare::PreparedI128CarrierKind::Missing;
  carriers.carriers.front().missing_required_facts = {"missing_pair"};
  const auto incomplete = aarch64_codegen::make_prepared_i128_carrier_transport_record(
      carriers,
      c4c::ValueNameId{30},
      aarch64_codegen::I128TransportKind::CarrierSnapshot);
  if (incomplete.record.has_value() ||
      incomplete.error !=
          aarch64_codegen::PreparedI128TransportRecordError::IncompletePreparedI128Carrier ||
      aarch64_codegen::prepared_i128_transport_record_error_name(incomplete.error) !=
          "incomplete_prepared_i128_carrier") {
    return fail("expected incomplete i128 carrier to fail closed");
  }
  return 0;
}

int f128_transport_records_preserve_full_width_carrier_and_memory_facts() {
  const auto function_name = c4c::FunctionNameId{12};
  const auto value_name = c4c::ValueNameId{120};
  prepare::PreparedF128CarrierFunction carriers{
      .function_name = function_name,
      .carriers =
          {f128_full_width_register_carrier(
              function_name, prepare::PreparedValueId{220}, value_name, "q6")},
  };
  auto prepared = aarch64_codegen::make_prepared_f128_carrier_transport_record(
      carriers,
      value_name,
      aarch64_codegen::F128TransportKind::StoreToMemory,
      f128_frame_slot_memory_operand(function_name, c4c::BlockLabelId{4}, 3));
  if (!prepared.record.has_value() ||
      prepared.error != aarch64_codegen::PreparedF128TransportRecordError::None) {
    return fail("expected complete f128 full-width carrier to select memory transport record");
  }
  const auto instruction =
      aarch64_codegen::make_f128_transport_instruction(*prepared.record);
  const auto* payload =
      std::get_if<aarch64_codegen::F128TransportRecord>(&instruction.payload);
  if (payload == nullptr ||
      instruction.family != aarch64_codegen::InstructionFamily::F128Transport ||
      aarch64_codegen::instruction_family_name(instruction.family) != "f128_transport" ||
      instruction.opcode != aarch64_codegen::MachineOpcode::F128Transport ||
      aarch64_codegen::machine_opcode_name(instruction.opcode) != "f128_transport" ||
      instruction.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      payload->transport_kind != aarch64_codegen::F128TransportKind::StoreToMemory ||
      payload->value_id != prepare::PreparedValueId{220} ||
      payload->carrier_kind != prepare::PreparedF128CarrierKind::FullWidthRegister ||
      payload->register_bank != prepare::PreparedRegisterBank::Vreg ||
      payload->register_class != prepare::PreparedRegisterClass::Vector ||
      payload->occupied_register_names.size() != 1 ||
      payload->occupied_register_names.front() != "q6" ||
      !payload->reg.has_value() ||
      payload->reg->reg != aarch64_abi::q_register(6) ||
      payload->total_size_bytes != 16 ||
      payload->total_align_bytes != 16 ||
      !payload->memory.has_value() ||
      payload->memory->size_bytes != 16 ||
      payload->memory->align_bytes != 16 ||
      instruction.operands.size() != 2 ||
      instruction.defs.size() != 1 ||
      instruction.uses.size() != 1 ||
      instruction.side_effects.size() != 1 ||
      instruction.side_effects.front() != aarch64_codegen::MachineSideEffectKind::MemoryWrite) {
    return fail("expected f128 memory transport record to preserve full-width carrier facts");
  }

  carriers.carriers.front().total_align_bytes = 8;
  carriers.carriers.front().missing_required_facts = {"f128_full_width_align"};
  const auto incomplete = aarch64_codegen::make_prepared_f128_carrier_transport_record(
      carriers,
      value_name,
      aarch64_codegen::F128TransportKind::StoreToMemory,
      f128_frame_slot_memory_operand(function_name, c4c::BlockLabelId{4}, 4));
  if (incomplete.record.has_value() ||
      incomplete.error !=
          aarch64_codegen::PreparedF128TransportRecordError::IncompletePreparedF128Carrier ||
      aarch64_codegen::prepared_f128_transport_record_error_name(incomplete.error) !=
          "incomplete_prepared_f128_carrier") {
    return fail("expected incomplete f128 full-width carrier to fail closed");
  }
  return 0;
}

int i128_runtime_helper_boundary_records_consume_prepared_helper_authority() {
  const auto function_name = c4c::FunctionNameId{9};
  const auto result_name = c4c::ValueNameId{90};
  const auto lhs_name = c4c::ValueNameId{91};
  const auto rhs_name = c4c::ValueNameId{92};
  prepare::PreparedI128CarrierFunction carriers{
      .function_name = function_name,
      .carriers =
          {make_i128_register_pair_carrier(function_name,
                                           prepare::PreparedValueId{190},
                                           result_name,
                                           6),
           make_i128_register_pair_carrier(function_name,
                                           prepare::PreparedValueId{191},
                                           lhs_name,
                                           8),
           make_i128_register_pair_carrier(function_name,
                                           prepare::PreparedValueId{192},
                                           rhs_name,
                                           10)},
  };
  auto helper = make_i128_runtime_helper(function_name,
                                         2,
                                         bir::BinaryOpcode::UDiv,
                                         prepare::PreparedI128RuntimeHelperKind::UnsignedDiv,
                                         "__udivti3",
                                         carriers.carriers[0],
                                         carriers.carriers[1],
                                         carriers.carriers[2]);
  auto prepared =
      aarch64_codegen::make_prepared_i128_runtime_helper_boundary_record(
          carriers, helper);
  if (!prepared.record.has_value() ||
      prepared.error !=
          aarch64_codegen::PreparedI128RuntimeHelperRecordError::None ||
      prepared.record->boundary_kind !=
          aarch64_codegen::I128RuntimeHelperBoundaryKind::UnsignedDiv ||
      prepared.record->helper_kind !=
          prepare::PreparedI128RuntimeHelperKind::UnsignedDiv ||
      prepared.record->callee_name != "__udivti3" ||
      prepared.record->source_binary_opcode != bir::BinaryOpcode::UDiv ||
      prepared.record->source_helper != &helper ||
      prepared.record->result.value_id != prepare::PreparedValueId{190} ||
      prepared.record->lhs.low_lane.reg->reg != aarch64_abi::x_register(8) ||
      prepared.record->rhs.high_lane.reg->reg != aarch64_abi::x_register(11) ||
      !prepared.record->resource_policy.call_boundary ||
      prepared.record->abi_policy.argument_bank != prepare::PreparedRegisterBank::Gpr ||
      !prepared.record->live_preservation_policy.evaluated ||
      !prepared.record->live_preservation_policy
           .no_additional_live_preservation_required ||
      prepared.record->live_preservation_policy.preserved_values.size() != 1 ||
      prepared.record->live_preservation_policy.preserved_values.front().route !=
          prepare::PreparedCallPreservationRoute::StackSlot ||
      !prepared.record->selected_call_ownership.owns_terminal_call ||
      !prepared.record->selected_call_ownership.has_marshaling ||
      !prepared.record->selected_call_ownership.has_live_preservation ||
      prepared.record->clobbered_registers.empty()) {
    return fail("expected i128 helper boundary record to consume prepared helper authority");
  }
  const auto instruction =
      aarch64_codegen::make_i128_runtime_helper_boundary_instruction(*prepared.record);
  const auto* payload =
      std::get_if<aarch64_codegen::I128RuntimeHelperBoundaryRecord>(&instruction.payload);
  if (payload == nullptr ||
      instruction.family != aarch64_codegen::InstructionFamily::CallBoundary ||
      instruction.opcode != aarch64_codegen::MachineOpcode::I128RuntimeHelper ||
      aarch64_codegen::machine_opcode_name(instruction.opcode) !=
          "i128_runtime_helper" ||
      instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      instruction.operands.size() != 6 ||
      instruction.defs.size() != 2 ||
      instruction.uses.size() != 4 ||
      instruction.clobbers.empty() ||
      instruction.side_effects.size() != 1 ||
      instruction.side_effects.front() != aarch64_codegen::MachineSideEffectKind::Call ||
      !payload->selected_call_ownership.owns_terminal_call ||
      payload->result.high_lane.reg->reg != aarch64_abi::x_register(7)) {
    return fail("expected selected i128 helper boundary instruction effects");
  }

  helper.live_preservation_policy.no_additional_live_preservation_required = false;
  helper.selected_call_ownership.owns_terminal_call = false;
  helper.selected_call_ownership.has_live_preservation = false;
  const auto missing_ownership =
      aarch64_codegen::make_prepared_i128_runtime_helper_boundary_record(
          carriers, helper);
  if (missing_ownership.record.has_value() ||
      missing_ownership.error !=
          aarch64_codegen::PreparedI128RuntimeHelperRecordError::
              IncompletePreparedI128RuntimeHelper) {
    return fail("expected i128 helper boundary to fail closed without live preservation");
  }
  helper.live_preservation_policy.no_additional_live_preservation_required = true;
  helper.selected_call_ownership.owns_terminal_call = true;
  helper.selected_call_ownership.has_live_preservation = true;

  helper.clobbered_registers.clear();
  const auto missing =
      aarch64_codegen::make_prepared_i128_runtime_helper_boundary_record(
          carriers, helper);
  if (missing.record.has_value() ||
      missing.error !=
          aarch64_codegen::PreparedI128RuntimeHelperRecordError::MissingClobberPolicy ||
      aarch64_codegen::prepared_i128_runtime_helper_record_error_name(missing.error) !=
          "missing_clobber_policy") {
    return fail("expected i128 helper boundary to fail closed without clobber policy");
  }
  return 0;
}

int f128_runtime_helper_boundary_records_consume_prepared_helper_authority() {
  const auto function_name = c4c::FunctionNameId{10};
  const auto result_name = c4c::ValueNameId{100};
  const auto lhs_name = c4c::ValueNameId{101};
  const auto rhs_name = c4c::ValueNameId{102};
  prepare::PreparedF128CarrierFunction carriers{
      .function_name = function_name,
      .carriers =
          {f128_full_width_register_carrier(
               function_name, prepare::PreparedValueId{200}, result_name, "q4"),
           f128_full_width_register_carrier(
               function_name, prepare::PreparedValueId{201}, lhs_name, "q6"),
           f128_full_width_register_carrier(
               function_name, prepare::PreparedValueId{202}, rhs_name, "q8")},
  };
  auto helper = make_f128_runtime_helper(function_name,
                                         3,
                                         bir::BinaryOpcode::Add,
                                         prepare::PreparedF128RuntimeHelperKind::Add,
                                         "__addtf3",
                                         carriers.carriers[0],
                                         carriers.carriers[1],
                                         carriers.carriers[2]);
  auto prepared =
      aarch64_codegen::make_prepared_f128_runtime_helper_boundary_record(
          carriers, helper);
  if (!prepared.record.has_value() ||
      prepared.error !=
          aarch64_codegen::PreparedF128RuntimeHelperRecordError::None ||
      prepared.record->boundary_kind != aarch64_codegen::F128RuntimeHelperBoundaryKind::Add ||
      aarch64_codegen::f128_runtime_helper_boundary_kind_name(
          prepared.record->boundary_kind) != "add" ||
      prepared.record->helper_kind != prepare::PreparedF128RuntimeHelperKind::Add ||
      prepared.record->callee_name != "__addtf3" ||
      prepared.record->source_binary_opcode != bir::BinaryOpcode::Add ||
      prepared.record->source_helper != &helper ||
      prepared.record->result.value_id != prepare::PreparedValueId{200} ||
      prepared.record->result.carrier_register->reg != aarch64_abi::q_register(4) ||
      prepared.record->result.abi_register->reg != aarch64_abi::q_register(0) ||
      prepared.record->lhs.carrier_register->reg != aarch64_abi::q_register(6) ||
      prepared.record->rhs.abi_register->reg != aarch64_abi::q_register(1) ||
      !prepared.record->resource_policy.call_boundary ||
      prepared.record->abi_policy.argument_bank != prepare::PreparedRegisterBank::Vreg ||
      prepared.record->abi_policy.result_count != 1 ||
      !prepared.record->live_preservation_policy.evaluated ||
      !prepared.record->live_preservation_policy
           .no_additional_live_preservation_required ||
      prepared.record->live_preservation_policy.preserved_values.size() != 1 ||
      prepared.record->live_preservation_policy.preserved_values.front().route !=
          prepare::PreparedCallPreservationRoute::StackSlot ||
      !prepared.record->selected_call_ownership.owns_terminal_call ||
      !prepared.record->selected_call_ownership.has_marshaling ||
      !prepared.record->selected_call_ownership.has_live_preservation ||
      prepared.record->clobbered_registers.empty()) {
    return fail("expected f128 helper boundary record to consume prepared helper authority");
  }
  const auto instruction =
      aarch64_codegen::make_f128_runtime_helper_boundary_instruction(*prepared.record);
  const auto* payload =
      std::get_if<aarch64_codegen::F128RuntimeHelperBoundaryRecord>(
          &instruction.payload);
  if (payload == nullptr ||
      instruction.family != aarch64_codegen::InstructionFamily::CallBoundary ||
      instruction.opcode != aarch64_codegen::MachineOpcode::F128RuntimeHelper ||
      aarch64_codegen::machine_opcode_name(instruction.opcode) !=
          "f128_runtime_helper" ||
      instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      instruction.operands.size() != 6 ||
      instruction.defs.size() != 2 ||
      instruction.uses.size() != 4 ||
      instruction.clobbers.empty() ||
      instruction.side_effects.size() != 1 ||
      instruction.side_effects.front() != aarch64_codegen::MachineSideEffectKind::Call ||
      !payload->selected_call_ownership.owns_terminal_call ||
      payload->result.abi_register->reg != aarch64_abi::q_register(0)) {
    return fail("expected selected f128 helper boundary instruction effects");
  }

  helper.selected_call_ownership.owns_terminal_call = false;
  const auto missing_ownership =
      aarch64_codegen::make_prepared_f128_runtime_helper_boundary_record(
          carriers, helper);
  if (missing_ownership.record.has_value() ||
      missing_ownership.error !=
          aarch64_codegen::PreparedF128RuntimeHelperRecordError::
              IncompletePreparedF128RuntimeHelper) {
    return fail("expected f128 helper boundary to fail closed without selected ownership");
  }
  helper.selected_call_ownership.owns_terminal_call = true;

  helper.clobbered_registers.clear();
  const auto missing =
      aarch64_codegen::make_prepared_f128_runtime_helper_boundary_record(
          carriers, helper);
  if (missing.record.has_value() ||
      missing.error !=
          aarch64_codegen::PreparedF128RuntimeHelperRecordError::MissingClobberPolicy ||
      aarch64_codegen::prepared_f128_runtime_helper_record_error_name(missing.error) !=
          "missing_clobber_policy") {
    return fail("expected f128 helper boundary to fail closed without clobber policy");
  }

  auto sub_helper = make_f128_runtime_helper(function_name,
                                             4,
                                             bir::BinaryOpcode::Sub,
                                             prepare::PreparedF128RuntimeHelperKind::Sub,
                                             "__subtf3",
                                             carriers.carriers[0],
                                             carriers.carriers[1],
                                             carriers.carriers[2]);
  const auto prepared_sub =
      aarch64_codegen::make_prepared_f128_runtime_helper_boundary_record(
          carriers, sub_helper);
  if (!prepared_sub.record.has_value() ||
      prepared_sub.error !=
          aarch64_codegen::PreparedF128RuntimeHelperRecordError::None ||
      prepared_sub.record->boundary_kind !=
          aarch64_codegen::F128RuntimeHelperBoundaryKind::Sub ||
      aarch64_codegen::f128_runtime_helper_boundary_kind_name(
          prepared_sub.record->boundary_kind) != "sub" ||
      prepared_sub.record->helper_kind != prepare::PreparedF128RuntimeHelperKind::Sub ||
      prepared_sub.record->callee_name != "__subtf3" ||
      prepared_sub.record->source_binary_opcode != bir::BinaryOpcode::Sub ||
      prepared_sub.record->source_helper != &sub_helper ||
      !prepared_sub.record->selected_call_ownership.owns_terminal_call) {
    return fail("expected f128 sub helper boundary record to consume prepared authority");
  }
  const auto sub_instruction =
      aarch64_codegen::make_f128_runtime_helper_boundary_instruction(
          *prepared_sub.record);
  const auto* sub_payload =
      std::get_if<aarch64_codegen::F128RuntimeHelperBoundaryRecord>(
          &sub_instruction.payload);
  if (sub_payload == nullptr ||
      sub_instruction.opcode != aarch64_codegen::MachineOpcode::F128RuntimeHelper ||
      sub_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      sub_payload->boundary_kind != aarch64_codegen::F128RuntimeHelperBoundaryKind::Sub ||
      sub_payload->callee_name != "__subtf3") {
    return fail("expected selected f128 sub helper boundary instruction effects");
  }

  auto mul_helper = make_f128_runtime_helper(function_name,
                                             5,
                                             bir::BinaryOpcode::Mul,
                                             prepare::PreparedF128RuntimeHelperKind::Mul,
                                             "__multf3",
                                             carriers.carriers[0],
                                             carriers.carriers[1],
                                             carriers.carriers[2]);
  const auto prepared_mul =
      aarch64_codegen::make_prepared_f128_runtime_helper_boundary_record(
          carriers, mul_helper);
  if (!prepared_mul.record.has_value() ||
      prepared_mul.error !=
          aarch64_codegen::PreparedF128RuntimeHelperRecordError::None ||
      prepared_mul.record->boundary_kind !=
          aarch64_codegen::F128RuntimeHelperBoundaryKind::Mul ||
      aarch64_codegen::f128_runtime_helper_boundary_kind_name(
          prepared_mul.record->boundary_kind) != "mul" ||
      prepared_mul.record->helper_kind != prepare::PreparedF128RuntimeHelperKind::Mul ||
      prepared_mul.record->callee_name != "__multf3" ||
      prepared_mul.record->source_binary_opcode != bir::BinaryOpcode::Mul ||
      prepared_mul.record->source_helper != &mul_helper ||
      !prepared_mul.record->selected_call_ownership.owns_terminal_call) {
    return fail("expected f128 mul helper boundary record to consume prepared authority");
  }
  const auto mul_instruction =
      aarch64_codegen::make_f128_runtime_helper_boundary_instruction(
          *prepared_mul.record);
  const auto* mul_payload =
      std::get_if<aarch64_codegen::F128RuntimeHelperBoundaryRecord>(
          &mul_instruction.payload);
  if (mul_payload == nullptr ||
      mul_instruction.opcode != aarch64_codegen::MachineOpcode::F128RuntimeHelper ||
      mul_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      mul_payload->boundary_kind != aarch64_codegen::F128RuntimeHelperBoundaryKind::Mul ||
      mul_payload->callee_name != "__multf3") {
    return fail("expected selected f128 mul helper boundary instruction effects");
  }

  auto div_helper = make_f128_runtime_helper(function_name,
                                             6,
                                             bir::BinaryOpcode::SDiv,
                                             prepare::PreparedF128RuntimeHelperKind::Div,
                                             "__divtf3",
                                             carriers.carriers[0],
                                             carriers.carriers[1],
                                             carriers.carriers[2]);
  const auto prepared_div =
      aarch64_codegen::make_prepared_f128_runtime_helper_boundary_record(
          carriers, div_helper);
  if (!prepared_div.record.has_value() ||
      prepared_div.error !=
          aarch64_codegen::PreparedF128RuntimeHelperRecordError::None ||
      prepared_div.record->boundary_kind !=
          aarch64_codegen::F128RuntimeHelperBoundaryKind::Div ||
      aarch64_codegen::f128_runtime_helper_boundary_kind_name(
          prepared_div.record->boundary_kind) != "div" ||
      prepared_div.record->helper_kind != prepare::PreparedF128RuntimeHelperKind::Div ||
      prepared_div.record->callee_name != "__divtf3" ||
      prepared_div.record->source_binary_opcode != bir::BinaryOpcode::SDiv ||
      prepared_div.record->source_helper != &div_helper ||
      !prepared_div.record->selected_call_ownership.owns_terminal_call) {
    return fail("expected f128 div helper boundary record to consume prepared authority");
  }
  const auto div_instruction =
      aarch64_codegen::make_f128_runtime_helper_boundary_instruction(
          *prepared_div.record);
  const auto* div_payload =
      std::get_if<aarch64_codegen::F128RuntimeHelperBoundaryRecord>(
          &div_instruction.payload);
  if (div_payload == nullptr ||
      div_instruction.opcode != aarch64_codegen::MachineOpcode::F128RuntimeHelper ||
      div_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      div_payload->boundary_kind != aarch64_codegen::F128RuntimeHelperBoundaryKind::Div ||
      div_payload->callee_name != "__divtf3") {
    return fail("expected selected f128 div helper boundary instruction effects");
  }

  auto eq_helper = make_f128_runtime_helper(function_name,
                                            7,
                                            bir::BinaryOpcode::Add,
                                            prepare::PreparedF128RuntimeHelperKind::Add,
                                            "__addtf3",
                                            carriers.carriers[0],
                                            carriers.carriers[1],
                                            carriers.carriers[2]);
  retarget_f128_helper_as_compare(eq_helper,
                                  bir::BinaryOpcode::Eq,
                                  prepare::PreparedF128RuntimeHelperKind::Eq,
                                  "__eqtf2");
  const auto prepared_eq =
      aarch64_codegen::make_prepared_f128_runtime_helper_boundary_record(
          carriers, eq_helper);
  if (!prepared_eq.record.has_value() ||
      prepared_eq.error !=
          aarch64_codegen::PreparedF128RuntimeHelperRecordError::None ||
      prepared_eq.record->boundary_kind !=
          aarch64_codegen::F128RuntimeHelperBoundaryKind::Eq ||
      aarch64_codegen::f128_runtime_helper_boundary_kind_name(
          prepared_eq.record->boundary_kind) != "eq" ||
      prepared_eq.record->helper_family !=
          prepare::PreparedF128RuntimeHelperFamily::Comparison ||
      prepared_eq.record->helper_kind != prepare::PreparedF128RuntimeHelperKind::Eq ||
      prepared_eq.record->callee_name != "__eqtf2" ||
      prepared_eq.record->result_ownership !=
          prepare::PreparedF128RuntimeHelperResultOwnership::ScalarCmpResult ||
      prepared_eq.record->result_type != bir::TypeKind::I32 ||
      prepared_eq.record->scalar_result.type != bir::TypeKind::I32 ||
      prepared_eq.record->scalar_result.width_bytes != 4 ||
      prepared_eq.record->scalar_result.register_bank !=
          prepare::PreparedRegisterBank::Gpr ||
      prepared_eq.record->scalar_result.abi_register->reg !=
          aarch64_abi::w_register(0) ||
      prepared_eq.record->scalar_result.materialized_i1_register->reg !=
          aarch64_abi::w_register(9) ||
      !prepared_eq.record->scalar_result.cmp_result_consumption.has_value() ||
      prepared_eq.record->scalar_result.cmp_result_consumption->zero_test !=
          prepare::PreparedF128CmpResultZeroTest::EqualZero ||
      !prepared_eq.record->scalar_result.cmp_result_consumption
           ->consumes_helper_cmp_result ||
      !prepared_eq.record->scalar_result.cmp_result_consumption->owns_bir_i1_result ||
      prepared_eq.record->lhs.carrier_register->reg != aarch64_abi::q_register(6) ||
      prepared_eq.record->rhs.carrier_register->reg != aarch64_abi::q_register(8) ||
      !prepared_eq.record->selected_call_ownership.owns_terminal_call) {
    return fail("expected f128 comparison helper boundary to preserve scalar cmp result and sources");
  }
  const auto eq_instruction =
      aarch64_codegen::make_f128_runtime_helper_boundary_instruction(
          *prepared_eq.record);
  const auto* eq_payload =
      std::get_if<aarch64_codegen::F128RuntimeHelperBoundaryRecord>(
          &eq_instruction.payload);
  if (eq_payload == nullptr ||
      eq_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      eq_instruction.defs.size() != 3 ||
      eq_instruction.uses.size() != 5 ||
      eq_payload->scalar_result.abi_register->reg != aarch64_abi::w_register(0) ||
      eq_payload->scalar_result.materialized_i1_register->reg !=
          aarch64_abi::w_register(9) ||
      eq_payload->lhs.source_carrier == nullptr ||
      eq_payload->rhs.source_carrier == nullptr) {
    return fail("expected selected f128 comparison helper effects to own scalar cmp result");
  }

  auto unsigned_helper = eq_helper;
  unsigned_helper.source_binary_opcode = bir::BinaryOpcode::Ult;
  const auto unsupported =
      aarch64_codegen::make_prepared_f128_runtime_helper_boundary_record(
          carriers, unsigned_helper);
  if (unsupported.record.has_value() ||
      unsupported.error !=
          aarch64_codegen::PreparedF128RuntimeHelperRecordError::
              UnsupportedSourceOperation) {
    return fail("expected unmodeled f128 comparison predicate to fail closed");
  }

  const auto scalar_name = c4c::ValueNameId{103};
  auto f64_to_f128 = make_f128_runtime_helper(function_name,
                                              8,
                                              bir::BinaryOpcode::Add,
                                              prepare::PreparedF128RuntimeHelperKind::Add,
                                              "__addtf3",
                                              carriers.carriers[0],
                                              carriers.carriers[1],
                                              carriers.carriers[2]);
  retarget_f128_helper_as_f64_to_f128_cast(
      f64_to_f128, carriers.carriers[0], prepare::PreparedValueId{203}, scalar_name);
  const auto prepared_f64_to_f128 =
      aarch64_codegen::make_prepared_f128_runtime_helper_boundary_record(
          carriers, f64_to_f128);
  if (!prepared_f64_to_f128.record.has_value() ||
      prepared_f64_to_f128.record->boundary_kind !=
          aarch64_codegen::F128RuntimeHelperBoundaryKind::F64ToF128 ||
      aarch64_codegen::f128_runtime_helper_boundary_kind_name(
          prepared_f64_to_f128.record->boundary_kind) != "f64_to_f128" ||
      prepared_f64_to_f128.record->source_cast_opcode != bir::CastOpcode::FPExt ||
      prepared_f64_to_f128.record->helper_family !=
          prepare::PreparedF128RuntimeHelperFamily::Cast ||
      prepared_f64_to_f128.record->helper_kind !=
          prepare::PreparedF128RuntimeHelperKind::F64ToF128 ||
      prepared_f64_to_f128.record->callee_name != "__extenddftf2" ||
      prepared_f64_to_f128.record->scalar_operand.abi_register->reg !=
          aarch64_abi::d_register(0) ||
      prepared_f64_to_f128.record->scalar_operand.materialized_i1_register->reg !=
          aarch64_abi::d_register(9) ||
      prepared_f64_to_f128.record->result.abi_register->reg !=
          aarch64_abi::q_register(0) ||
      prepared_f64_to_f128.record->abi_policy.transition !=
          prepare::PreparedF128RuntimeHelperAbiTransition::DirectScalarArgumentAndF128Result) {
    return fail("expected f64 to f128 cast helper boundary to consume unary prepared authority");
  }
  const auto f64_to_f128_instruction =
      aarch64_codegen::make_f128_runtime_helper_boundary_instruction(
          *prepared_f64_to_f128.record);
  if (f64_to_f128_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      f64_to_f128_instruction.defs.size() != 2 ||
      f64_to_f128_instruction.uses.size() != 2) {
    return fail("expected selected f64 to f128 helper instruction effects");
  }

  auto f128_to_f32 = make_f128_runtime_helper(function_name,
                                              9,
                                              bir::BinaryOpcode::Add,
                                              prepare::PreparedF128RuntimeHelperKind::Add,
                                              "__addtf3",
                                              carriers.carriers[0],
                                              carriers.carriers[1],
                                              carriers.carriers[2]);
  retarget_f128_helper_as_f128_to_f32_cast(
      f128_to_f32, carriers.carriers[1], prepare::PreparedValueId{204}, c4c::ValueNameId{104});
  const auto prepared_f128_to_f32 =
      aarch64_codegen::make_prepared_f128_runtime_helper_boundary_record(
          carriers, f128_to_f32);
  if (!prepared_f128_to_f32.record.has_value() ||
      prepared_f128_to_f32.record->boundary_kind !=
          aarch64_codegen::F128RuntimeHelperBoundaryKind::F128ToF32 ||
      prepared_f128_to_f32.record->source_cast_opcode != bir::CastOpcode::FPTrunc ||
      prepared_f128_to_f32.record->helper_kind !=
          prepare::PreparedF128RuntimeHelperKind::F128ToF32 ||
      prepared_f128_to_f32.record->callee_name != "__trunctfsf2" ||
      prepared_f128_to_f32.record->result_ownership !=
          prepare::PreparedF128RuntimeHelperResultOwnership::ScalarValue ||
      prepared_f128_to_f32.record->lhs.abi_register->reg != aarch64_abi::q_register(0) ||
      prepared_f128_to_f32.record->scalar_result.abi_register->reg !=
          aarch64_abi::s_register(0) ||
      prepared_f128_to_f32.record->scalar_result.materialized_i1_register->reg !=
          aarch64_abi::s_register(9) ||
      prepared_f128_to_f32.record->abi_policy.transition !=
          prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentAndScalarResult) {
    return fail("expected f128 to f32 cast helper boundary to consume unary prepared authority");
  }
  const auto f128_to_f32_instruction =
      aarch64_codegen::make_f128_runtime_helper_boundary_instruction(
          *prepared_f128_to_f32.record);
  if (f128_to_f32_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      f128_to_f32_instruction.defs.size() != 2 ||
      f128_to_f32_instruction.uses.size() != 2) {
    return fail("expected selected f128 to f32 helper instruction effects");
  }

  auto mismatched_cast = f64_to_f128;
  mismatched_cast.helper_kind = prepare::PreparedF128RuntimeHelperKind::F128ToF64;
  mismatched_cast.callee_name = "__trunctfdf2";
  const auto mismatched =
      aarch64_codegen::make_prepared_f128_runtime_helper_boundary_record(
          carriers, mismatched_cast);
  if (mismatched.record.has_value() ||
      mismatched.error !=
          aarch64_codegen::PreparedF128RuntimeHelperRecordError::
              UnsupportedSourceOperation) {
    return fail("expected mismatched f128 cast helper identity to fail closed");
  }

  auto missing_cast_facts = f64_to_f128;
  missing_cast_facts.missing_required_facts.push_back("cast_operand_missing_scalar_storage");
  const auto incomplete_cast =
      aarch64_codegen::make_prepared_f128_runtime_helper_boundary_record(
          carriers, missing_cast_facts);
  if (incomplete_cast.record.has_value() ||
      incomplete_cast.error !=
          aarch64_codegen::PreparedF128RuntimeHelperRecordError::
              IncompletePreparedF128RuntimeHelper) {
    return fail("expected incomplete f128 cast helper facts to fail closed");
  }
  return 0;
}

int i128_pair_records_preserve_sources_result_and_lane_semantics() {
  prepare::PreparedNameTables names;
  const auto result_name = names.value_names.intern("%wide.sum");
  const auto lhs_name = names.value_names.intern("%lhs");
  const auto rhs_name = names.value_names.intern("%rhs");
  prepare::PreparedI128CarrierFunction carriers{
      .function_name = c4c::FunctionNameId{7},
      .carriers =
          {make_i128_register_pair_carrier(c4c::FunctionNameId{7},
                                           prepare::PreparedValueId{70},
                                           result_name,
                                           8),
           make_i128_register_pair_carrier(c4c::FunctionNameId{7},
                                           prepare::PreparedValueId{71},
                                           lhs_name,
                                           10),
           make_i128_register_pair_carrier(c4c::FunctionNameId{7},
                                           prepare::PreparedValueId{72},
                                           rhs_name,
                                           12)},
  };
  const bir::BinaryInst add{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I128, "%wide.sum"),
      .operand_type = bir::TypeKind::I128,
      .lhs = bir::Value::named(bir::TypeKind::I128, "%lhs"),
      .rhs = bir::Value::named(bir::TypeKind::I128, "%rhs"),
  };
  auto prepared =
      aarch64_codegen::make_prepared_i128_pair_operation_record(names, carriers, add);
  if (!prepared.record.has_value() ||
      prepared.error != aarch64_codegen::PreparedI128PairRecordError::None ||
      prepared.record->operation != aarch64_codegen::I128PairOperationKind::Add ||
      prepared.record->lane_semantics !=
          aarch64_codegen::I128PairLaneSemantics::CarryPropagating) {
    return fail("expected complete i128 add carriers to select pair operation record");
  }
  const bir::BinaryInst sub{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I128, "%wide.sum"),
      .operand_type = bir::TypeKind::I128,
      .lhs = bir::Value::named(bir::TypeKind::I128, "%lhs"),
      .rhs = bir::Value::named(bir::TypeKind::I128, "%rhs"),
  };
  const auto sub_record =
      aarch64_codegen::make_prepared_i128_pair_operation_record(names, carriers, sub);
  if (!sub_record.record.has_value() ||
      sub_record.record->operation != aarch64_codegen::I128PairOperationKind::Sub ||
      sub_record.record->lane_semantics !=
          aarch64_codegen::I128PairLaneSemantics::BorrowPropagating) {
    return fail("expected complete i128 sub carriers to select pair operation record");
  }
  const auto instruction =
      aarch64_codegen::make_i128_pair_operation_instruction(*prepared.record);
  const auto* payload =
      std::get_if<aarch64_codegen::I128PairOperationRecord>(&instruction.payload);
  if (payload == nullptr ||
      instruction.family != aarch64_codegen::InstructionFamily::I128Pair ||
      aarch64_codegen::instruction_family_name(instruction.family) != "i128_pair" ||
      instruction.opcode != aarch64_codegen::MachineOpcode::I128Pair ||
      aarch64_codegen::machine_opcode_name(instruction.opcode) != "i128_pair" ||
      instruction.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      payload->result.value_id != prepare::PreparedValueId{70} ||
      payload->lhs.value_id != prepare::PreparedValueId{71} ||
      payload->rhs.value_id != prepare::PreparedValueId{72} ||
      !payload->result.low_lane.reg.has_value() ||
      !payload->lhs.high_lane.reg.has_value() ||
      !payload->rhs.low_lane.reg.has_value() ||
      payload->result.low_lane.reg->reg != aarch64_abi::x_register(8) ||
      payload->lhs.high_lane.reg->reg != aarch64_abi::x_register(11) ||
      payload->rhs.low_lane.reg->reg != aarch64_abi::x_register(12) ||
      instruction.operands.size() != 6 ||
      instruction.defs.size() != 2 ||
      instruction.uses.size() != 4) {
    return fail("expected i128 pair operation to preserve result/source lane registers");
  }

  const bir::BinaryInst bitwise{
      .opcode = bir::BinaryOpcode::Xor,
      .result = bir::Value::named(bir::TypeKind::I128, "%wide.sum"),
      .operand_type = bir::TypeKind::I128,
      .lhs = bir::Value::named(bir::TypeKind::I128, "%lhs"),
      .rhs = bir::Value::named(bir::TypeKind::I128, "%rhs"),
  };
  const auto bitwise_record =
      aarch64_codegen::make_prepared_i128_pair_operation_record(names, carriers, bitwise);
  if (!bitwise_record.record.has_value() ||
      bitwise_record.record->operation != aarch64_codegen::I128PairOperationKind::Xor ||
      bitwise_record.record->lane_semantics !=
          aarch64_codegen::I128PairLaneSemantics::IndependentBitwise ||
      aarch64_codegen::i128_pair_lane_semantics_name(
          bitwise_record.record->lane_semantics) != "independent_bitwise") {
    return fail("expected i128 bitwise operation to preserve independent lane semantics");
  }

  prepare::PreparedValueLocationFunction value_locations{
      .function_name = c4c::FunctionNameId{7},
      .value_homes =
          {prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{80},
              .function_name = c4c::FunctionNameId{7},
              .value_name = names.value_names.intern("%cmp"),
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"w0"},
          }},
  };
  prepare::PreparedStoragePlanFunction storage_plan{
      .function_name = c4c::FunctionNameId{7},
      .values = {make_gpr_storage(prepare::PreparedValueId{80},
                                  value_locations.value_homes.front().value_name,
                                  "w0")},
  };
  const bir::BinaryInst shift{
      .opcode = bir::BinaryOpcode::AShr,
      .result = bir::Value::named(bir::TypeKind::I128, "%wide.sum"),
      .operand_type = bir::TypeKind::I128,
      .lhs = bir::Value::named(bir::TypeKind::I128, "%lhs"),
      .rhs = bir::Value::immediate_i32(17),
  };
  const auto shift_record = aarch64_codegen::make_prepared_i128_shift_record(
      names, value_locations, storage_plan, carriers, shift);
  if (!shift_record.record.has_value() ||
      shift_record.record->shift_kind != aarch64_codegen::I128ShiftKind::ArithmeticRight ||
      shift_record.record->lane_semantics !=
          aarch64_codegen::I128ShiftLaneSemantics::CrossLaneArithmeticRight ||
      shift_record.record->count_kind != aarch64_codegen::I128ShiftCountKind::Immediate ||
      aarch64_codegen::i128_shift_lane_semantics_name(
          shift_record.record->lane_semantics) != "cross_lane_arithmetic_right") {
    return fail("expected i128 shift record to preserve count and lane semantics");
  }
  const auto shift_instruction =
      aarch64_codegen::make_i128_shift_instruction(*shift_record.record);
  if (shift_instruction.opcode != aarch64_codegen::MachineOpcode::I128Shift ||
      shift_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      shift_instruction.defs.size() != 2 || shift_instruction.uses.size() != 3) {
    return fail("expected selected i128 shift instruction effects");
  }

  const bir::BinaryInst compare{
      .opcode = bir::BinaryOpcode::Slt,
      .result = bir::Value::named(bir::TypeKind::I1, "%cmp"),
      .operand_type = bir::TypeKind::I128,
      .lhs = bir::Value::named(bir::TypeKind::I128, "%lhs"),
      .rhs = bir::Value::named(bir::TypeKind::I128, "%rhs"),
  };
  const auto compare_record = aarch64_codegen::make_prepared_i128_compare_record(
      names, value_locations, storage_plan, carriers, compare);
  if (!compare_record.record.has_value() ||
      compare_record.record->signedness != aarch64_codegen::I128CompareSignedness::Signed ||
      compare_record.record->high_word_semantics !=
          aarch64_codegen::I128CompareHighWordSemantics::SignedHighWordFirst ||
      !compare_record.record->result_register.has_value() ||
      compare_record.record->result_register->reg != aarch64_abi::w_register(0) ||
      aarch64_codegen::i128_compare_high_word_semantics_name(
          compare_record.record->high_word_semantics) != "signed_high_word_first") {
    return fail("expected i128 compare record to preserve signed high-word semantics");
  }
  const auto compare_instruction =
      aarch64_codegen::make_i128_compare_instruction(*compare_record.record);
  if (compare_instruction.opcode != aarch64_codegen::MachineOpcode::I128Compare ||
      compare_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      compare_instruction.defs.size() != 1 || compare_instruction.uses.size() != 4) {
    return fail("expected selected i128 compare instruction effects");
  }

  carriers.carriers.pop_back();
  const auto missing =
      aarch64_codegen::make_prepared_i128_pair_operation_record(names, carriers, add);
  if (missing.record.has_value() ||
      missing.error !=
          aarch64_codegen::PreparedI128PairRecordError::MissingPreparedI128Carrier ||
      aarch64_codegen::prepared_i128_pair_record_error_name(missing.error) !=
          "missing_prepared_i128_carrier") {
    return fail("expected i128 pair operation to fail closed without source carrier");
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
  if (const int status = stack_slot_preserved_value_consumes_prepared_size_alignment_extent();
      status != 0) {
    return status;
  }
  if (const int status = stack_slot_preserved_value_fails_closed_without_prepared_extent();
      status != 0) {
    return status;
  }
  if (const int status = memory_return_call_record_exposes_prepared_frame_slot_storage();
      status != 0) {
    return status;
  }
  if (const int status = variadic_entry_helper_call_records_select_prepared_va_start();
      status != 0) {
    return status;
  }
  if (const int status = scalar_va_arg_call_record_requires_prepared_access_plan();
      status != 0) {
    return status;
  }
  if (const int status = aggregate_va_arg_call_record_requires_prepared_access_plan();
      status != 0) {
    return status;
  }
  if (const int status = va_copy_call_record_selects_prepared_layout_field_copies();
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
  if (const int status = f128_call_boundary_move_record_selects_structured_q_register_subset();
      status != 0) {
    return status;
  }
  if (const int status = machine_node_printer_mnemonics_have_one_supported_spelling_source();
      status != 0) {
    return status;
  }
  if (const int status = i128_transport_records_preserve_prepared_carrier_lanes();
      status != 0) {
    return status;
  }
  if (const int status =
          f128_transport_records_preserve_full_width_carrier_and_memory_facts();
      status != 0) {
    return status;
  }
  if (const int status =
          i128_runtime_helper_boundary_records_consume_prepared_helper_authority();
      status != 0) {
    return status;
  }
  if (const int status =
          f128_runtime_helper_boundary_records_consume_prepared_helper_authority();
      status != 0) {
    return status;
  }
  if (const int status =
          i128_pair_records_preserve_sources_result_and_lane_semantics();
      status != 0) {
    return status;
  }
  return 0;
}
