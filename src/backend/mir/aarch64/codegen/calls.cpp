#include "calls.hpp"
#include "dispatch_diagnostics.hpp"
#include "dispatch.hpp"
#include "effects.hpp"
#include "float_ops.hpp"
#include "f128.hpp"
#include "machine_printer.hpp"
#include "memory.hpp"
#include "variadic.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;

const prepare::PreparedCallPlan* find_prepared_call_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  return prepare::find_indexed_prepared_call_plan(context.function.call_plan_lookups,
                                                  context.function.call_plans,
                                                  context.block_index,
                                                  instruction_index);
}

const prepare::PreparedCallPlan* require_prepared_call_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* call_plan = find_prepared_call_plan(context, instruction_index);
  if (call_plan == nullptr) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingPreparedCallPlan,
        context,
        instruction_index,
        "AArch64 call lowering requires an authoritative PreparedCallPlan");
  }
  return call_plan;
}


namespace {

constexpr std::size_t kStackPointerAlignmentBytes = 16;

[[nodiscard]] std::size_t align_to(std::size_t value, std::size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  const auto remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

[[nodiscard]] std::size_t incoming_stack_argument_size_bytes(
    const bir::CallArgAbiInfo& abi) {
  return align_to(std::max<std::size_t>(abi.size_bytes, 8), 8);
}

[[nodiscard]] std::size_t incoming_stack_argument_alignment_bytes(
    const bir::CallArgAbiInfo& abi) {
  const std::size_t abi_alignment = abi.align_bytes == 0 ? abi.size_bytes : abi.align_bytes;
  return std::min<std::size_t>(std::max<std::size_t>(abi_alignment, 8), 16);
}

[[nodiscard]] bool entry_param_uses_incoming_stack(const bir::Param& param) {
  return param.abi.has_value() && param.abi->passed_on_stack;
}

[[nodiscard]] std::size_t named_incoming_stack_bytes(const bir::Function& function,
                                                     std::size_t named_parameter_count) {
  std::size_t next_offset = 0;
  const std::size_t limit = std::min(named_parameter_count, function.params.size());
  for (std::size_t index = 0; index < limit; ++index) {
    const auto& param = function.params[index];
    if (!entry_param_uses_incoming_stack(param)) {
      continue;
    }
    next_offset =
        align_to(next_offset, incoming_stack_argument_alignment_bytes(*param.abi));
    next_offset += incoming_stack_argument_size_bytes(*param.abi);
  }
  return next_offset;
}

[[nodiscard]] bool function_has_call(const bir::Function& function) {
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (std::holds_alternative<bir::CallInst>(inst)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] std::optional<std::size_t> fixed_frame_adjustment_bytes(
    const module::FunctionLoweringContext& context) {
  const auto* frame = context.frame_plan;
  const auto* function = context.bir_function;
  if (frame == nullptr || function == nullptr ||
      frame->has_dynamic_stack || context.dynamic_stack_plan != nullptr) {
    return std::nullopt;
  }

  std::size_t frame_alignment =
      std::max<std::size_t>(frame->frame_alignment_bytes, kStackPointerAlignmentBytes);
  std::size_t prepared_frame_size = frame->frame_size_bytes;
  for (const auto& saved : frame->saved_callee_registers) {
    if (!saved.slot_placement.has_value() ||
        !saved.slot_placement->stack_offset_bytes.has_value() ||
        !saved.slot_placement->size_bytes.has_value() ||
        !saved.slot_placement->align_bytes.has_value()) {
      return std::nullopt;
    }
    prepared_frame_size = std::max(
        prepared_frame_size,
        *saved.slot_placement->stack_offset_bytes + *saved.slot_placement->size_bytes);
    frame_alignment = std::max(frame_alignment, *saved.slot_placement->align_bytes);
  }

  std::size_t frame_size = align_to(prepared_frame_size, frame_alignment);
  if (function_has_call(*function)) {
    frame_size = align_to(prepared_frame_size + 16, frame_alignment);
  }
  return frame_size == 0 ? std::optional<std::size_t>{}
                         : std::optional<std::size_t>{frame_size};
}

[[nodiscard]] std::optional<std::size_t> va_start_overflow_area_stack_offset(
    const module::BlockLoweringContext& context,
    const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan,
    std::optional<prepare::PreparedVariadicEntryHelperKind> variadic_helper) {
  if (variadic_helper !=
          std::optional<prepare::PreparedVariadicEntryHelperKind>{
              prepare::PreparedVariadicEntryHelperKind::VaStart} ||
      variadic_entry_plan == nullptr || context.function.bir_function == nullptr) {
    return std::nullopt;
  }
  const auto frame_size = fixed_frame_adjustment_bytes(context.function);
  if (!frame_size.has_value()) {
    return std::nullopt;
  }
  return *frame_size + named_incoming_stack_bytes(*context.function.bir_function,
                                                 variadic_entry_plan->named_parameter_count);
}

[[nodiscard]] std::optional<RegisterOperand> make_indirect_callee_register(
    const module::BlockLoweringContext& context,
    const prepare::PreparedIndirectCalleePlan& callee,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (callee.encoding != prepare::PreparedStorageEncodingKind::Register ||
      !callee.register_name.has_value() || callee.bank != prepare::PreparedRegisterBank::Gpr ||
      callee.slot_id.has_value() || callee.stack_offset_bytes.has_value() ||
      callee.immediate_i32.has_value() || callee.pointer_base_value_name.has_value() ||
      callee.pointer_byte_delta.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        "AArch64 indirect call lowering requires an explicit prepared GPR callee register");
    return std::nullopt;
  }

  const auto converted = abi::convert_prepared_register(
      *callee.register_name,
      callee.bank,
      prepare::PreparedRegisterClass::General,
      abi::RegisterView::X);
  if (!converted.reg.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::RegisterConversionFailed,
        context,
        instruction_index,
        converted.error.has_value()
            ? converted.error->message
            : "prepared indirect callee register could not be converted");
    return std::nullopt;
  }

  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::CallAbi,
      .value_id = callee.value_id,
      .value_name = callee.value_name,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = callee.bank,
      .expected_view = abi::RegisterView::X,
      .contiguous_width = 1,
      .occupied_registers = {*callee.register_name},
  };
}

[[nodiscard]] std::optional<MemoryOperand> make_memory_return_storage(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMemoryReturnPlan& memory_return,
    std::size_t instruction_index) {
  if (memory_return.encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      !memory_return.slot_id.has_value() ||
      !memory_return.stack_offset_bytes.has_value()) {
    return std::nullopt;
  }
  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = memory_return.slot_id,
      .byte_offset = static_cast<std::int64_t>(*memory_return.stack_offset_bytes),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = memory_return.size_bytes,
      .align_bytes = memory_return.align_bytes,
      .can_use_base_plus_offset = true,
  };
}

MachineNodeStatusRecord call_boundary_move_selection_status(
    const CallBoundaryMoveInstructionRecord& instruction) {
  if (instruction.source_bundle == nullptr ||
      (instruction.source_move == nullptr && !instruction.source_immediate.has_value() &&
       !instruction.source_memory.has_value()) ||
      instruction.function_name == c4c::kInvalidFunctionName) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "call-boundary move node is missing prepared move provenance"};
  }
  const bool selected_register_argument_move =
      instruction.phase == prepare::PreparedMovePhase::BeforeCall &&
      instruction.move.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      instruction.move.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  const bool selected_register_result_move =
      instruction.phase == prepare::PreparedMovePhase::AfterCall &&
      instruction.move.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallResultAbi &&
      instruction.move.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  const bool selected_register_return_move =
      instruction.phase == prepare::PreparedMovePhase::BeforeReturn &&
      instruction.move.destination_kind ==
          prepare::PreparedMoveDestinationKind::FunctionReturnAbi &&
      instruction.move.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  const bool selected_value_register_move =
      (instruction.phase == prepare::PreparedMovePhase::BlockEntry ||
       instruction.phase == prepare::PreparedMovePhase::BeforeInstruction) &&
      instruction.move.destination_kind == prepare::PreparedMoveDestinationKind::Value &&
      instruction.move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  const bool selected_preservation_home_population =
      instruction.phase == prepare::PreparedMovePhase::BeforeCall &&
      instruction.move.destination_kind == prepare::PreparedMoveDestinationKind::Value &&
      instruction.move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      instruction.move.reason == "callee_saved_preservation_home_population";
  const bool stack_argument_move =
      instruction.phase == prepare::PreparedMovePhase::BeforeCall &&
      instruction.move.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      instruction.move.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::StackSlot &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  if (stack_argument_move) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary stack argument move requires AArch64 stack-copy lowering"};
  }
  if (!selected_register_argument_move && !selected_register_result_move &&
      !selected_register_return_move && !selected_value_register_move &&
      !selected_preservation_home_population) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node is outside the selected register call-boundary move subset"};
  }
  if ((!instruction.source_register.has_value() && !instruction.source_immediate.has_value() &&
       !instruction.source_memory.has_value()) ||
      !instruction.destination_register.has_value()) {
    const bool selected_f128_constant_argument_move =
        selected_register_argument_move &&
        !instruction.source_register.has_value() &&
        instruction.destination_register.has_value() &&
        instruction.destination_register->prepared_bank ==
            prepare::PreparedRegisterBank::Vreg &&
        instruction.destination_register->expected_view == abi::RegisterView::Q &&
        instruction.source_f128_carrier != nullptr &&
        instruction.source_f128_carrier->source_type == bir::TypeKind::F128 &&
        instruction.source_f128_carrier->kind ==
            prepare::PreparedF128CarrierKind::Missing &&
        instruction.source_f128_carrier->missing_required_facts.empty() &&
        instruction.source_f128_carrier->total_size_bytes == 16 &&
        instruction.source_f128_carrier->total_align_bytes == 16 &&
        instruction.source_f128_carrier->constant_payload.has_value() &&
        instruction.source_f128_constant_payload.has_value() &&
        instruction.source_f128_constant_payload->low_bits ==
            instruction.source_f128_carrier->constant_payload->low_bits &&
        instruction.source_f128_constant_payload->high_bits ==
            instruction.source_f128_carrier->constant_payload->high_bits;
    if (selected_f128_constant_argument_move) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared register source and destination"};
  }
  if (instruction.source_immediate.has_value() &&
      (instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Gpr ||
       (instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Fpr &&
        scalar_fp_register_view(instruction.source_immediate->type).has_value() &&
        instruction.destination_register->expected_view ==
            *scalar_fp_register_view(instruction.source_immediate->type)))) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  if (is_aggregate_register_lane_publication(instruction)) {
    if (instruction.source_memory->base_kind == MemoryBaseKind::PointerValue &&
        instruction.source_memory->base_register.has_value()) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    if (instruction.source_memory->base_kind == MemoryBaseKind::FrameSlot &&
        instruction.source_memory->frame_slot_id.has_value() &&
        instruction.source_memory->byte_offset_is_prepared_snapshot &&
        instruction.source_memory->can_use_base_plus_offset) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
  }
  if (instruction.source_memory.has_value() &&
      instruction.source_memory->support == MemoryOperandSupportKind::Prepared &&
      instruction.source_memory->base_kind == MemoryBaseKind::FrameSlot &&
      instruction.source_memory->frame_slot_id.has_value() &&
      instruction.source_memory->byte_offset_is_prepared_snapshot &&
      instruction.source_memory->can_use_base_plus_offset &&
      (instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Gpr ||
       instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Fpr ||
       (instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Vreg &&
        instruction.destination_register->expected_view == abi::RegisterView::Q &&
        instruction.source_memory->size_bytes == 16))) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  if (instruction.source_memory.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared frame-slot source and GPR destination"};
  }
  if (!instruction.source_register.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared register source and destination"};
  }
  if (instruction.source_register->prepared_bank == prepare::PreparedRegisterBank::Gpr &&
      instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Gpr) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  const bool selected_scalar_fpr_register_move =
      instruction.source_register->prepared_bank == prepare::PreparedRegisterBank::Fpr &&
      instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Fpr &&
      (instruction.source_register->expected_view == abi::RegisterView::S ||
       instruction.source_register->expected_view == abi::RegisterView::D) &&
      instruction.source_register->expected_view ==
          instruction.destination_register->expected_view;
  if (selected_scalar_fpr_register_move) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  const auto* f128_carrier =
      instruction.source_f128_carrier != nullptr
          ? instruction.source_f128_carrier
          : instruction.destination_f128_carrier;
  const bool selected_f128_register_move =
      instruction.source_register->prepared_bank == prepare::PreparedRegisterBank::Vreg &&
      instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Vreg &&
      instruction.source_register->expected_view == abi::RegisterView::Q &&
      instruction.destination_register->expected_view == abi::RegisterView::Q &&
      f128_carrier != nullptr &&
      f128_carrier->kind == prepare::PreparedF128CarrierKind::FullWidthRegister &&
      f128_carrier->missing_required_facts.empty() &&
      f128_carrier->total_size_bytes == 16 && f128_carrier->total_align_bytes == 16;
  if (!selected_f128_register_move) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared GPR registers, scalar FPR registers, or structured f128 q-register authority"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord call_boundary_abi_binding_selection_status(
    const CallBoundaryAbiBindingInstructionRecord& instruction) {
  if (instruction.source_bundle == nullptr || instruction.source_binding == nullptr ||
      instruction.function_name == c4c::kInvalidFunctionName) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "call-boundary ABI binding node is missing prepared binding provenance"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord call_selection_status(const CallInstructionRecord& instruction) {
  if (auto variadic_status = variadic_call_selection_status(instruction)) {
    return *variadic_status;
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

}  // namespace

InstructionRecord make_call_boundary_move_instruction(
    CallBoundaryMoveInstructionRecord instruction) {
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  if (instruction.destination_register.has_value()) {
    const auto destination = make_register_operand(*instruction.destination_register);
    operands.push_back(destination);
    defs.push_back(machine_effect_from_operand(destination));
  } else if (instruction.move.to_value_id != 0) {
    defs.push_back(machine_prepared_value_def(instruction.move.to_value_id,
                                              c4c::kInvalidValueName));
  }
  if (instruction.source_register.has_value()) {
    const auto source = make_register_operand(*instruction.source_register);
    operands.push_back(source);
    uses.push_back(machine_effect_from_operand(source));
  } else if (instruction.source_memory.has_value()) {
    const auto source = make_memory_operand(*instruction.source_memory);
    operands.push_back(source);
    if (!instruction.source_memory_materializes_address) {
      uses.push_back(machine_effect_from_operand(source));
    }
  } else if (instruction.source_immediate.has_value()) {
    const auto source = make_immediate_operand(*instruction.source_immediate);
    operands.push_back(source);
    uses.push_back(machine_effect_from_operand(source));
  } else if (instruction.move.from_value_id != 0) {
    uses.push_back(machine_prepared_value_def(instruction.move.from_value_id,
                                              c4c::kInvalidValueName));
  }
  return InstructionRecord{
      .family = InstructionFamily::CallBoundary,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::CallBoundaryMove,
      .selection = call_boundary_move_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_index = instruction.block_index,
      .instruction_index = instruction.instruction_index,
      .operands = operands,
      .defs = defs,
      .uses = uses,
      .payload = instruction,
  };
}

InstructionRecord make_call_boundary_abi_binding_instruction(
    CallBoundaryAbiBindingInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::CallBoundary,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::CallBoundaryAbiBinding,
      .selection = call_boundary_abi_binding_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_index = instruction.block_index,
      .instruction_index = instruction.instruction_index,
      .payload = instruction,
  };
}

InstructionRecord make_call_instruction(CallInstructionRecord instruction) {
  complete_variadic_call_record(instruction);

  std::vector<OperandRecord> operands = instruction.arguments;
  if (instruction.indirect_callee.has_value()) {
    operands.insert(operands.begin(), *instruction.indirect_callee);
  } else if (instruction.direct_callee.has_value()) {
    operands.insert(operands.begin(), make_symbol_operand(*instruction.direct_callee));
  }
  std::vector<MachineEffectResource> defs;
  if (instruction.result.has_value()) {
    defs.push_back(machine_effect_from_operand(*instruction.result));
  }
  std::vector<MachineEffectResource> uses = machine_effects_from_operands(operands);
  std::vector<MachineSideEffectKind> side_effects = {MachineSideEffectKind::Call};
  if (instruction.memory_return_storage.has_value()) {
    defs.push_back(machine_effect_from_operand(
        make_memory_operand(*instruction.memory_return_storage)));
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_va_start.has_value()) {
    defs.push_back(machine_prepared_value_def(
        instruction.variadic_va_start->destination_va_list.value_id,
        instruction.variadic_va_start->destination_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_scalar_va_arg.has_value()) {
    defs.push_back(machine_prepared_value_def(
        instruction.variadic_scalar_va_arg->result_home.value_id,
        instruction.variadic_scalar_va_arg->result_home.value_name));
    uses.push_back(machine_prepared_value_def(
        instruction.variadic_scalar_va_arg->source_va_list.value_id,
        instruction.variadic_scalar_va_arg->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_aggregate_va_arg.has_value()) {
    defs.push_back(machine_prepared_value_def(
        instruction.variadic_aggregate_va_arg->destination_payload_home.value_id,
        instruction.variadic_aggregate_va_arg->destination_payload_home.value_name));
    uses.push_back(machine_prepared_value_def(
        instruction.variadic_aggregate_va_arg->source_va_list.value_id,
        instruction.variadic_aggregate_va_arg->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_va_copy.has_value()) {
    defs.push_back(machine_prepared_value_def(
        instruction.variadic_va_copy->destination_va_list.value_id,
        instruction.variadic_va_copy->destination_va_list.value_name));
    uses.push_back(machine_prepared_value_def(
        instruction.variadic_va_copy->source_va_list.value_id,
        instruction.variadic_va_copy->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  return InstructionRecord{
      .family = InstructionFamily::Call,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = instruction.variadic_va_start.has_value()
                    ? MachineOpcode::VariadicVaStart
                    : (instruction.variadic_scalar_va_arg.has_value()
                           ? MachineOpcode::VariadicVaArgScalar
                           : (instruction.variadic_aggregate_va_arg.has_value()
                                  ? MachineOpcode::VariadicVaArgAggregate
                                  : (instruction.variadic_va_copy.has_value()
                                         ? MachineOpcode::VariadicVaCopy
                                         : (instruction.is_indirect
                                                ? MachineOpcode::IndirectCall
                                                : MachineOpcode::DirectCall)))),
      .selection = call_selection_status(instruction),
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .clobbers = effects_from_prepared_call_clobbers(instruction.clobbered_registers),
      .preserves = prepared_call_preserve_effect_publication_enabled()
                       ? effects_from_prepared_call_preserved_values(
                             instruction.preserved_values)
                       : std::vector<MachineEffectResource>{},
      .side_effects = std::move(side_effects),
      .payload = std::move(instruction),
  };
}


std::optional<module::MachineInstruction> lower_prepared_call_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan,
    const prepare::PreparedVariadicEntryHelperOperandHomes* variadic_helper_operand_homes,
    std::optional<prepare::PreparedVariadicEntryHelperKind> variadic_helper,
    module::ModuleLoweringDiagnostics& diagnostics) {
  CallInstructionRecord call_record{
      .wrapper_kind = call_plan.wrapper_kind,
      .variadic_fpr_arg_register_count = call_plan.variadic_fpr_arg_register_count,
      .memory_return = call_plan.memory_return,
      .memory_return_storage =
          call_plan.memory_return.has_value()
              ? make_memory_return_storage(context,
                                           *call_plan.memory_return,
                                           instruction_index)
              : std::nullopt,
      .prepared_indirect_callee = call_plan.indirect_callee,
      .prepared_arguments = call_plan.arguments,
      .prepared_result = call_plan.result,
      .preserved_values = prepared_call_preserve_effect_publication_enabled()
                              ? call_plan.preserved_values
                              : std::vector<prepare::PreparedCallPreservedValue>{},
      .clobbered_registers = call_plan.clobbered_registers,
      .source_call = &call_plan,
      .outgoing_stack_argument_bytes = outgoing_stack_argument_bytes(call_plan),
      .source_variadic_entry = variadic_entry_plan,
      .source_variadic_helper_operand_homes = variadic_helper_operand_homes,
      .variadic_entry_helper = variadic_helper,
      .variadic_va_start_overflow_area_stack_offset_bytes =
          va_start_overflow_area_stack_offset(context, variadic_entry_plan, variadic_helper),
      .calling_convention = call_inst.calling_convention,
      .is_indirect = call_plan.is_indirect,
      .is_variadic =
          call_plan.wrapper_kind == prepare::PreparedCallWrapperKind::DirectExternVariadic ||
          call_inst.is_variadic,
      .is_noreturn = call_inst.is_noreturn,
  };

  if (call_plan.is_indirect) {
    if (!call_inst.is_indirect || !call_plan.indirect_callee.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 indirect call lowering requires matching retained BIR and prepared indirect callee facts");
      return std::nullopt;
    }
    auto callee = make_indirect_callee_register(
        context, *call_plan.indirect_callee, instruction_index, diagnostics);
    if (!callee.has_value()) {
      return std::nullopt;
    }
    call_record.indirect_callee = make_register_operand(*callee);
  } else {
    if (call_inst.is_indirect || !call_plan.direct_callee_name.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 direct call lowering requires matching retained BIR and prepared direct callee facts");
      return std::nullopt;
    }
    call_record.direct_callee = SymbolOperand{
        .link_name = call_inst.callee_link_name_id,
        .type = bir::TypeKind::Ptr,
        .is_extern = call_plan.wrapper_kind != prepare::PreparedCallWrapperKind::SameModule,
    };
    call_record.direct_callee_label = *call_plan.direct_callee_name;
  }

  InstructionRecord target = make_call_instruction(std::move(call_record));
  target.function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName;
  target.block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;

  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}


}  // namespace c4c::backend::aarch64::codegen
