#include "calls.hpp"
#include "constant_materialization.hpp"
#include "dispatch_diagnostics.hpp"
#include "dispatch.hpp"
#include "dispatch_lookup.hpp"
#include "dispatch_publication_common.hpp"
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

[[nodiscard]] std::size_t outgoing_stack_argument_bytes(
    const prepare::PreparedCallPlan& call_plan) {
  std::size_t bytes = 0;
  for (const auto& argument : call_plan.arguments) {
    if (!argument.destination_stack_offset_bytes.has_value() ||
        !argument.destination_stack_size_bytes.has_value()) {
      continue;
    }
    bytes = std::max(bytes,
                     *argument.destination_stack_offset_bytes +
                         *argument.destination_stack_size_bytes);
  }
  return align_to(bytes, kStackPointerAlignmentBytes);
}

[[nodiscard]] abi::RegisterReference outgoing_stack_argument_base_register() {
  return abi::x_register(16);
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_integer_register_view_from_size(
    std::size_t size_bytes) {
  if (size_bytes > 0 && size_bytes <= 4) {
    return abi::RegisterView::W;
  }
  if (size_bytes == 8) {
    return abi::RegisterView::X;
  }
  return std::nullopt;
}

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

namespace {

struct PreservedCallArgumentSource {
  std::optional<RegisterOperand> source_register;
  std::optional<MemoryOperand> source_memory;
};

[[nodiscard]] prepare::PreparedRegisterClass register_class_from_bank(
    prepare::PreparedRegisterBank bank) {
  switch (bank) {
    case prepare::PreparedRegisterBank::Gpr:
      return prepare::PreparedRegisterClass::General;
    case prepare::PreparedRegisterBank::Fpr:
      return prepare::PreparedRegisterClass::Float;
    case prepare::PreparedRegisterBank::Vreg:
      return prepare::PreparedRegisterClass::Vector;
    case prepare::PreparedRegisterBank::AggregateAddress:
      return prepare::PreparedRegisterClass::AggregateAddress;
    case prepare::PreparedRegisterBank::None:
      return prepare::PreparedRegisterClass::None;
  }
  return prepare::PreparedRegisterClass::None;
}

[[nodiscard]] bool complete_full_width_f128_carrier(
    const prepare::PreparedF128Carrier* carrier) {
  return carrier != nullptr &&
         carrier->kind == prepare::PreparedF128CarrierKind::FullWidthRegister &&
         carrier->missing_required_facts.empty() &&
         carrier->total_size_bytes == 16 && carrier->total_align_bytes == 16 &&
         carrier->register_bank == prepare::PreparedRegisterBank::Vreg &&
         carrier->register_class == prepare::PreparedRegisterClass::Vector &&
         carrier->contiguous_width == 1 && carrier->register_name.has_value();
}

[[nodiscard]] bool complete_f128_constant_carrier(
    const prepare::PreparedF128Carrier* carrier) {
  return carrier != nullptr &&
         carrier->source_type == bir::TypeKind::F128 &&
         carrier->kind == prepare::PreparedF128CarrierKind::Missing &&
         carrier->missing_required_facts.empty() &&
         carrier->total_size_bytes == 16 &&
         carrier->total_align_bytes == 16 &&
         carrier->constant_payload.has_value();
}

[[nodiscard]] const prepare::PreparedF128Carrier*
find_prepared_f128_carrier_in_module(const prepare::PreparedBirModule& prepared,
                                     prepare::PreparedValueId value_id) {
  for (const auto& function_carriers : prepared.f128_carriers.functions) {
    if (const auto* carrier =
            prepare::find_prepared_f128_carrier(function_carriers, value_id)) {
      return carrier;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_fp_view_from_register_name(
    const std::optional<std::string>& register_name) {
  if (!register_name.has_value() || register_name->empty()) {
    return std::nullopt;
  }
  switch (register_name->front()) {
    case 's':
      return abi::RegisterView::S;
    case 'd':
      return abi::RegisterView::D;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_view_from_register_name(
    const std::optional<std::string>& register_name) {
  if (!register_name.has_value() || register_name->empty()) {
    return std::nullopt;
  }
  switch (register_name->front()) {
    case 'w':
      return abi::RegisterView::W;
    case 'x':
      return abi::RegisterView::X;
    case 's':
      return abi::RegisterView::S;
    case 'd':
      return abi::RegisterView::D;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::size_t scalar_size_from_register_view(
    std::optional<abi::RegisterView> view) {
  switch (view.value_or(abi::RegisterView::W)) {
    case abi::RegisterView::D:
    case abi::RegisterView::X:
      return 8U;
    case abi::RegisterView::S:
    case abi::RegisterView::W:
    default:
      return 4U;
  }
}

[[nodiscard]] std::optional<std::string> register_name_with_expected_view(
    const std::optional<std::string>& register_name,
    std::optional<abi::RegisterView> expected_view) {
  if (!register_name.has_value() || !expected_view.has_value()) {
    return register_name;
  }
  const auto parsed = abi::parse_aarch64_register_name(*register_name);
  if (!parsed.has_value() || parsed->view == *expected_view) {
    return register_name;
  }
  if (abi::is_gp_register(*parsed)) {
    if (const auto viewed = abi::gp_register(parsed->index, *expected_view);
        viewed.has_value()) {
      return std::string{abi::register_name(*viewed)};
    }
  }
  if (abi::is_fp_simd_register(*parsed)) {
    if (const auto viewed = abi::fp_simd_register(parsed->index, *expected_view);
        viewed.has_value()) {
      return std::string{abi::register_name(*viewed)};
    }
  }
  return register_name;
}

[[nodiscard]] std::optional<RegisterOperand> make_register_operand_from_prepared_authority(
    const std::optional<std::string>& register_name,
    const std::optional<prepare::PreparedRegisterPlacement>& placement,
    const std::optional<prepare::PreparedRegisterBank>& bank,
    RegisterOperandRole role,
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name,
    std::size_t contiguous_width,
    const std::vector<std::string>& occupied_registers,
    std::optional<abi::RegisterView> expected_view,
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  const auto prepared_class =
      bank.has_value() ? std::optional<prepare::PreparedRegisterClass>{
                             register_class_from_bank(*bank)}
                       : std::nullopt;
  abi::PreparedRegisterConversionResult converted;
  if (register_name.has_value()) {
    converted = abi::convert_prepared_register(
        *register_name, bank, prepared_class, expected_view);
  } else if (placement.has_value()) {
    converted = abi::convert_prepared_register(*placement, prepared_class, expected_view);
  } else {
    return std::nullopt;
  }
  if (!converted.reg.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::RegisterConversionFailed,
        context,
        instruction_index,
        converted.error.has_value()
            ? converted.error->message
            : "prepared call-boundary move register could not be converted");
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = role,
      .value_id = value_id,
      .value_name = value_name,
      .prepared_class = prepared_class.value_or(prepare::PreparedRegisterClass::None),
      .prepared_bank = bank.value_or(prepare::PreparedRegisterBank::None),
      .expected_view = expected_view,
      .contiguous_width = contiguous_width,
      .occupied_register_references = {*converted.reg},
      .occupied_registers =
          occupied_registers.empty()
              ? std::vector<std::string_view>{abi::register_name(*converted.reg)}
              : std::vector<std::string_view>(occupied_registers.begin(),
                                               occupied_registers.end()),
  };
}

[[nodiscard]] std::optional<RegisterOperand>
make_f128_q_register_operand_from_carrier(
    const prepare::PreparedF128Carrier& carrier,
    RegisterOperandRole role,
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name,
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (!carrier.register_name.has_value()) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*carrier.register_name);
  if (!parsed.has_value() || !abi::is_fp_simd_register(*parsed)) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::RegisterConversionFailed,
        context,
        instruction_index,
        "AArch64 binary128 call-boundary source carrier is not an FP/SIMD register");
    return std::nullopt;
  }
  const auto viewed = abi::fp_simd_register(parsed->index, abi::RegisterView::Q);
  if (!viewed.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::RegisterConversionFailed,
        context,
        instruction_index,
        "AArch64 binary128 call-boundary source carrier could not be re-viewed as q-register");
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *viewed,
      .role = role,
      .value_id = value_id,
      .value_name = value_name,
      .prepared_class = carrier.register_class,
      .prepared_bank = carrier.register_bank,
      .expected_view = abi::RegisterView::Q,
      .contiguous_width = carrier.contiguous_width,
      .occupied_register_references = {*viewed},
      .occupied_registers =
          carrier.occupied_register_names.empty()
              ? std::vector<std::string_view>{abi::register_name(*viewed)}
              : std::vector<std::string_view>(
                    carrier.occupied_register_names.begin(),
                    carrier.occupied_register_names.end()),
  };
}

[[nodiscard]] std::optional<ImmediateOperand> make_scalar_call_argument_immediate(
    const bir::Value& value,
    std::optional<prepare::PreparedValueId> source_value_id) {
  ImmediateKind immediate_kind = ImmediateKind::SignedInteger;
  if (value.type == bir::TypeKind::I1) {
    immediate_kind = ImmediateKind::Boolean;
  }

  switch (value.type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
      return ImmediateOperand{
          .kind = immediate_kind,
          .type = value.type,
          .signed_value = value.immediate,
          .unsigned_value = value.immediate_bits != 0U
                                ? value.immediate_bits
                                : static_cast<std::uint64_t>(value.immediate),
          .source_value_id = source_value_id,
      };
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
      return ImmediateOperand{
          .kind = ImmediateKind::UnsignedInteger,
          .type = value.type,
          .signed_value = value.immediate,
          .unsigned_value = value.immediate_bits,
          .source_value_id = source_value_id,
      };
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_integer_register_view(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return abi::RegisterView::W;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return abi::RegisterView::X;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<bir::TypeKind> scalar_integer_type_from_size(
    std::size_t size_bytes) {
  switch (size_bytes) {
    case 1:
      return bir::TypeKind::I8;
    case 2:
      return bir::TypeKind::I16;
    case 4:
      return bir::TypeKind::I32;
    case 8:
      return bir::TypeKind::I64;
  }
  return std::nullopt;
}

[[nodiscard]] bool is_aarch64_byval_register_lane_move(
    const prepare::PreparedMoveResolution& move) {
  return move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
         (move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register ||
          move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot) &&
         move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
         move.reason == "call_arg_byval_aggregate_register_lanes";
}

[[nodiscard]] std::optional<MemoryOperand>
make_byval_register_lane_prepared_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t size_bytes,
    std::size_t instruction_index) {
  if (size_bytes == 0) {
    return std::nullopt;
  }
  if (argument.source_selection.has_value() &&
      argument.source_selection->kind ==
          prepare::PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane &&
      argument.source_selection->byval_lane_extent_bytes ==
          std::optional<std::size_t>{size_bytes} &&
      argument.source_selection->source_slot_id.has_value() &&
      argument.source_selection->source_stack_offset_bytes.has_value() &&
      argument.source_selection->source_size_bytes.has_value() &&
      argument.source_selection->source_align_bytes.has_value() &&
      *argument.source_selection->source_size_bytes >= size_bytes) {
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
        .result_value_id = argument.source_value_id.has_value()
                               ? argument.source_value_id
                               : std::optional<prepare::PreparedValueId>{
                                     source_home.value_id},
        .result_value_name = std::nullopt,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = argument.source_selection->source_slot_id,
        .byte_offset = static_cast<std::int64_t>(
            *argument.source_selection->source_stack_offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = size_bytes,
        .align_bytes = *argument.source_selection->source_align_bytes,
        .can_use_base_plus_offset = true,
    };
  }
  return std::nullopt;
}

MachineEffectResource local_effect_from_operand(const OperandRecord& operand) {
  MachineEffectResource resource;
  resource.operand = operand;
  switch (operand.kind) {
    case OperandKind::Register: {
      const auto* reg = std::get_if<RegisterOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::Register;
      if (reg != nullptr) {
        resource.value_id = reg->value_id;
        resource.value_name = reg->value_name;
        resource.reg = reg->reg;
      }
      break;
    }
    case OperandKind::Immediate: {
      const auto* immediate = std::get_if<ImmediateOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::PreparedValue;
      if (immediate != nullptr) {
        resource.value_id = immediate->source_value_id;
        resource.value_name = immediate->source_value_name;
      }
      break;
    }
    case OperandKind::PreparedValue: {
      const auto* value = std::get_if<PreparedValueOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::PreparedValue;
      if (value != nullptr) {
        resource.value_id = value->value_id;
        resource.value_name = value->value_name;
      }
      break;
    }
    case OperandKind::FrameSlot: {
      const auto* slot = std::get_if<FrameSlotOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::FrameSlot;
      if (slot != nullptr) {
        resource.frame_slot_id = slot->slot_id;
        if (slot->value_name.has_value()) {
          resource.value_name = *slot->value_name;
        }
      }
      break;
    }
    case OperandKind::Symbol: {
      const auto* symbol = std::get_if<SymbolOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::Symbol;
      if (symbol != nullptr) {
        resource.symbol_name = symbol->link_name;
      }
      break;
    }
    case OperandKind::BranchTarget: {
      const auto* target = std::get_if<BranchTargetOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::BranchTarget;
      if (target != nullptr) {
        resource.value_id = target->condition_value_id;
        resource.block_label = target->block_label;
      }
      break;
    }
    case OperandKind::Memory: {
      const auto* memory = std::get_if<MemoryOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::Memory;
      if (memory != nullptr) {
        resource.value_id = memory->result_value_id.has_value() ? memory->result_value_id
                                                                : memory->stored_value_id;
        if (memory->result_value_name.has_value()) {
          resource.value_name = *memory->result_value_name;
        } else if (memory->stored_value_name.has_value()) {
          resource.value_name = *memory->stored_value_name;
        }
        resource.frame_slot_id = memory->frame_slot_id;
        resource.symbol_name = memory->symbol_name.has_value() ? memory->symbol_name
                                                               : memory->string_symbol_name;
      }
      break;
    }
  }
  return resource;
}

[[nodiscard]] bool call_boundary_frame_slot_direct_offset_is_encodable(
    const MemoryOperand& memory,
    std::size_t load_width_bytes) {
  if (memory.base_kind != MemoryBaseKind::FrameSlot || memory.byte_offset < 0 ||
      load_width_bytes == 0) {
    return false;
  }
  if (load_width_bytes != 1 && load_width_bytes != 2 && load_width_bytes != 4 &&
      load_width_bytes != 8 && load_width_bytes != 16) {
    return false;
  }
  const auto offset = static_cast<std::uint64_t>(memory.byte_offset);
  return offset % load_width_bytes == 0 && offset / load_width_bytes <= 4095U;
}

std::vector<std::string> materialize_call_boundary_frame_slot_address_lines(
    abi::RegisterReference scratch,
    const MemoryOperand& memory) {
  if (memory.base_kind != MemoryBaseKind::FrameSlot || memory.byte_offset < 0) {
    return {};
  }
  const auto offset = static_cast<std::uint64_t>(memory.byte_offset);
  const std::string scratch_name = abi::register_name(scratch);
  if (offset <= 4095U) {
    return {"add " + scratch_name + ", sp, #" + std::to_string(offset)};
  }
  auto lines = materialize_integer_constant_lines(scratch, offset, 64);
  if (lines.empty()) {
    return {};
  }
  lines.push_back("add " + scratch_name + ", sp, " + scratch_name);
  return lines;
}

[[nodiscard]] std::string stack_copy_address(std::string_view base,
                                             std::int64_t offset) {
  std::ostringstream out;
  out << "[" << base;
  if (offset != 0) {
    out << ", #" << offset;
  }
  out << "]";
  return out.str();
}

[[nodiscard]] module::MachineInstruction make_call_boundary_machine_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    InstructionRecord target) {
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

[[nodiscard]] std::optional<MemoryOperand> make_selected_frame_slot_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome* source_home,
    const prepare::PreparedCallArgumentSourceSelection& selection,
    prepare::PreparedCallArgumentSourceSelectionKind expected_kind,
    bool materialized_address,
    std::size_t instruction_index) {
  if (selection.kind != expected_kind ||
      !selection.source_slot_id.has_value() ||
      !selection.source_stack_offset_bytes.has_value() ||
      !selection.source_size_bytes.has_value() ||
      !selection.source_align_bytes.has_value()) {
    return std::nullopt;
  }
  if (expected_kind ==
          prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue &&
      (!selection.source_value_id.has_value() ||
       !selection.source_value_name.has_value() ||
       selection.source_home_kind !=
           std::optional<prepare::PreparedValueHomeKind>{
               prepare::PreparedValueHomeKind::StackSlot})) {
    return std::nullopt;
  }
  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = selection.address_materialization_block_label.has_value()
                         ? *selection.address_materialization_block_label
                         : (context.control_flow_block != nullptr
                                ? context.control_flow_block->block_label
                                : c4c::kInvalidBlockLabel),
      .instruction_index =
          selection.address_materialization_inst_index.value_or(instruction_index),
      .result_value_id = selection.source_value_id.has_value()
                             ? selection.source_value_id
                             : argument.source_value_id,
      .result_value_name = selection.source_value_name.has_value()
                               ? selection.source_value_name
                               : (source_home != nullptr
                                      ? std::optional<c4c::ValueNameId>{
                                            source_home->value_name}
                                      : std::nullopt),
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = materialized_address &&
                               selection.address_materialization_frame_slot_id
                                   .has_value()
                           ? selection.address_materialization_frame_slot_id
                           : selection.source_slot_id,
      .byte_offset =
          materialized_address &&
                  selection.address_materialization_byte_offset.has_value()
              ? *selection.address_materialization_byte_offset
              : static_cast<std::int64_t>(*selection.source_stack_offset_bytes),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = *selection.source_size_bytes,
      .align_bytes = *selection.source_align_bytes,
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] std::optional<MemoryOperand> make_sret_memory_return_address_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index) {
  if (argument.source_selection.has_value()) {
    if (auto selected = make_selected_frame_slot_source(
            context,
            argument,
            nullptr,
            *argument.source_selection,
            prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress,
            true,
            instruction_index)) {
      return selected;
    }
    return std::nullopt;
  }
  if (!call_plan.memory_return.has_value() ||
      !call_plan.memory_return->sret_arg_index.has_value() ||
      *call_plan.memory_return->sret_arg_index != argument.arg_index ||
      call_plan.memory_return->encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      !call_plan.memory_return->slot_id.has_value() ||
      !call_plan.memory_return->stack_offset_bytes.has_value()) {
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
      .result_value_id = argument.source_value_id,
      .result_value_name = std::nullopt,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = call_plan.memory_return->slot_id,
      .byte_offset = static_cast<std::int64_t>(*call_plan.memory_return->stack_offset_bytes),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = call_plan.memory_return->size_bytes,
      .align_bytes = call_plan.memory_return->align_bytes,
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] std::optional<MemoryOperand> make_selected_local_frame_address_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    const prepare::PreparedCallArgumentSourceSelection& selection,
    std::size_t instruction_index) {
  if (selection.kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::
              LocalFrameAddressMaterialization ||
      !selection.source_size_bytes.has_value() ||
      !selection.source_align_bytes.has_value()) {
    return std::nullopt;
  }
  const auto frame_slot_id =
      selection.address_materialization_frame_slot_id.has_value()
          ? selection.address_materialization_frame_slot_id
          : selection.source_slot_id;
  std::optional<std::int64_t> byte_offset;
  if (selection.address_materialization_byte_offset.has_value()) {
    byte_offset = selection.address_materialization_byte_offset;
  } else if (selection.source_stack_offset_bytes.has_value()) {
    byte_offset = static_cast<std::int64_t>(*selection.source_stack_offset_bytes);
  }
  if (!frame_slot_id.has_value() || !byte_offset.has_value()) {
    return std::nullopt;
  }
  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = selection.address_materialization_block_label.has_value()
                         ? *selection.address_materialization_block_label
                         : (context.control_flow_block != nullptr
                                ? context.control_flow_block->block_label
                                : c4c::kInvalidBlockLabel),
      .instruction_index =
          selection.address_materialization_inst_index.value_or(instruction_index),
      .result_value_id = selection.source_value_id.has_value()
                             ? selection.source_value_id
                             : argument.source_value_id,
      .result_value_name = selection.source_value_name.has_value()
                               ? selection.source_value_name
                               : std::optional<c4c::ValueNameId>{source_home.value_name},
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = frame_slot_id,
      .byte_offset = *byte_offset,
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = *selection.source_size_bytes,
      .align_bytes = *selection.source_align_bytes,
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] std::optional<MemoryOperand> make_stack_call_argument_destination(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    const prepare::PreparedMoveResolution& move,
    const prepare::PreparedAbiBinding* binding,
    const MemoryOperand& source,
    std::size_t instruction_index) {
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      source.size_bytes == 0 ||
      source_home.value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto destination_stack_offset =
      binding != nullptr && binding->destination_stack_offset_bytes.has_value()
          ? binding->destination_stack_offset_bytes
          : (move.destination_stack_offset_bytes.has_value()
                 ? move.destination_stack_offset_bytes
                 : argument.destination_stack_offset_bytes);
  if (!destination_stack_offset.has_value()) {
    return std::nullopt;
  }
  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow->function_name,
      .block_label = context.control_flow_block->block_label,
      .instruction_index = instruction_index,
      .stored_value_id = argument.source_value_id.has_value()
                             ? argument.source_value_id
                             : std::optional<prepare::PreparedValueId>{move.from_value_id},
      .stored_value_name = source_home.value_name,
      .base_kind = MemoryBaseKind::Register,
      .base_register = RegisterOperand{
          .reg = outgoing_stack_argument_base_register(),
          .role = RegisterOperandRole::Physical,
          .expected_view = abi::RegisterView::X,
      },
      .byte_offset = static_cast<std::int64_t>(*destination_stack_offset),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = source.size_bytes,
      .align_bytes = source.align_bytes,
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] std::optional<MemoryOperand> make_aggregate_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    const RegisterOperand& source_register,
    std::size_t size_bytes,
    std::int64_t byte_offset,
    std::size_t instruction_index) {
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      source_home.value_name == c4c::kInvalidValueName || size_bytes == 0) {
    return std::nullopt;
  }
  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow->function_name,
      .block_label = context.control_flow_block->block_label,
      .instruction_index = instruction_index,
      .result_value_id = argument.source_value_id.has_value()
                             ? argument.source_value_id
                             : std::optional<prepare::PreparedValueId>{source_home.value_id},
      .result_value_name = source_home.value_name,
      .base_kind = MemoryBaseKind::PointerValue,
      .base_register = source_register,
      .pointer_value_name = source_home.value_name,
      .pointer_value_id = source_home.value_id,
      .byte_offset = byte_offset,
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = size_bytes,
      .align_bytes = source_home.align_bytes.value_or(1),
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] std::optional<PreservedCallArgumentSource>
make_prior_preserved_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentSourceSelection& selection,
    const prepare::PreparedValueHome* source_home,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (selection.kind !=
      prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation) {
    return std::nullopt;
  }

  if (selection.preservation_route ==
      prepare::PreparedCallPreservationRoute::CalleeSavedRegister) {
    if (!selection.preserved_register_name.has_value() ||
        !selection.preserved_register_bank.has_value() ||
        !selection.preserved_register_contiguous_width.has_value() ||
        *selection.preserved_register_contiguous_width == 0 ||
        selection.preserved_occupied_register_names.empty() ||
        !selection.preserved_register_placement.has_value()) {
      return std::nullopt;
    }
    const auto source_view =
        source_home != nullptr && source_home->size_bytes.has_value()
            ? scalar_integer_register_view_from_size(*source_home->size_bytes)
            : scalar_view_from_register_name(selection.preserved_register_name);
    auto source = make_register_operand_from_prepared_authority(
        selection.preserved_register_name,
        selection.preserved_register_placement,
        selection.preserved_register_bank,
        RegisterOperandRole::CallAbi,
        selection.source_value_id,
        selection.source_value_name.value_or(c4c::kInvalidValueName),
        *selection.preserved_register_contiguous_width,
        selection.preserved_occupied_register_names,
        source_view,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value()) {
      return std::nullopt;
    }
    source->value_name = c4c::kInvalidValueName;
    PreservedCallArgumentSource result;
    result.source_register = *source;
    return result;
  }

  if (selection.preservation_route !=
          prepare::PreparedCallPreservationRoute::StackSlot ||
      !selection.source_value_id.has_value() ||
      !selection.source_value_name.has_value() ||
      !selection.preserved_stack_slot_id.has_value() ||
      !selection.preserved_stack_offset_bytes.has_value() ||
      !selection.preserved_stack_size_bytes.has_value() ||
      !selection.preserved_stack_align_bytes.has_value() ||
      *selection.preserved_stack_size_bytes == 0) {
    return std::nullopt;
  }
  PreservedCallArgumentSource result;
  result.source_memory = MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .result_value_id = selection.source_value_id,
      .result_value_name = selection.source_value_name,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = selection.preserved_stack_slot_id,
      .byte_offset =
          static_cast<std::int64_t>(*selection.preserved_stack_offset_bytes),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = *selection.preserved_stack_size_bytes,
      .align_bytes = *selection.preserved_stack_align_bytes,
      .can_use_base_plus_offset = true,
  };
  return result;
}

[[nodiscard]] std::optional<prepare::PreparedValueId> effect_value_id(
    const prepare::PreparedCallBoundaryEffectEndpoint& primary,
    const prepare::PreparedCallBoundaryEffectEndpoint& fallback) {
  return primary.value_id.has_value() ? primary.value_id : fallback.value_id;
}

[[nodiscard]] ValueNameId effect_value_name(
    const prepare::PreparedCallBoundaryEffectEndpoint& primary,
    const prepare::PreparedCallBoundaryEffectEndpoint& fallback) {
  return primary.value_name != c4c::kInvalidValueName ? primary.value_name
                                                       : fallback.value_name;
}

[[nodiscard]] bool endpoint_has_stack_storage(
    const prepare::PreparedCallBoundaryEffectEndpoint& endpoint) {
  return endpoint.storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
         endpoint.slot_id.has_value() &&
         endpoint.stack_offset_bytes.has_value() &&
         endpoint.stack_size_bytes.has_value() &&
         *endpoint.stack_size_bytes != 0;
}

[[nodiscard]] std::optional<MemoryOperand> make_frame_slot_memory_from_endpoint(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallBoundaryEffectEndpoint& endpoint,
    prepare::PreparedValueId value_id,
    ValueNameId value_name,
    std::size_t instruction_index) {
  if (!endpoint_has_stack_storage(endpoint)) {
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
      .result_value_id = value_id,
      .result_value_name = value_name,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = endpoint.slot_id,
      .byte_offset = static_cast<std::int64_t>(*endpoint.stack_offset_bytes),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = *endpoint.stack_size_bytes,
      .align_bytes = endpoint.stack_align_bytes.value_or(*endpoint.stack_size_bytes),
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] const prepare::PreparedCallPreservedValue*
find_prior_stack_preserved_value_before_instruction(
    const module::BlockLoweringContext& context,
    prepare::PreparedValueId value_id,
    std::size_t instruction_index) {
  if (context.function.call_plan_lookups == nullptr) {
    return nullptr;
  }
  const auto* preserved =
      prepare::find_latest_indexed_prior_stack_preserved_value_before_instruction(
          *context.function.call_plan_lookups,
          value_id,
          context.block_index,
          instruction_index);
  if (preserved == nullptr ||
      preserved->route != prepare::PreparedCallPreservationRoute::StackSlot) {
    return nullptr;
  }
  if (!endpoint_has_stack_storage(preserved->preservation_destination)) {
    return nullptr;
  }
  return preserved;
}

[[nodiscard]] std::optional<MemoryOperand> make_prior_stack_preserved_value_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPreservedValue& preserved,
    std::size_t instruction_index) {
  return make_frame_slot_memory_from_endpoint(
      context,
      preserved.preservation_destination,
      preserved.value_id,
      preserved.value_name,
      instruction_index);
}

[[nodiscard]] module::MachineInstruction make_outgoing_stack_base_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::size_t outgoing_bytes) {
  const auto scratch = outgoing_stack_argument_base_register();
  std::ostringstream asm_text;
  asm_text << "sub " << abi::register_name(scratch) << ", sp, #" << outgoing_bytes;
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .defs = {MachineEffectResource{
          .kind = MachineEffectResourceKind::Register,
          .reg = scratch,
      }},
      .side_effects = {MachineSideEffectKind::InlineAssembly},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = asm_text.str(),
      },
  };
  return make_call_boundary_machine_instruction(context, instruction_index, std::move(target));
}

[[nodiscard]] std::optional<std::string_view> value_move_load_mnemonic(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return std::string_view{"ldrb"};
    case 2:
      return std::string_view{"ldrh"};
    case 4:
    case 8:
      return std::string_view{"ldr"};
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> value_move_store_mnemonic(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return std::string_view{"strb"};
    case 2:
      return std::string_view{"strh"};
    case 4:
    case 8:
      return std::string_view{"str"};
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<abi::RegisterReference> value_move_scratch_register(
    std::size_t width_bytes) {
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  if (width_bytes == 1 || width_bytes == 2 || width_bytes == 4) {
    return abi::w_register(scratches.front().index);
  }
  if (width_bytes == 8) {
    return abi::x_register(scratches.front().index);
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string> value_move_frame_slot_address(
    const prepare::PreparedValueHome& home) {
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home.offset_bytes.has_value()) {
    return std::nullopt;
  }
  std::ostringstream out;
  out << "[sp";
  if (*home.offset_bytes != 0) {
    out << ", #" << *home.offset_bytes;
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_value_stack_move_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move,
    const prepare::PreparedValueHome& destination_home,
    const prepare::PreparedValueHome* source_home,
    std::size_t instruction_index) {
  const auto width_bytes =
      destination_home.size_bytes.value_or(source_home != nullptr
                                               ? source_home->size_bytes.value_or(4)
                                               : 4);
  const auto store_mnemonic = value_move_store_mnemonic(width_bytes);
  const auto destination = value_move_frame_slot_address(destination_home);
  if (!store_mnemonic.has_value() || !destination.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  std::string value_register;
  if (move.source_immediate_i32.has_value()) {
    const auto scratch = value_move_scratch_register(width_bytes);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    value_register = std::string{abi::register_name(*scratch)};
    auto materialized = materialize_integer_constant_lines(
        *scratch,
        static_cast<std::uint64_t>(
            static_cast<std::uint32_t>(*move.source_immediate_i32)),
        width_bytes == 8 ? 64U : 32U);
    if (materialized.empty()) {
      return std::nullopt;
    }
    lines.insert(lines.end(), materialized.begin(), materialized.end());
  } else if (source_home != nullptr &&
             source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
             source_home->offset_bytes.has_value()) {
    const auto scratch = value_move_scratch_register(width_bytes);
    const auto load_mnemonic = value_move_load_mnemonic(width_bytes);
    const auto source = value_move_frame_slot_address(*source_home);
    if (!scratch.has_value() || !load_mnemonic.has_value() || !source.has_value()) {
      return std::nullopt;
    }
    value_register = std::string{abi::register_name(*scratch)};
    lines.push_back(std::string{*load_mnemonic} + " " + value_register + ", " +
                    *source);
  } else if (source_home != nullptr &&
             source_home->kind == prepare::PreparedValueHomeKind::Register &&
             source_home->register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*source_home->register_name);
    if (!parsed.has_value() ||
        (parsed->bank != abi::RegisterBank::GeneralPurpose &&
         parsed->bank != abi::RegisterBank::FpSimd)) {
      return std::nullopt;
    }
    std::optional<abi::RegisterReference> source_reg;
    if (parsed->bank == abi::RegisterBank::FpSimd) {
      source_reg = abi::fp_simd_register(
          parsed->index,
          width_bytes == 8 ? abi::RegisterView::D : abi::RegisterView::S);
    } else {
      source_reg = width_bytes == 8 ? abi::x_register(parsed->index)
                                    : abi::w_register(parsed->index);
    }
    if (!source_reg.has_value()) {
      return std::nullopt;
    }
    value_register = std::string{abi::register_name(*source_reg)};
  } else {
    return std::nullopt;
  }
  lines.push_back(std::string{*store_mnemonic} + " " + value_register + ", " +
                  *destination);

  std::string asm_text;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += lines[index];
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .defs = {MachineEffectResource{
          .kind = MachineEffectResourceKind::PreparedValue,
          .value_id = destination_home.value_id,
          .value_name = destination_home.value_name,
      }},
      .uses = source_home != nullptr
                  ? std::vector<MachineEffectResource>{MachineEffectResource{
                        .kind = MachineEffectResourceKind::PreparedValue,
                        .value_id = source_home->value_id,
                        .value_name = source_home->value_name,
                    }}
                  : std::vector<MachineEffectResource>{},
      .side_effects = {MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_call_boundary_machine_instruction(context, instruction_index,
                                                std::move(target));
}

[[nodiscard]] module::MachineInstruction make_aggregate_stack_copy_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const MemoryOperand& source,
    const MemoryOperand& destination) {
  const auto source_base = abi::register_name(source.base_register->reg);
  const std::string destination_base =
      destination.base_kind == MemoryBaseKind::Register && destination.base_register.has_value()
          ? std::string{abi::register_name(destination.base_register->reg)}
          : std::string{"sp"};
  std::string asm_text;
  std::size_t offset = 0;
  for (const auto width : aggregate_stack_copy_chunks(source.size_bytes)) {
    const auto scratch = aggregate_stack_copy_scratch(width);
    const auto load_mnemonic = aggregate_stack_copy_load_mnemonic(width);
    const auto store_mnemonic = aggregate_stack_copy_store_mnemonic(width);
    if (!scratch.has_value() || load_mnemonic.empty() || store_mnemonic.empty()) {
      continue;
    }
    if (!asm_text.empty()) {
      asm_text += '\n';
    }
    asm_text += std::string{load_mnemonic};
    asm_text += " ";
    asm_text += std::string{abi::register_name(*scratch)};
    asm_text += ", ";
    asm_text += stack_copy_address(
        source_base, source.byte_offset + static_cast<std::int64_t>(offset));
    asm_text += '\n';
    asm_text += std::string{store_mnemonic};
    asm_text += " ";
    asm_text += std::string{abi::register_name(*scratch)};
    asm_text += ", ";
    asm_text += stack_copy_address(
        destination_base, destination.byte_offset + static_cast<std::int64_t>(offset));
    offset += width;
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected},
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .operands = {make_memory_operand(source), make_memory_operand(destination)},
      .defs = {local_effect_from_operand(make_memory_operand(destination))},
      .uses = {local_effect_from_operand(make_memory_operand(source))},
      .side_effects = {MachineSideEffectKind::MemoryRead,
                       MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .operands = {make_memory_operand(source), make_memory_operand(destination)},
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_call_boundary_machine_instruction(context, instruction_index,
                                                std::move(target));
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_byval_register_lane_stack_publication_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const MemoryOperand& source,
    const MemoryOperand& destination) {
  if (source.size_bytes == 0 || source.size_bytes != destination.size_bytes) {
    return std::nullopt;
  }

  auto chunks = aggregate_stack_copy_chunks(source.size_bytes);
  bool printable_chunks = true;
  std::size_t printable_offset = 0;
  for (const auto width : chunks) {
    auto source_chunk =
        aggregate_register_lane_memory(source, printable_offset, width);
    auto destination_chunk =
        aggregate_register_lane_memory(destination, printable_offset, width);
    if (!aggregate_register_lane_memory_is_printable(source_chunk, width) ||
        !aggregate_register_lane_memory_is_printable(destination_chunk, width)) {
      printable_chunks = false;
      break;
    }
    printable_offset += width;
  }
  if (!printable_chunks) {
    chunks.assign(source.size_bytes, std::size_t{1});
  }

  std::string asm_text;
  std::size_t offset = 0;
  for (const auto width : chunks) {
    auto source_chunk = aggregate_register_lane_memory(source, offset, width);
    auto destination_chunk =
        aggregate_register_lane_memory(destination, offset, width);
    if (!aggregate_register_lane_memory_is_printable(source_chunk, width) ||
        !aggregate_register_lane_memory_is_printable(destination_chunk, width)) {
      return std::nullopt;
    }
    const auto scratch = aggregate_stack_copy_scratch(width);
    const auto load_mnemonic = aggregate_stack_copy_load_mnemonic(width);
    const auto store_mnemonic = aggregate_stack_copy_store_mnemonic(width);
    if (!scratch.has_value() || load_mnemonic.empty() || store_mnemonic.empty()) {
      return std::nullopt;
    }
    if (!asm_text.empty()) {
      asm_text += '\n';
    }
    asm_text += std::string{load_mnemonic};
    asm_text += " ";
    asm_text += std::string{abi::register_name(*scratch)};
    asm_text += ", ";
    asm_text += memory_address(source_chunk);
    asm_text += '\n';
    asm_text += std::string{store_mnemonic};
    asm_text += " ";
    asm_text += std::string{abi::register_name(*scratch)};
    asm_text += ", ";
    asm_text += memory_address(destination_chunk);
    offset += width;
  }
  if (asm_text.empty()) {
    return std::nullopt;
  }

  const auto source_operand = make_memory_operand(source);
  const auto destination_operand = make_memory_operand(destination);
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .operands = {source_operand, destination_operand},
      .defs = {local_effect_from_operand(destination_operand)},
      .uses = {local_effect_from_operand(source_operand)},
      .side_effects = {MachineSideEffectKind::MemoryRead,
                       MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .operands = {source_operand, destination_operand},
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_call_boundary_machine_instruction(context, instruction_index,
                                                std::move(target));
}

[[nodiscard]] std::optional<std::size_t> selected_byval_lane_extent_bytes(
    const prepare::PreparedCallArgumentPlan& argument) {
  if (!argument.source_selection.has_value() ||
      argument.source_selection->kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane) {
    return std::nullopt;
  }
  return argument.source_selection->byval_lane_extent_bytes;
}

[[nodiscard]] bool has_selected_byval_register_lane_source(
    const prepare::PreparedCallArgumentPlan& argument) {
  return argument.source_selection.has_value() &&
         argument.source_selection->kind ==
             prepare::PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane;
}

[[nodiscard]] const bir::CastInst* find_same_block_cast_producer(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr || context.bir_block == nullptr ||
      value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  const auto spelling = context.function.prepared->names.value_names.spelling(value_name);
  if (spelling.empty()) {
    return nullptr;
  }
  const auto limit = std::min(before_instruction_index, context.bir_block->insts.size());
  for (std::size_t index = 0; index < limit; ++index) {
    const auto* cast = std::get_if<bir::CastInst>(&context.bir_block->insts[index]);
    if (cast != nullptr && cast->result.kind == bir::Value::Kind::Named &&
        cast->result.name == spelling) {
      return cast;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<unsigned> integer_width_bits_for_type(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return 32U;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return 64U;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::uint64_t immediate_integer_bits(const bir::Value& value,
                                                   unsigned width_bits) {
  if (value.immediate_bits != 0U) {
    return width_bits == 32U ? static_cast<std::uint32_t>(value.immediate_bits)
                             : value.immediate_bits;
  }
  return width_bits == 32U ? static_cast<std::uint32_t>(value.immediate)
                           : static_cast<std::uint64_t>(value.immediate);
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_fp_register_view(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
      return abi::RegisterView::S;
    case bir::TypeKind::F64:
      return abi::RegisterView::D;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<abi::RegisterReference> scalar_fp_register_view(
    abi::RegisterReference reg,
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
      return abi::fp_simd_register(reg.index, abi::RegisterView::S);
    case bir::TypeKind::F64:
      return abi::fp_simd_register(reg.index, abi::RegisterView::D);
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<abi::RegisterReference> scalar_gp_register_view(
    abi::RegisterReference reg,
    unsigned width_bits) {
  if (width_bits == 32U) {
    return abi::gp_register(reg.index, abi::RegisterView::W);
  }
  if (width_bits == 64U) {
    return abi::gp_register(reg.index, abi::RegisterView::X);
  }
  return std::nullopt;
}

bool append_materialize_fp_immediate(std::vector<std::string>& lines,
                                     const bir::Value& value,
                                     abi::RegisterReference gp_scratch_base,
                                     abi::RegisterReference fp_scratch_base,
                                     abi::RegisterReference& fp_scratch) {
  const auto fp_view = scalar_fp_register_view(fp_scratch_base, value.type);
  if (!fp_view.has_value() || value.kind != bir::Value::Kind::Immediate) {
    return false;
  }
  fp_scratch = *fp_view;
  if (value.type == bir::TypeKind::F32) {
    const auto gp_scratch = abi::gp_register(gp_scratch_base.index, abi::RegisterView::W);
    if (!gp_scratch.has_value()) {
      return false;
    }
    auto materialize =
        materialize_integer_constant_lines(*gp_scratch, value.immediate_bits, 32);
    if (materialize.empty()) {
      return false;
    }
    lines.insert(lines.end(), materialize.begin(), materialize.end());
    lines.push_back("fmov " + std::string{abi::register_name(*fp_view)} + ", " +
                    std::string{abi::register_name(*gp_scratch)});
    return true;
  }
  if (value.type == bir::TypeKind::F64) {
    const auto gp_scratch = abi::gp_register(gp_scratch_base.index, abi::RegisterView::X);
    if (!gp_scratch.has_value()) {
      return false;
    }
    auto materialize =
        materialize_integer_constant_lines(*gp_scratch, value.immediate_bits, 64);
    if (materialize.empty()) {
      return false;
    }
    lines.insert(lines.end(), materialize.begin(), materialize.end());
    lines.push_back("fmov " + std::string{abi::register_name(*fp_view)} + ", " +
                    std::string{abi::register_name(*gp_scratch)});
    return true;
  }
  return false;
}

[[nodiscard]] std::optional<std::vector<std::string>>
make_immediate_cast_call_argument_publication_lines(
    const bir::CastInst& cast,
    const RegisterOperand& destination) {
  if (cast.operand.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  const auto gp_scratch_base = abi::reserved_mir_scratch_gp_registers().front();
  const auto fp_scratch_base = abi::reserved_mir_scratch_fp_simd_registers().front();
  std::vector<std::string> lines;
  switch (cast.opcode) {
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI: {
      if (!abi::is_gp_register(destination.reg)) {
        return std::nullopt;
      }
      abi::RegisterReference fp_source{};
      if (!append_materialize_fp_immediate(
              lines, cast.operand, gp_scratch_base, fp_scratch_base, fp_source)) {
        return std::nullopt;
      }
      const auto width_bits = integer_width_bits_for_type(cast.result.type);
      const auto viewed_destination =
          width_bits.has_value()
              ? scalar_gp_register_view(destination.reg, *width_bits)
              : std::nullopt;
      if (!viewed_destination.has_value()) {
        return std::nullopt;
      }
      lines.push_back(std::string{cast.opcode == bir::CastOpcode::FPToSI ? "fcvtzs "
                                                                         : "fcvtzu "} +
                      std::string{abi::register_name(*viewed_destination)} + ", " +
                      std::string{abi::register_name(fp_source)});
      return lines;
    }
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP: {
      if (!abi::is_fp_simd_register(destination.reg)) {
        return std::nullopt;
      }
      const auto source_width = integer_width_bits_for_type(cast.operand.type);
      const auto gp_scratch = source_width.has_value()
                                  ? scalar_gp_register_view(gp_scratch_base, *source_width)
                                  : std::nullopt;
      const auto fp_destination = scalar_fp_register_view(destination.reg, cast.result.type);
      if (!source_width.has_value() || !gp_scratch.has_value() ||
          !fp_destination.has_value()) {
        return std::nullopt;
      }
      auto materialize = materialize_integer_constant_lines(
          *gp_scratch, immediate_integer_bits(cast.operand, *source_width), *source_width);
      if (materialize.empty()) {
        return std::nullopt;
      }
      lines.insert(lines.end(), materialize.begin(), materialize.end());
      lines.push_back(std::string{cast.opcode == bir::CastOpcode::SIToFP ? "scvtf "
                                                                         : "ucvtf "} +
                      std::string{abi::register_name(*fp_destination)} + ", " +
                      std::string{abi::register_name(*gp_scratch)});
      return lines;
    }
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::FPTrunc: {
      if (!abi::is_fp_simd_register(destination.reg)) {
        return std::nullopt;
      }
      abi::RegisterReference fp_source{};
      if (!append_materialize_fp_immediate(
              lines, cast.operand, gp_scratch_base, fp_scratch_base, fp_source)) {
        return std::nullopt;
      }
      const auto fp_destination = scalar_fp_register_view(destination.reg, cast.result.type);
      if (!fp_destination.has_value()) {
        return std::nullopt;
      }
      lines.push_back("fcvt " + std::string{abi::register_name(*fp_destination)} +
                      ", " + std::string{abi::register_name(fp_source)});
      return lines;
    }
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_immediate_cast_call_argument_publication_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& source_home,
    const RegisterOperand& destination,
    std::size_t instruction_index) {
  const auto* cast =
      find_same_block_cast_producer(context, source_home.value_name, instruction_index);
  if (cast == nullptr) {
    return std::nullopt;
  }
  const auto lines =
      make_immediate_cast_call_argument_publication_lines(*cast, destination);
  if (!lines.has_value() || lines->empty()) {
    return std::nullopt;
  }
  std::string asm_text;
  for (std::size_t index = 0; index < lines->size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += (*lines)[index];
  }
  const auto destination_operand = make_register_operand(destination);
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected},
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .operands = {destination_operand},
      .defs = {local_effect_from_operand(destination_operand)},
      .uses = {MachineEffectResource{
          .kind = MachineEffectResourceKind::PreparedValue,
          .value_id = source_home.value_id,
          .value_name = source_home.value_name,
      }},
      .side_effects = {MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .operands = {destination_operand},
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_call_boundary_machine_instruction(context, instruction_index, std::move(target));
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_before_call_move(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* source_home =
      context.function.value_locations == nullptr
          ? nullptr
          : find_value_home(context, move.from_value_id);
  const auto classification =
      prepare::classify_prepared_call_boundary_move(call_plan, bundle, move);
  const auto* argument = classification.argument_plan;
  const auto* binding = classification.abi_binding;
  const auto* f128_carriers =
      context.function.prepared == nullptr
          ? nullptr
          : prepare::find_prepared_f128_carriers(
                *context.function.prepared,
                context.function.control_flow != nullptr
                    ? context.function.control_flow->function_name
                    : c4c::kInvalidFunctionName);
  const auto* source_f128_carrier =
      f128_carriers != nullptr && source_home != nullptr
          ? prepare::find_prepared_f128_carrier(*f128_carriers, source_home->value_name)
          : nullptr;
  if (source_f128_carrier == nullptr && f128_carriers != nullptr && argument != nullptr &&
      argument->source_value_id.has_value()) {
    source_f128_carrier =
        prepare::find_prepared_f128_carrier(*f128_carriers, *argument->source_value_id);
  }
  if (source_f128_carrier == nullptr && context.function.prepared != nullptr &&
      argument != nullptr && argument->source_value_id.has_value()) {
    source_f128_carrier =
        find_prepared_f128_carrier_in_module(*context.function.prepared,
                                             *argument->source_value_id);
  }
  const bool structured_f128_register_argument_move =
      argument != nullptr &&
      (argument->source_register_bank == prepare::PreparedRegisterBank::Vreg ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Fpr) &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Vreg &&
      complete_full_width_f128_carrier(source_f128_carrier);

  CallBoundaryMoveInstructionRecord move_record{
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .phase = bundle.phase,
      .authority_kind = bundle.authority_kind,
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .source_parallel_copy_predecessor_label =
          bundle.source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label =
          bundle.source_parallel_copy_successor_label,
      .move = move,
      .source_bundle = &bundle,
      .source_move = &move,
  };
  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      argument != nullptr) {
    const auto* source_selection = argument->source_selection.has_value()
                                       ? &*argument->source_selection
                                       : nullptr;
    const bool frame_slot_address_argument =
        source_home != nullptr &&
        argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
        ((source_selection != nullptr &&
          source_selection->kind ==
              prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress) ||
         (source_selection == nullptr &&
          make_sret_memory_return_address_source(
              context, call_plan, *argument, instruction_index)
              .has_value()));
    const bool local_frame_address_argument =
        source_home != nullptr &&
        source_selection != nullptr &&
        source_selection->kind ==
            prepare::PreparedCallArgumentSourceSelectionKind::
                LocalFrameAddressMaterialization;
    const bool register_byval_argument =
        is_aarch64_byval_register_lane_move(move) ||
        (source_home != nullptr &&
         selected_byval_lane_extent_bytes(*argument).has_value());
    const bool indirect_byval_argument =
        source_home != nullptr &&
        source_selection != nullptr &&
        source_selection->kind ==
            prepare::PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane;
    if (!frame_slot_address_argument && !local_frame_address_argument &&
        !structured_f128_register_argument_move && !register_byval_argument &&
        !indirect_byval_argument) {
      const auto* prior_selection =
          argument->source_selection.has_value() &&
                  argument->source_selection->kind ==
                      prepare::PreparedCallArgumentSourceSelectionKind::
                          PriorPreservation
              ? &*argument->source_selection
              : nullptr;
      std::optional<PreservedCallArgumentSource> preserved_source;
      std::optional<ValueNameId> preserved_source_value_name;
      if (prior_selection == nullptr && argument->source_selection.has_value() &&
          argument->source_selection->kind ==
              prepare::PreparedCallArgumentSourceSelectionKind::
                  LocalFrameAddressMaterialization) {
        append_call_diagnostic(
            diagnostics,
            module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
            context,
            instruction_index,
            "AArch64 prior-preserved call argument requires prepared PriorPreservation source selection");
        return std::nullopt;
      }
      if (prior_selection != nullptr) {
        preserved_source = make_prior_preserved_call_argument_source(
            context,
            *prior_selection,
            source_home,
            instruction_index,
            diagnostics);
        preserved_source_value_name = prior_selection->source_value_name;
        if (!preserved_source.has_value()) {
          append_call_diagnostic(
              diagnostics,
              module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
              context,
              instruction_index,
              "AArch64 prior-preserved call argument requires complete prepared source selection");
          return std::nullopt;
        }
      }
      if (prior_selection != nullptr || preserved_source.has_value()) {
        if (preserved_source.has_value()) {
          const auto destination_register_placement =
              binding != nullptr && binding->destination_register_placement.has_value()
                  ? binding->destination_register_placement
                  : (move.destination_register_placement.has_value()
                         ? move.destination_register_placement
                         : argument->destination_register_placement);
          const auto destination_register_name =
              destination_register_placement.has_value()
                  ? std::optional<std::string>{}
                  : (binding != nullptr && binding->destination_register_name.has_value()
                         ? binding->destination_register_name
                         : (argument->destination_register_name.has_value()
                                ? argument->destination_register_name
                                : move.destination_register_name));
          auto destination = make_register_operand_from_prepared_authority(
              destination_register_name,
              destination_register_placement,
              argument->destination_register_bank,
              RegisterOperandRole::CallAbi,
              move.to_value_id != 0
                  ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                  : argument->source_value_id,
              preserved_source_value_name.value_or(c4c::kInvalidValueName),
              binding != nullptr ? binding->destination_contiguous_width
                                 : argument->destination_contiguous_width,
              binding != nullptr ? binding->destination_occupied_register_names
                                 : argument->destination_occupied_register_names,
              preserved_source->source_memory.has_value()
                  ? scalar_integer_register_view_from_size(
                        preserved_source->source_memory->size_bytes)
                  : (source_home != nullptr && source_home->size_bytes.has_value()
                         ? scalar_integer_register_view_from_size(*source_home->size_bytes)
                         : scalar_view_from_register_name(move.destination_register_name)),
              diagnostics,
              context,
              instruction_index);
          if (destination.has_value()) {
            move_record.source_register = preserved_source->source_register;
            move_record.source_memory = preserved_source->source_memory;
            if (prior_selection != nullptr &&
                prior_selection->preservation_route ==
                    prepare::PreparedCallPreservationRoute::StackSlot &&
                move_record.source_memory.has_value()) {
              move_record.source_memory->result_value_name.reset();
            }
            move_record.destination_register = *destination;
            return make_call_boundary_machine_instruction(
                context,
                instruction_index,
                make_call_boundary_move_instruction(std::move(move_record)));
          }
        } else {
          return std::nullopt;
        }
      }
    }
  }

  const bool selected_gpr_argument_move =
      argument != nullptr &&
      (argument->source_encoding == prepare::PreparedStorageEncodingKind::Register ||
       argument->source_encoding == prepare::PreparedStorageEncodingKind::SymbolAddress) &&
      argument->source_register_name.has_value() &&
      argument->source_register_bank == prepare::PreparedRegisterBank::Gpr &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr;
  const bool selected_f128_argument_move =
      structured_f128_register_argument_move;
  const bool selected_scalar_fpr_argument_move =
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::Register &&
      argument->source_register_bank == prepare::PreparedRegisterBank::Fpr &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Fpr &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register) &&
      scalar_fp_view_from_register_name(
          binding != nullptr && binding->destination_register_name.has_value()
              ? binding->destination_register_name
              : move.destination_register_name).has_value();
  const bool selected_f128_constant_argument_move =
      argument != nullptr &&
      argument->value_bank == prepare::PreparedRegisterBank::Vreg &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
      argument->source_literal.has_value() &&
      argument->source_literal->type == bir::TypeKind::F128 &&
      argument->source_literal->f128_payload.has_value() &&
      argument->source_value_id.has_value() &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Vreg &&
      complete_f128_constant_carrier(source_f128_carrier) &&
      source_f128_carrier->constant_payload->low_bits ==
          argument->source_literal->f128_payload->low_bits &&
      source_f128_carrier->constant_payload->high_bits ==
          argument->source_literal->f128_payload->high_bits &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register;

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
      argument->value_bank == prepare::PreparedRegisterBank::Vreg &&
      !selected_f128_constant_argument_move) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        "AArch64 binary128 constant argument move requires a complete structured full-width constant carrier");
    return std::nullopt;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::Register &&
      source_home->register_name.has_value() && argument != nullptr &&
      (selected_gpr_argument_move || selected_scalar_fpr_argument_move ||
       selected_f128_argument_move) &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register) &&
      !move_record.destination_register.has_value()) {
    if (argument->source_register_name.has_value() &&
        *argument->source_register_name != *source_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 call-boundary move source register disagrees with prepared value home");
      return std::nullopt;
    }
    if (selected_f128_argument_move &&
        source_f128_carrier->register_name != source_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 binary128 call-boundary move source register disagrees with prepared f128 carrier");
      return std::nullopt;
    }
    const auto expected_view =
        selected_f128_argument_move ? std::optional<abi::RegisterView>{abi::RegisterView::Q}
        : selected_scalar_fpr_argument_move
            ? scalar_fp_view_from_register_name(
                  binding != nullptr && binding->destination_register_name.has_value()
                      ? binding->destination_register_name
                      : move.destination_register_name)
            : std::nullopt;
    const auto source_register_name =
        selected_scalar_fpr_argument_move &&
                argument->source_register_placement.has_value()
            ? std::optional<std::string>{}
            : source_home->register_name;
    auto destination = make_register_operand_from_prepared_authority(
        binding != nullptr && binding->destination_register_name.has_value()
            ? binding->destination_register_name
            : move.destination_register_name,
        binding != nullptr && binding->destination_register_placement.has_value()
            ? binding->destination_register_placement
            : (move.destination_register_placement.has_value()
                   ? move.destination_register_placement
                   : argument->destination_register_placement),
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : std::nullopt,
        source_home->value_name,
        binding != nullptr ? binding->destination_contiguous_width
                           : move.destination_contiguous_width,
        binding != nullptr ? binding->destination_occupied_register_names
                           : move.destination_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      return std::nullopt;
    }
    move_record.destination_register = *destination;
    if (selected_f128_argument_move) {
      auto source = make_f128_q_register_operand_from_carrier(
          *source_f128_carrier,
          RegisterOperandRole::CallAbi,
          source_home->value_id,
          source_home->value_name,
          diagnostics,
          context,
          instruction_index);
      if (!source.has_value()) {
        return std::nullopt;
      }
      move_record.source_register = *source;
      move_record.source_f128_carrier = source_f128_carrier;
    } else {
      auto source = make_register_operand_from_prepared_authority(
          source_register_name,
          argument->source_register_placement,
          argument->source_register_bank,
          RegisterOperandRole::CallAbi,
          source_home->value_id,
          source_home->value_name,
          1,
          std::vector<std::string>{},
          expected_view,
          diagnostics,
          context,
          instruction_index);
      if (!source.has_value()) {
        return std::nullopt;
      }
      move_record.source_register = *source;
    }
    }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::Register &&
      argument != nullptr &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register)) {
    const auto register_byval_size = selected_byval_lane_extent_bytes(*argument);
    std::optional<MemoryOperand> source;
    std::optional<MemoryOperand> address_source;
    if (register_byval_size.has_value()) {
      source = make_byval_register_lane_prepared_source(
          context, *argument, *source_home, *register_byval_size, call_plan.instruction_index);
    } else {
      if (argument->source_selection.has_value()) {
        address_source = make_selected_call_argument_source(
            context,
            *argument,
            source_home,
            *argument->source_selection,
            prepare::PreparedCallArgumentSourceSelectionKind::
                LocalFrameAddressMaterialization,
            instruction_index);
      }
      source = address_source;
      if (!source.has_value() && argument->source_selection.has_value() &&
          argument->source_selection->kind ==
              prepare::PreparedCallArgumentSourceSelectionKind::
                  LocalFrameAddressMaterialization) {
        append_call_diagnostic(
            diagnostics,
            module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
            context,
            instruction_index,
            "AArch64 prior-preserved call argument requires prepared PriorPreservation source selection");
        return std::nullopt;
      }
    }
    if (source.has_value()) {
      const auto destination_register_placement =
          binding != nullptr && binding->destination_register_placement.has_value()
              ? binding->destination_register_placement
              : (move.destination_register_placement.has_value()
                     ? move.destination_register_placement
                     : argument->destination_register_placement);
      const auto destination_register_name =
          destination_register_placement.has_value()
              ? std::optional<std::string>{}
              : (binding != nullptr && binding->destination_register_name.has_value()
                     ? binding->destination_register_name
                     : move.destination_register_name);
      auto destination = make_register_operand_from_prepared_authority(
          destination_register_name,
          destination_register_placement,
          argument->destination_register_bank,
          RegisterOperandRole::CallAbi,
          move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                                : argument->source_value_id,
          source_home->value_name,
          binding != nullptr ? binding->destination_contiguous_width
                             : move.destination_contiguous_width,
          binding != nullptr ? binding->destination_occupied_register_names
                             : move.destination_occupied_register_names,
          abi::RegisterView::X,
          diagnostics,
          context,
          instruction_index);
      if (!destination.has_value()) {
        return std::nullopt;
      }
      move_record.source_register.reset();
      move_record.source_memory = *source;
      move_record.source_memory_materializes_address = true;
      move_record.destination_register = *destination;
      if (register_byval_size.has_value()) {
        move_record.source_memory_materializes_address = false;
        move_record.move.reason = "call_arg_byval_aggregate_register_lanes";
      }
    }
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::Register &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::Register &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      move_record.destination_register.has_value()) {
    if (auto cast_publication =
            make_immediate_cast_call_argument_publication_instruction(
                context, *source_home, *move_record.destination_register, instruction_index)) {
      return cast_publication;
    }
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::Register &&
      source_home->register_name.has_value() &&
      argument != nullptr &&
      (argument->source_encoding == prepare::PreparedStorageEncodingKind::Register ||
       argument->source_encoding == prepare::PreparedStorageEncodingKind::ComputedAddress ||
       argument->source_encoding == prepare::PreparedStorageEncodingKind::SymbolAddress) &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr &&
      is_aarch64_byval_register_lane_move(move) &&
      (argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Gpr) &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register)) {
    const auto lane_size = selected_byval_lane_extent_bytes(*argument);
    if (!lane_size.has_value() || *lane_size == 0 || *lane_size > 16) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument publication requires a small ABI byval size");
      return std::nullopt;
    }
    if (argument->source_register_name.has_value() &&
        *argument->source_register_name != *source_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument source register disagrees with prepared value home");
      return std::nullopt;
    }
    auto source = make_byval_register_lane_prepared_source(
        context, *argument, *source_home, *lane_size, call_plan.instruction_index);
    const auto destination_register_placement =
        move.destination_register_placement.has_value()
            ? move.destination_register_placement
            : argument->destination_register_placement;
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : move.destination_register_name;
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        std::max(move.destination_contiguous_width,
                 argument->destination_contiguous_width),
        !move.destination_occupied_register_names.empty()
            ? move.destination_occupied_register_names
            : argument->destination_occupied_register_names,
        abi::RegisterView::X,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument publication requires prepared source bytes and destination register");
      return std::nullopt;
    }
    move_record.source_register.reset();
    move_record.source_memory = *source;
    move_record.destination_register = *destination;
    move_record.move.reason = "call_arg_byval_aggregate_register_lanes";
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr &&
      is_aarch64_byval_register_lane_move(move) &&
      (argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Gpr) &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register)) {
    const auto lane_size = selected_byval_lane_extent_bytes(*argument);
    if (!lane_size.has_value() || *lane_size == 0 || *lane_size > 16) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument publication requires a small ABI byval size");
      return std::nullopt;
    }
    const auto destination_register_placement =
        move.destination_register_placement.has_value()
            ? move.destination_register_placement
            : argument->destination_register_placement;
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : move.destination_register_name;
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        std::max(move.destination_contiguous_width,
                 argument->destination_contiguous_width),
        !move.destination_occupied_register_names.empty()
            ? move.destination_occupied_register_names
            : argument->destination_occupied_register_names,
        abi::RegisterView::X,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      return std::nullopt;
    }
    auto source = make_byval_register_lane_prepared_source(
        context, *argument, *source_home, *lane_size, call_plan.instruction_index);
    if (!source.has_value() && has_selected_byval_register_lane_source(*argument)) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument publication requires complete prepared selected source bytes");
      return std::nullopt;
    }
    if (!source.has_value() || source->size_bytes == 0 || source->size_bytes > 16) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument publication requires a small prepared frame-slot source");
      return std::nullopt;
    }
    move_record.source_register.reset();
    move_record.source_memory = *source;
    move_record.destination_register = *destination;
    move_record.move.reason = "call_arg_byval_aggregate_register_lanes";
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      !is_aarch64_byval_register_lane_move(move) &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->source_register_bank == prepare::PreparedRegisterBank::Fpr &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Fpr &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      scalar_fp_view_from_register_name(binding->destination_register_name).has_value()) {
    auto source = argument->source_selection.has_value()
                      ? make_selected_call_argument_source(
                            context,
                            *argument,
                            source_home,
                            *argument->source_selection,
                            prepare::PreparedCallArgumentSourceSelectionKind::
                                FrameSlotValue,
                            instruction_index)
                      : std::optional<MemoryOperand>{};
    const auto destination_register_placement =
        binding->destination_register_placement.has_value()
            ? binding->destination_register_placement
            : (move.destination_register_placement.has_value()
                   ? move.destination_register_placement
                   : argument->destination_register_placement);
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : (binding->destination_register_name.has_value()
                   ? binding->destination_register_name
                   : move.destination_register_name);
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        binding->destination_contiguous_width,
        binding->destination_occupied_register_names,
        scalar_fp_view_from_register_name(binding->destination_register_name),
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 scalar FPR frame-slot call-argument move requires a prepared load access for the source value");
      return std::nullopt;
    }
    move_record.source_memory = *source;
    move_record.destination_register = *destination;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      !is_aarch64_byval_register_lane_move(move) &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      source_home->size_bytes == std::optional<std::size_t>{16} &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Vreg &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register) {
    auto source = argument->source_selection.has_value()
                      ? make_selected_call_argument_source(
                            context,
                            *argument,
                            source_home,
                            *argument->source_selection,
                            prepare::PreparedCallArgumentSourceSelectionKind::
                                FrameSlotValue,
                            instruction_index)
                      : std::optional<MemoryOperand>{};
    const auto destination_register_placement =
        binding->destination_register_placement.has_value()
            ? binding->destination_register_placement
            : (move.destination_register_placement.has_value()
                   ? move.destination_register_placement
                   : argument->destination_register_placement);
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : (binding->destination_register_name.has_value()
                   ? binding->destination_register_name
                   : move.destination_register_name);
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        binding->destination_contiguous_width,
        binding->destination_occupied_register_names,
        abi::RegisterView::Q,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 binary128 frame-slot call-argument move requires a prepared frame-slot source and q-register destination");
      return std::nullopt;
    }
    move_record.source_memory = *source;
    move_record.destination_register = *destination;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      !is_aarch64_byval_register_lane_move(move) &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      (argument->source_register_bank == prepare::PreparedRegisterBank::Gpr ||
       argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress) &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register)) {
    std::optional<MemoryOperand> address_source;
    std::optional<MemoryOperand> source;
    if (argument->source_selection.has_value()) {
      switch (argument->source_selection->kind) {
        case prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress:
          address_source = make_sret_memory_return_address_source(
              context, call_plan, *argument, instruction_index);
          if (!address_source.has_value()) {
            address_source = make_selected_call_argument_source(
                context,
                *argument,
                source_home,
                *argument->source_selection,
                prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress,
                instruction_index);
          }
          break;
        case prepare::PreparedCallArgumentSourceSelectionKind::
            LocalFrameAddressMaterialization:
          address_source = make_selected_call_argument_source(
              context,
              *argument,
              source_home,
              *argument->source_selection,
              prepare::PreparedCallArgumentSourceSelectionKind::
                  LocalFrameAddressMaterialization,
              instruction_index);
          break;
        case prepare::PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane:
          if (const auto byval_size = selected_byval_lane_extent_bytes(*argument);
              byval_size.has_value()) {
            address_source = make_byval_register_lane_prepared_source(
                context, *argument, *source_home, *byval_size, call_plan.instruction_index);
          }
          if (!address_source.has_value()) {
            append_call_diagnostic(
                diagnostics,
                module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
                context,
                instruction_index,
                "AArch64 indirect byval call-argument publication requires complete prepared selected source bytes");
            return std::nullopt;
          }
          break;
        case prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue:
        case prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation:
          address_source = make_selected_call_argument_source(
              context,
              *argument,
              source_home,
              *argument->source_selection,
              prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress,
              instruction_index);
          break;
        case prepare::PreparedCallArgumentSourceSelectionKind::None:
          break;
      }
    } else {
      address_source = make_sret_memory_return_address_source(
          context, call_plan, *argument, instruction_index);
    }
    if (!address_source.has_value() &&
        argument->source_selection.has_value() &&
        (argument->source_selection->kind ==
             prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue ||
         argument->source_selection->kind ==
             prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation)) {
      source = make_selected_call_argument_source(
          context,
          *argument,
          source_home,
          *argument->source_selection,
          prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue,
          instruction_index);
    } else {
      source = address_source;
    }
    const auto destination_register_placement =
        binding != nullptr && binding->destination_register_placement.has_value()
            ? binding->destination_register_placement
            : (move.destination_register_placement.has_value()
                   ? move.destination_register_placement
                   : argument->destination_register_placement);
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : (binding != nullptr && binding->destination_register_name.has_value()
                   ? binding->destination_register_name
                   : move.destination_register_name);
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        binding != nullptr ? binding->destination_contiguous_width
                           : move.destination_contiguous_width,
        binding != nullptr ? binding->destination_occupied_register_names
                           : move.destination_occupied_register_names,
        address_source.has_value()
            ? std::optional<abi::RegisterView>{abi::RegisterView::X}
            : source.has_value()
            ? scalar_integer_register_view_from_size(source->size_bytes)
            : std::nullopt,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 frame-slot call-argument move requires a prepared load access for the source value");
      return std::nullopt;
    }
    move_record.source_memory = *source;
    move_record.source_memory_materializes_address = address_source.has_value();
    move_record.destination_register = *destination;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      (argument->value_bank == prepare::PreparedRegisterBank::Gpr ||
       argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr) &&
      is_aarch64_byval_register_lane_move(move) &&
      (!argument->source_register_bank.has_value() ||
       argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Gpr) &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot ||
       move.destination_stack_offset_bytes.has_value())) {
    const auto lane_size = selected_byval_lane_extent_bytes(*argument);
    if (!lane_size.has_value() || *lane_size == 0 || *lane_size > 16) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack-lane call-argument publication requires a small ABI byval size");
      return std::nullopt;
    }
    auto source = make_byval_register_lane_prepared_source(
        context, *argument, *source_home, *lane_size, call_plan.instruction_index);
    if (!source.has_value() && has_selected_byval_register_lane_source(*argument)) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument publication requires complete prepared selected source bytes");
      return std::nullopt;
    }
    if (!source.has_value() || source->size_bytes == 0 || source->size_bytes > 16) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack-lane call-argument publication requires prepared source bytes");
      return std::nullopt;
    }
    const auto destination = make_stack_call_argument_destination(
        context, *argument, *source_home, move, binding, *source, instruction_index);
    if (!destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack-lane call-argument publication requires a prepared destination stack offset");
      return std::nullopt;
    }
    auto lowered = make_byval_register_lane_stack_publication_instruction(
        context, instruction_index, *source, *destination);
    if (!lowered.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 aggregate stack-lane call-argument publication requires printable prepared source and destination stack slots");
      return std::nullopt;
    }
    return lowered;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      argument != nullptr &&
      (argument->source_encoding == prepare::PreparedStorageEncodingKind::Register ||
       argument->source_encoding == prepare::PreparedStorageEncodingKind::ComputedAddress) &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->value_bank == prepare::PreparedRegisterBank::AggregateAddress &&
      (!argument->source_register_bank.has_value() ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Gpr ||
       argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress) &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot)) {
    const auto* source_register_home = source_home;
    if (source_home->kind == prepare::PreparedValueHomeKind::PointerBasePlusOffset &&
        argument->source_base_value_id.has_value() &&
        context.function.value_locations != nullptr) {
      source_register_home = prepare::find_prepared_value_home(
          *context.function.value_locations, *argument->source_base_value_id);
    }
    if (source_register_home == nullptr ||
        source_register_home->kind != prepare::PreparedValueHomeKind::Register ||
        !source_register_home->register_name.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack call-argument copy requires a prepared aggregate address register");
      return std::nullopt;
    }
    if (argument->source_register_name.has_value() &&
        *argument->source_register_name != *source_register_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack call-argument source register disagrees with prepared value home");
      return std::nullopt;
    }
    const auto size_bytes = source_home->size_bytes.has_value()
                                ? source_home->size_bytes
                                : argument->destination_stack_size_bytes;
    if (!size_bytes.has_value() || *size_bytes == 0) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack call-argument copy requires a prepared aggregate size");
      return std::nullopt;
    }
    auto source_register = make_register_operand_from_prepared_authority(
        source_register_home->register_name,
        argument->source_register_placement,
        argument->source_register_bank.has_value()
            ? argument->source_register_bank
            : std::optional<prepare::PreparedRegisterBank>{
                  prepare::PreparedRegisterBank::Gpr},
        RegisterOperandRole::CallAbi,
        source_register_home->value_id,
        source_register_home->value_name,
        1,
        {},
        abi::RegisterView::X,
        diagnostics,
        context,
        instruction_index);
    if (!source_register.has_value()) {
      return std::nullopt;
    }
    const auto source_byte_offset = source_home->pointer_byte_delta.value_or(0);
    const auto source = make_aggregate_call_argument_source(
        context,
        *argument,
        *source_home,
        *source_register,
        *size_bytes,
        source_byte_offset,
        instruction_index);
    if (!source.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack call-argument copy requires a prepared aggregate address source");
      return std::nullopt;
    }
    const auto destination = make_stack_call_argument_destination(
        context, *argument, *source_home, move, binding, *source, instruction_index);
    if (!destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack call-argument copy requires a prepared destination stack offset");
      return std::nullopt;
    }
    return make_aggregate_stack_copy_instruction(
        context, instruction_index, *source, *destination);
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      source_home->size_bytes == std::optional<std::size_t>{16} &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->value_bank == prepare::PreparedRegisterBank::Vreg &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot)) {
    const auto source =
        argument->source_selection.has_value()
            ? make_selected_call_argument_source(
                  context,
                  *argument,
                  source_home,
                  *argument->source_selection,
                  prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue,
                  instruction_index)
            : std::optional<MemoryOperand>{};
    if (!source.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 binary128 stack call-argument move requires a prepared frame-slot source");
      return std::nullopt;
    }
    const auto destination = make_stack_call_argument_destination(
        context, *argument, *source_home, move, binding, *source, instruction_index);
    if (!destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 binary128 stack call-argument move requires a prepared destination stack offset");
      return std::nullopt;
    }
    if (f128_carriers == nullptr) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 binary128 stack call-argument move requires prepared f128 carrier facts");
      return std::nullopt;
    }
    auto prepared = make_prepared_f128_carrier_transport_record(
        *f128_carriers,
        source_home->value_name,
        F128TransportKind::StoreToMemory,
        *destination);
    if (!prepared.record.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 binary128 stack call-argument move requires a complete source carrier");
      return std::nullopt;
    }
    return make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_f128_transport_instruction(std::move(*prepared.record)));
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot)) {
    const auto source =
        argument->source_selection.has_value()
            ? make_selected_call_argument_source(
                  context,
                  *argument,
                  source_home,
                  *argument->source_selection,
                  prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue,
                  instruction_index)
            : std::optional<MemoryOperand>{};
    if (!source.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 stack call-argument move requires a prepared frame-slot source");
      return std::nullopt;
    }
    const auto value_type = scalar_integer_type_from_size(source->size_bytes);
    if (!value_type.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 stack call-argument move lowering requires a 1, 4, or 8 byte prepared stack slot");
      return std::nullopt;
    }
    const auto destination = make_stack_call_argument_destination(
        context, *argument, *source_home, move, binding, *source, instruction_index);
    if (!destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 stack call-argument move requires a prepared destination stack offset");
      return std::nullopt;
    }
    return make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_memory_instruction(MemoryInstructionRecord{
            .memory_kind = MemoryInstructionKind::Store,
            .address = *destination,
            .value = make_memory_operand(*source),
            .value_type = *value_type,
        }));
  }

  if (selected_f128_constant_argument_move) {
    auto destination = make_register_operand_from_prepared_authority(
        binding->destination_register_name,
        binding->destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_f128_carrier->value_name,
        binding->destination_contiguous_width,
        binding->destination_occupied_register_names,
        abi::RegisterView::Q,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      return std::nullopt;
    }
    move_record.destination_register = *destination;
    move_record.source_f128_carrier = source_f128_carrier;
    move_record.source_f128_constant_payload = source_f128_carrier->constant_payload;
  }

  const bool symbol_address_argument_materialized_at_call_site =
      bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::SymbolAddress &&
      source_home != nullptr &&
      source_home->kind != prepare::PreparedValueHomeKind::Register &&
      context.function.prepared != nullptr &&
      context.function.control_flow != nullptr &&
      context.control_flow_block != nullptr &&
      [&]() {
        const auto* addressing = prepare::find_prepared_addressing(
            *context.function.prepared, context.function.control_flow->function_name);
        if (addressing == nullptr) {
          return false;
        }
        for (const auto& materialization : addressing->address_materializations) {
          if (materialization.block_label == context.control_flow_block->block_label &&
              materialization.inst_index <= instruction_index &&
              materialization.result_value_name == source_home->value_name) {
            return true;
          }
        }
        return false;
      }();
  if (symbol_address_argument_materialized_at_call_site) {
    return std::nullopt;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      !selected_f128_constant_argument_move &&
      ((!move_record.source_register.has_value() &&
        !move_record.source_memory.has_value()) ||
       !move_record.destination_register.has_value())) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        "AArch64 call-argument move lowering requires selected prepared register source and destination");
    return std::nullopt;
  }

  return make_call_boundary_machine_instruction(
      context,
      instruction_index,
      make_call_boundary_move_instruction(std::move(move_record)));
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_before_call_immediate_binding(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedAbiBinding& binding,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (bundle.phase != prepare::PreparedMovePhase::BeforeCall ||
      binding.destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      !binding.destination_abi_index.has_value()) {
    return std::nullopt;
  }

  const auto* argument = [&]() -> const prepare::PreparedCallArgumentPlan* {
    for (const auto& candidate : call_plan.arguments) {
      if (candidate.arg_index == *binding.destination_abi_index &&
          candidate.source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
          candidate.source_literal.has_value() &&
          (binding.destination_storage_kind ==
               prepare::PreparedMoveStorageKind::StackSlot ||
           candidate.destination_register_bank == prepare::PreparedRegisterBank::Gpr ||
           candidate.destination_register_bank == prepare::PreparedRegisterBank::Fpr)) {
        return &candidate;
      }
    }
    return nullptr;
  }();
  if (argument == nullptr) {
    return std::nullopt;
  }

  const auto source_immediate =
      make_scalar_call_argument_immediate(*argument->source_literal,
                                          argument->source_value_id);
  const auto expected_view =
      argument->destination_register_bank == prepare::PreparedRegisterBank::Fpr
          ? scalar_fp_register_view(argument->source_literal->type)
          : scalar_integer_register_view(argument->source_literal->type);
  if (!source_immediate.has_value() || !expected_view.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        "AArch64 immediate call-argument move requires a scalar integer or FP literal");
    return std::nullopt;
  }

  if (binding.destination_storage_kind ==
      prepare::PreparedMoveStorageKind::StackSlot) {
    if (argument->destination_register_bank == prepare::PreparedRegisterBank::Fpr) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 immediate stack call-argument move requires a scalar integer literal");
      return std::nullopt;
    }
    if (!binding.destination_stack_offset_bytes.has_value()) {
      return std::nullopt;
    }
    const auto value_type = scalar_integer_type_from_size(
        scalar_size_from_register_view(expected_view));
    if (!value_type.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 immediate stack call-argument move requires a 4 or 8 byte scalar integer literal");
      return std::nullopt;
    }
    MemoryOperand destination{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_index = instruction_index,
        .stored_value_id = argument->source_value_id,
        .stored_value_name = c4c::kInvalidValueName,
        .base_kind = MemoryBaseKind::Register,
        .base_register = RegisterOperand{
            .reg = outgoing_stack_argument_base_register(),
            .role = RegisterOperandRole::Physical,
            .expected_view = abi::RegisterView::X,
        },
        .byte_offset =
            static_cast<std::int64_t>(*binding.destination_stack_offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = scalar_size_from_register_view(expected_view),
        .align_bytes = scalar_size_from_register_view(expected_view),
        .can_use_base_plus_offset = true,
    };
    return make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_memory_instruction(MemoryInstructionRecord{
            .memory_kind = MemoryInstructionKind::Store,
            .address = std::move(destination),
            .value = make_immediate_operand(*source_immediate),
            .value_type = *value_type,
        }));
  }

  if (binding.destination_storage_kind !=
          prepare::PreparedMoveStorageKind::Register ||
      !binding.destination_register_name.has_value()) {
    return std::nullopt;
  }

  auto destination = make_register_operand_from_prepared_authority(
      binding.destination_register_placement.has_value()
          ? std::optional<std::string>{}
          : binding.destination_register_name,
      binding.destination_register_placement.has_value()
          ? binding.destination_register_placement
          : argument->destination_register_placement,
      argument->destination_register_bank,
      RegisterOperandRole::CallAbi,
      argument->source_value_id,
      c4c::kInvalidValueName,
      binding.destination_contiguous_width,
      binding.destination_occupied_register_names.empty()
          ? argument->destination_occupied_register_names
          : binding.destination_occupied_register_names,
      expected_view,
      diagnostics,
      context,
      instruction_index);
  if (!destination.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedMoveResolution synthetic_move{
      .from_value_id = argument->source_value_id.value_or(prepare::PreparedValueId{0}),
      .to_value_id = argument->source_value_id.value_or(prepare::PreparedValueId{0}),
      .destination_kind = binding.destination_kind,
      .destination_storage_kind = binding.destination_storage_kind,
      .destination_abi_index = binding.destination_abi_index,
      .destination_register_name = binding.destination_register_name,
      .destination_contiguous_width = binding.destination_contiguous_width,
      .destination_occupied_register_names = binding.destination_occupied_register_names,
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .reason = "call_arg_immediate_to_register",
      .destination_register_placement = binding.destination_register_placement,
  };

  CallBoundaryMoveInstructionRecord move_record{
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .phase = bundle.phase,
      .authority_kind = bundle.authority_kind,
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .source_parallel_copy_predecessor_label =
          bundle.source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label =
          bundle.source_parallel_copy_successor_label,
      .move = std::move(synthetic_move),
      .source_immediate = source_immediate,
      .destination_register = *destination,
      .source_bundle = &bundle,
  };
  return make_call_boundary_machine_instruction(
      context,
      instruction_index,
      make_call_boundary_move_instruction(std::move(move_record)));
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_after_call_move(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* destination_home =
      context.function.value_locations == nullptr
          ? nullptr
          : find_value_home(context, move.to_value_id);
  const auto classification =
      prepare::classify_prepared_call_boundary_move(call_plan, bundle, move);
  const auto* result_plan = classification.result_plan;
  const auto* binding = classification.abi_binding;
  const auto* f128_carriers =
      context.function.prepared == nullptr
          ? nullptr
          : prepare::find_prepared_f128_carriers(
                *context.function.prepared,
                context.function.control_flow != nullptr
                    ? context.function.control_flow->function_name
                    : c4c::kInvalidFunctionName);
  const auto* destination_f128_carrier =
      f128_carriers != nullptr && destination_home != nullptr
          ? prepare::find_prepared_f128_carrier(*f128_carriers, destination_home->value_name)
          : nullptr;

  CallBoundaryMoveInstructionRecord move_record{
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .phase = bundle.phase,
      .authority_kind = bundle.authority_kind,
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .source_parallel_copy_predecessor_label =
          bundle.source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label =
          bundle.source_parallel_copy_successor_label,
      .move = move,
      .source_bundle = &bundle,
      .source_move = &move,
  };

  const bool selected_gpr_result_move =
      result_plan != nullptr &&
      result_plan->source_register_bank == prepare::PreparedRegisterBank::Gpr &&
      result_plan->destination_register_bank == prepare::PreparedRegisterBank::Gpr;
  const bool selected_scalar_fpr_result_move =
      result_plan != nullptr &&
      result_plan->source_register_bank == prepare::PreparedRegisterBank::Fpr &&
      result_plan->destination_register_bank == prepare::PreparedRegisterBank::Fpr &&
      result_plan->source_contiguous_width == 1 &&
      result_plan->destination_contiguous_width == 1;
  const bool selected_f128_result_move =
      result_plan != nullptr &&
      result_plan->source_register_bank == prepare::PreparedRegisterBank::Vreg &&
      result_plan->destination_register_bank == prepare::PreparedRegisterBank::Vreg &&
      complete_full_width_f128_carrier(destination_f128_carrier);

  if (bundle.phase == prepare::PreparedMovePhase::AfterCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallResultAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      !move.destination_abi_index.has_value() &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      destination_home != nullptr &&
      destination_home->kind == prepare::PreparedValueHomeKind::Register &&
      destination_home->register_name.has_value() &&
      result_plan != nullptr &&
      result_plan->instruction_index == instruction_index &&
      result_plan->destination_value_id == std::optional<prepare::PreparedValueId>{move.to_value_id} &&
      result_plan->source_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      result_plan->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      result_plan->source_register_name.has_value() &&
      result_plan->destination_register_name.has_value() &&
      (selected_gpr_result_move || selected_scalar_fpr_result_move ||
       selected_f128_result_move) &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      binding->destination_register_name.has_value()) {
    if (*result_plan->source_register_name != *binding->destination_register_name ||
        *result_plan->source_register_name != *move.destination_register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 call-result move source register disagrees with prepared ABI binding");
      return std::nullopt;
    }
    if (*result_plan->destination_register_name != *destination_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 call-result move destination register disagrees with prepared value home");
      return std::nullopt;
    }
    if (selected_f128_result_move &&
        destination_f128_carrier->register_name != destination_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 binary128 call-result move destination register disagrees with prepared f128 carrier");
      return std::nullopt;
    }
    const auto expected_view =
        selected_f128_result_move ? std::optional<abi::RegisterView>{abi::RegisterView::Q}
        : selected_scalar_fpr_result_move
            ? scalar_fp_view_from_register_name(binding->destination_register_name)
        : std::nullopt;
    auto source = make_register_operand_from_prepared_authority(
        binding->destination_register_name,
        result_plan->source_register_placement.has_value()
            ? result_plan->source_register_placement
            : binding->destination_register_placement,
        result_plan->source_register_bank,
        RegisterOperandRole::CallAbi,
        move.from_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.from_value_id}
                                : std::nullopt,
        destination_home->value_name,
        result_plan->source_contiguous_width,
        result_plan->source_occupied_register_names.empty()
            ? binding->destination_occupied_register_names
            : result_plan->source_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    auto destination = make_register_operand_from_prepared_authority(
        selected_scalar_fpr_result_move &&
                result_plan->destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : destination_home->register_name,
        result_plan->destination_register_placement,
        result_plan->destination_register_bank,
        RegisterOperandRole::CallAbi,
        destination_home->value_id,
        destination_home->value_name,
        result_plan->destination_contiguous_width,
        result_plan->destination_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      return std::nullopt;
    }
    move_record.source_register = *source;
    move_record.destination_register = *destination;
    move_record.destination_f128_carrier =
        selected_f128_result_move ? destination_f128_carrier : nullptr;
  }

  if (bundle.phase == prepare::PreparedMovePhase::AfterCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallResultAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      result_plan != nullptr &&
      result_plan->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot) {
    if (destination_home == nullptr ||
        destination_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
        !destination_home->offset_bytes.has_value() ||
        result_plan->instruction_index != instruction_index ||
        result_plan->destination_value_id !=
            std::optional<prepare::PreparedValueId>{move.to_value_id} ||
        result_plan->source_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        !result_plan->source_register_name.has_value() ||
        result_plan->source_register_bank != prepare::PreparedRegisterBank::Gpr) {
      return std::nullopt;
    }
    if ((binding != nullptr &&
         binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
         binding->destination_register_name.has_value() &&
         *result_plan->source_register_name != *binding->destination_register_name) ||
        (move.destination_register_name.has_value() &&
         *result_plan->source_register_name != *move.destination_register_name)) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 stack call-result publication source register disagrees with prepared ABI binding");
      return std::nullopt;
    }
    const auto width_bytes = destination_home->size_bytes.value_or(
        scalar_size_from_register_view(
            scalar_view_from_register_name(result_plan->source_register_name)));
    const auto expected_view = scalar_integer_register_view_from_size(width_bytes);
    const auto value_type = scalar_integer_type_from_size(width_bytes);
    if (!expected_view.has_value() || !value_type.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 stack call-result publication requires a 1, 2, 4, or 8 byte scalar GPR result");
      return std::nullopt;
    }
    const auto source_register_name =
        register_name_with_expected_view(result_plan->source_register_name, expected_view);
    auto source = make_register_operand_from_prepared_authority(
        source_register_name,
        result_plan->source_register_placement,
        result_plan->source_register_bank,
        RegisterOperandRole::CallAbi,
        move.from_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.from_value_id}
                                : std::nullopt,
        destination_home->value_name,
        result_plan->source_contiguous_width,
        result_plan->source_occupied_register_names.empty()
            ? (binding != nullptr ? binding->destination_occupied_register_names
                                  : move.destination_occupied_register_names)
            : result_plan->source_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value()) {
      return std::nullopt;
    }
    MemoryOperand destination{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_index = instruction_index,
        .stored_value_id = destination_home->value_id,
        .stored_value_name = destination_home->value_name,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = destination_home->slot_id,
        .byte_offset = static_cast<std::int64_t>(*destination_home->offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = width_bytes,
        .align_bytes = destination_home->align_bytes.value_or(width_bytes),
        .can_use_base_plus_offset = true,
    };
    return make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_memory_instruction(MemoryInstructionRecord{
            .memory_kind = MemoryInstructionKind::Store,
            .address = std::move(destination),
            .value = make_register_operand(*source),
            .value_type = *value_type,
        }));
  }

  if (bundle.phase == prepare::PreparedMovePhase::AfterCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallResultAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      destination_home != nullptr &&
      destination_home->kind == prepare::PreparedValueHomeKind::Register &&
      destination_home->register_name.has_value() &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      binding->destination_register_name.has_value() &&
      binding->destination_register_placement.has_value() &&
      binding->destination_register_placement->bank == prepare::PreparedRegisterBank::Fpr &&
      (!move_record.source_register.has_value() ||
       !move_record.destination_register.has_value())) {
    const auto expected_view =
        scalar_fp_view_from_register_name(binding->destination_register_name);
    auto source = make_register_operand_from_prepared_authority(
        binding->destination_register_name,
        binding->destination_register_placement,
        prepare::PreparedRegisterBank::Fpr,
        RegisterOperandRole::CallAbi,
        move.from_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.from_value_id}
                                : std::nullopt,
        destination_home->value_name,
        1,
        binding->destination_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    auto destination = make_register_operand_from_prepared_authority(
        destination_home->register_name,
        std::nullopt,
        prepare::PreparedRegisterBank::Fpr,
        RegisterOperandRole::CallAbi,
        destination_home->value_id,
        destination_home->value_name,
        1,
        {},
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      return std::nullopt;
    }
    move_record.source_register = *source;
    move_record.destination_register = *destination;
  }

  if (bundle.phase == prepare::PreparedMovePhase::AfterCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallResultAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      (!move_record.source_register.has_value() ||
       !move_record.destination_register.has_value())) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        "AArch64 call-result move lowering requires selected prepared register source and destination");
    return std::nullopt;
  }

  return make_call_boundary_machine_instruction(
      context,
      instruction_index,
      make_call_boundary_move_instruction(std::move(move_record)));
}

[[nodiscard]] bool move_source_aliases_destination(
    const module::MachineInstruction& source_instruction,
    const module::MachineInstruction& destination_instruction) {
  const auto* source =
      std::get_if<CallBoundaryMoveInstructionRecord>(&source_instruction.target.payload);
  const auto* destination =
      std::get_if<CallBoundaryMoveInstructionRecord>(&destination_instruction.target.payload);
  return source != nullptr &&
         destination != nullptr &&
         destination->move.reason != "callee_saved_preservation_home_population" &&
         source->source_register.has_value() &&
         destination->destination_register.has_value() &&
         registers_alias(*source->source_register, *destination->destination_register);
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_callee_saved_preservation_home_republication_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedCallBoundaryEffectPlan& effect,
    prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index,
    std::string reason,
    module::ModuleLoweringDiagnostics& diagnostics) {
  // Emission-only: prepared lookups select eligibility and block-entry reachability.
  const auto& storage = effect.source;
  const auto& value = effect.destination;
  const auto value_id = effect_value_id(value, storage);
  const auto value_name = effect_value_name(value, storage);
  if (context.function.control_flow == nullptr ||
      effect.effect_kind !=
          prepare::PreparedCallBoundaryEffectKind::PreservationRepublication ||
      effect.preservation_route !=
          prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      !value_id.has_value() ||
      value_name == c4c::kInvalidValueName ||
      storage.storage_kind != prepare::PreparedMoveStorageKind::Register ||
      !storage.register_name.has_value() ||
      !storage.register_bank.has_value() ||
      value.storage_kind != prepare::PreparedMoveStorageKind::Register ||
      !value.register_name.has_value() ||
      !value.register_bank.has_value()) {
    return std::nullopt;
  }

  const auto expected_view =
      value.stack_size_bytes.has_value()
          ? scalar_integer_register_view_from_size(*value.stack_size_bytes)
      : storage.stack_size_bytes.has_value()
          ? scalar_integer_register_view_from_size(*storage.stack_size_bytes)
      : scalar_view_from_register_name(value.register_name).has_value()
          ? scalar_view_from_register_name(value.register_name)
          : scalar_view_from_register_name(storage.register_name);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }

  auto source = make_register_operand_from_prepared_authority(
      storage.register_name,
      storage.register_placement,
      storage.register_bank,
      RegisterOperandRole::StoragePlan,
      *value_id,
      value_name,
      storage.contiguous_width,
      storage.occupied_register_names,
      expected_view,
      diagnostics,
      context,
      instruction_index);
  auto destination_register_name =
      register_name_with_expected_view(value.register_name, expected_view);
  if (!destination_register_name.has_value()) {
    destination_register_name = value.register_name;
  }
  auto destination = make_register_operand_from_prepared_authority(
      destination_register_name,
      value.register_placement,
      value.register_bank,
      RegisterOperandRole::StoragePlan,
      *value_id,
      value_name,
      value.contiguous_width,
      value.occupied_register_names,
      expected_view,
      diagnostics,
      context,
      instruction_index);
  if (!source.has_value() || !destination.has_value() ||
      (source->reg.bank == destination->reg.bank &&
       source->reg.index == destination->reg.index)) {
    return std::nullopt;
  }

  prepare::PreparedMoveResolution synthetic_move{
      .from_value_id = *value_id,
      .to_value_id = *value_id,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = destination_register_name,
      .destination_contiguous_width = value.contiguous_width,
      .destination_occupied_register_names = value.occupied_register_names,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .reason = std::move(reason),
      .destination_register_placement = value.register_placement,
  };
  CallBoundaryMoveInstructionRecord move_record{
      .function_name = context.function.control_flow->function_name,
      .phase = phase,
      .authority_kind = bundle.authority_kind,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .move = synthetic_move,
      .source_register = *source,
      .destination_register = *destination,
      .source_bundle = &bundle,
      .source_move = &synthetic_move,
  };
  return make_call_boundary_machine_instruction(
      context,
      instruction_index,
      make_call_boundary_move_instruction(std::move(move_record)));
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_callee_saved_preservation_home_population(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedCallBoundaryEffectPlan& effect,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto& value = effect.source;
  const auto& storage = effect.destination;
  const auto value_id = effect_value_id(value, storage);
  const auto value_name = effect_value_name(value, storage);
  if (context.function.control_flow == nullptr ||
      effect.effect_kind !=
          prepare::PreparedCallBoundaryEffectKind::PreservationHomePopulation ||
      effect.preservation_route !=
          prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      !value_id.has_value() ||
      value_name == c4c::kInvalidValueName ||
      storage.storage_kind != prepare::PreparedMoveStorageKind::Register ||
      !storage.register_name.has_value() ||
      !storage.register_bank.has_value()) {
    return std::nullopt;
  }
  const auto expected_view =
      value.stack_size_bytes.has_value()
          ? scalar_integer_register_view_from_size(*value.stack_size_bytes)
      : value.register_name.has_value()
          ? scalar_view_from_register_name(value.register_name)
      : storage.stack_size_bytes.has_value()
          ? scalar_integer_register_view_from_size(*storage.stack_size_bytes)
          : scalar_view_from_register_name(storage.register_name);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }

  auto destination = make_register_operand_from_prepared_authority(
      storage.register_name,
      storage.register_placement,
      storage.register_bank,
      RegisterOperandRole::CallAbi,
      *value_id,
      value_name,
      storage.contiguous_width,
      storage.occupied_register_names,
      expected_view,
      diagnostics,
      context,
      instruction_index);
  if (!destination.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedMoveResolution synthetic_move{
      .from_value_id = *value_id,
      .to_value_id = *value_id,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = storage.register_name,
      .destination_contiguous_width = storage.contiguous_width,
      .destination_occupied_register_names = storage.occupied_register_names,
      .block_index = call_plan.block_index,
      .instruction_index = call_plan.instruction_index,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .reason = "callee_saved_preservation_home_population",
      .destination_register_placement = storage.register_placement,
  };
  CallBoundaryMoveInstructionRecord move_record{
      .function_name = context.function.control_flow->function_name,
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .authority_kind = bundle.authority_kind,
      .block_index = call_plan.block_index,
      .instruction_index = call_plan.instruction_index,
      .move = synthetic_move,
      .destination_register = *destination,
      .source_bundle = &bundle,
      .source_move = &synthetic_move,
  };

  bool selected_source = false;
  if (value.storage_kind == prepare::PreparedMoveStorageKind::Register &&
      value.register_name.has_value() &&
      value.register_bank.has_value()) {
    auto source = make_register_operand_from_prepared_authority(
        value.register_name,
        value.register_placement,
        value.register_bank,
        RegisterOperandRole::StoragePlan,
        *value_id,
        value_name,
        value.contiguous_width,
        value.occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value()) {
      return std::nullopt;
    }
    if (source->reg.bank != destination->reg.bank ||
        source->reg.index != destination->reg.index) {
      move_record.source_register = *source;
      selected_source = true;
    }
  }
  if (!selected_source) {
    if (auto source = make_frame_slot_memory_from_endpoint(
            context, value, *value_id, value_name, instruction_index)) {
      move_record.source_memory = *source;
      selected_source = true;
    }
  }
  if (!selected_source) {
    return std::nullopt;
  }

  return make_call_boundary_machine_instruction(
      context,
      instruction_index,
      make_call_boundary_move_instruction(std::move(move_record)));
}

}  // namespace

[[nodiscard]] std::optional<MemoryOperand> make_selected_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome* source_home,
    const prepare::PreparedCallArgumentSourceSelection& selection,
    prepare::PreparedCallArgumentSourceSelectionKind expected_kind,
    std::size_t instruction_index) {
  if (selection.kind != expected_kind) {
    return std::nullopt;
  }
  switch (expected_kind) {
    case prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue:
      if (source_home == nullptr) {
        return std::nullopt;
      }
      return make_selected_frame_slot_source(
          context,
          argument,
          source_home,
          selection,
          expected_kind,
          false,
          instruction_index);
    case prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress:
      return make_selected_frame_slot_source(
          context,
          argument,
          source_home,
          selection,
          expected_kind,
          true,
          instruction_index);
    case prepare::PreparedCallArgumentSourceSelectionKind::
        LocalFrameAddressMaterialization:
      if (source_home == nullptr) {
        return std::nullopt;
      }
      return make_selected_local_frame_address_source(
          context, argument, *source_home, selection, instruction_index);
    case prepare::PreparedCallArgumentSourceSelectionKind::None:
    case prepare::PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane:
    case prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] const prepare::PreparedMoveBundle* find_move_bundle(
    const module::BlockLoweringContext& context,
    prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index) {
  return prepare::find_indexed_prepared_move_bundle(context.function.move_bundle_lookups,
                                                    context.function.value_locations,
                                                    phase,
                                                    block_index,
                                                    instruction_index);
}

[[nodiscard]] bool call_boundary_move_reloads_materialized_address(
    const module::MachineInstruction& instruction,
    const std::vector<module::MachineInstruction>& materialized_addresses) {
  const auto* move =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move == nullptr ||
      move->phase != prepare::PreparedMovePhase::BeforeCall ||
      move->move.destination_kind !=
          prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      move->move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      !move->source_memory.has_value() ||
      !move->destination_register.has_value() ||
      (!move->source_memory->result_value_id.has_value() &&
       !move->source_memory->result_value_name.has_value())) {
    return false;
  }
  for (const auto& materialized : materialized_addresses) {
    const auto* address =
        std::get_if<AddressMaterializationRecord>(&materialized.target.payload);
    if (address == nullptr || !address->result_register.has_value() ||
        !registers_alias(*address->result_register, *move->destination_register)) {
      continue;
    }
    const bool same_value_id =
        move->source_memory->result_value_id.has_value() &&
        address->result_value_id == move->source_memory->result_value_id;
    const bool same_value_name =
        move->source_memory->result_value_name.has_value() &&
        address->result_value_name == *move->source_memory->result_value_name;
    if (same_value_id || same_value_name) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::vector<module::MachineInstruction>
order_before_call_moves_for_source_preservation(
    std::vector<module::MachineInstruction> moves) {
  std::vector<module::MachineInstruction> ordered;
  ordered.reserve(moves.size());
  while (!moves.empty()) {
    auto selected = moves.begin();
    for (auto candidate = moves.begin(); candidate != moves.end(); ++candidate) {
      const bool protects_live_source =
          std::any_of(moves.begin(), moves.end(), [&](const auto& other) {
            return &other != &*candidate &&
                   move_source_aliases_destination(*candidate, other);
          });
      if (protects_live_source) {
        selected = candidate;
        break;
      }
    }
    ordered.push_back(std::move(*selected));
    moves.erase(selected);
  }
  return ordered;
}

std::vector<module::MachineInstruction> lower_before_call_moves(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr) {
    return lowered;
  }
  const auto* bundle = find_move_bundle(context,
                                        prepare::PreparedMovePhase::BeforeCall,
                                        context.block_index,
                                        instruction_index);
  const prepare::PreparedMoveBundle synthetic_bundle{
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
  };
  if (bundle == nullptr) {
    bundle = &synthetic_bundle;
  }
  const std::size_t outgoing_bytes = outgoing_stack_argument_bytes(call_plan);
  if (outgoing_bytes > 0) {
    lowered.push_back(
        make_outgoing_stack_base_instruction(context, instruction_index, outgoing_bytes));
  }
  const auto boundary_effects =
      prepare::plan_prepared_call_boundary_effects(call_plan, bundle, nullptr);
  for (const auto& effect : boundary_effects) {
    if (effect.effect_kind !=
            prepare::PreparedCallBoundaryEffectKind::PreservationHomePopulation ||
        effect.phase != prepare::PreparedMovePhase::BeforeCall) {
      continue;
    }
    if (auto instruction = make_callee_saved_preservation_home_population(
            context, call_plan, *bundle, effect, instruction_index, diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  for (const auto& effect : boundary_effects) {
    if (effect.effect_kind != prepare::PreparedCallBoundaryEffectKind::ExplicitMove ||
        effect.phase != prepare::PreparedMovePhase::BeforeCall ||
        effect.order_index >= bundle->moves.size()) {
      continue;
    }
    const auto& move = bundle->moves[effect.order_index];
    if (auto instruction =
            lower_before_call_move(context, call_plan, *bundle, move, instruction_index, diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  for (const auto& binding : bundle->abi_bindings) {
    if (auto instruction =
            lower_before_call_immediate_binding(
                context, call_plan, *bundle, binding, instruction_index, diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  return lowered;
}

std::vector<module::MachineInstruction> lower_after_call_moves(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr) {
    return lowered;
  }
  const auto* bundle = find_move_bundle(context,
                                        prepare::PreparedMovePhase::AfterCall,
                                        context.block_index,
                                        instruction_index);
  const prepare::PreparedMoveBundle synthetic_bundle{
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .phase = prepare::PreparedMovePhase::BeforeInstruction,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
  };
  if (bundle != nullptr) {
    for (const auto& move : bundle->moves) {
      if (auto instruction =
              lower_after_call_move(context, call_plan, *bundle, move, instruction_index, diagnostics)) {
        lowered.push_back(std::move(*instruction));
      }
    }
  }

  const auto& republication_bundle =
      bundle != nullptr ? *bundle : synthetic_bundle;
  const auto republication_effects =
      prepare::plan_prepared_call_boundary_effects(call_plan, nullptr, &republication_bundle);
  for (const auto& effect : republication_effects) {
    if (effect.effect_kind !=
            prepare::PreparedCallBoundaryEffectKind::PreservationRepublication ||
        effect.phase != prepare::PreparedMovePhase::AfterCall) {
      continue;
    }
    if (auto instruction = make_callee_saved_preservation_home_republication_instruction(
            context,
            republication_bundle,
            effect,
            prepare::PreparedMovePhase::BeforeInstruction,
            call_plan.block_index,
            call_plan.instruction_index,
            effect.reason.empty() ? "callee_saved_preservation_home_republication"
                                  : effect.reason,
            diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  return lowered;
}

std::vector<module::MachineInstruction> lower_before_return_moves(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr ||
      context.function.control_flow == nullptr) {
    return lowered;
  }
  const auto* bundle = find_move_bundle(context,
                                        prepare::PreparedMovePhase::BeforeReturn,
                                        context.block_index,
                                        instruction_index);
  if (bundle == nullptr) {
    return lowered;
  }

  for (const auto& move : bundle->moves) {
    if (move.destination_kind != prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
        move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        move.op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
      continue;
    }
    const auto* source_home =
        find_value_home(context, move.from_value_id);
    if (source_home == nullptr || !move.destination_register_name.has_value()) {
      continue;
    }

    const auto expected_view = scalar_view_from_register_name(move.destination_register_name);
    const auto destination_bank =
        move.destination_register_placement.has_value()
            ? std::optional<prepare::PreparedRegisterBank>{
                  move.destination_register_placement->bank}
            : std::nullopt;
    auto destination = make_register_operand_from_prepared_authority(
        move.destination_register_name,
        move.destination_register_placement,
        destination_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : std::nullopt,
        source_home->value_name,
        move.destination_contiguous_width,
        move.destination_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      continue;
    }

    CallBoundaryMoveInstructionRecord move_record{
        .function_name = context.function.control_flow->function_name,
        .phase = bundle->phase,
        .authority_kind = bundle->authority_kind,
        .block_index = bundle->block_index,
        .instruction_index = bundle->instruction_index,
        .source_parallel_copy_predecessor_label =
            bundle->source_parallel_copy_predecessor_label,
        .source_parallel_copy_successor_label =
            bundle->source_parallel_copy_successor_label,
        .move = move,
        .destination_register = *destination,
        .source_bundle = bundle,
        .source_move = &move,
    };
    if (source_home->kind == prepare::PreparedValueHomeKind::Register &&
        source_home->register_name.has_value()) {
      const auto source_register_name =
          register_name_with_expected_view(source_home->register_name, expected_view);
      auto source = make_register_operand_from_prepared_authority(
          source_register_name,
          std::nullopt,
          destination_bank,
          RegisterOperandRole::CallAbi,
          source_home->value_id,
          source_home->value_name,
          1,
          {},
          expected_view,
          diagnostics,
          context,
          instruction_index);
      if (!source.has_value()) {
        continue;
      }
      move_record.source_register = *source;
    } else if (source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
               source_home->offset_bytes.has_value()) {
      move_record.source_memory = MemoryOperand{
          .surface = RecordSurfaceKind::MachineInstructionNode,
          .support = MemoryOperandSupportKind::Prepared,
          .function_name = context.function.control_flow->function_name,
          .block_label = context.control_flow_block != nullptr
                             ? context.control_flow_block->block_label
                             : c4c::kInvalidBlockLabel,
          .instruction_index = instruction_index,
          .result_value_id = source_home->value_id,
          .result_value_name = source_home->value_name,
          .base_kind = MemoryBaseKind::FrameSlot,
          .frame_slot_id = source_home->slot_id,
          .byte_offset = static_cast<std::int64_t>(*source_home->offset_bytes),
          .byte_offset_is_prepared_snapshot = true,
          .size_bytes = source_home->size_bytes.value_or(
              scalar_size_from_register_view(expected_view)),
          .align_bytes = source_home->align_bytes.value_or(
              scalar_size_from_register_view(expected_view)),
          .can_use_base_plus_offset = true,
      };
    } else {
      continue;
    }
    lowered.push_back(make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_call_boundary_move_instruction(std::move(move_record))));
  }
  return lowered;
}

std::vector<module::MachineInstruction> lower_value_moves(
    const module::BlockLoweringContext& context,
    prepare::PreparedMovePhase phase,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      (phase != prepare::PreparedMovePhase::BlockEntry &&
       phase != prepare::PreparedMovePhase::BeforeInstruction)) {
    return lowered;
  }
  const auto* bundle = find_move_bundle(context, phase, context.block_index, instruction_index);
  const prepare::PreparedMoveBundle synthetic_bundle{
      .function_name = context.function.control_flow->function_name,
      .phase = phase,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
  };

  if (bundle != nullptr) {
    for (const auto& move : bundle->moves) {
    if (move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
        move.op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
      continue;
    }
    const auto* destination_home =
        find_value_home(context, move.to_value_id);
    if (destination_home == nullptr) {
      continue;
    }
    const auto* source_home =
        find_value_home(context, move.from_value_id);
    if (move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
        destination_home->kind == prepare::PreparedValueHomeKind::StackSlot) {
      if (auto stack_move = make_value_stack_move_instruction(
              context,
              *bundle,
              move,
              *destination_home,
              source_home,
              instruction_index)) {
        lowered.push_back(std::move(*stack_move));
      }
      continue;
    }
    if (move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        destination_home->kind != prepare::PreparedValueHomeKind::Register) {
      continue;
    }
    const auto destination_register_view =
        scalar_view_from_register_name(destination_home->register_name);
    const auto expected_view =
        destination_register_view.has_value()
            ? destination_register_view
            : (destination_home->size_bytes.has_value()
                   ? scalar_integer_register_view_from_size(*destination_home->size_bytes)
                   : std::nullopt);
    const auto destination_bank =
        expected_view == std::optional<abi::RegisterView>{abi::RegisterView::S} ||
                expected_view == std::optional<abi::RegisterView>{abi::RegisterView::D}
            ? prepare::PreparedRegisterBank::Fpr
            : prepare::PreparedRegisterBank::Gpr;
    auto destination = make_register_operand_from_prepared_authority(
        destination_home->register_name,
        move.destination_register_placement,
        destination_bank,
        RegisterOperandRole::StoragePlan,
        destination_home->value_id,
        destination_home->value_name,
        move.destination_contiguous_width,
        move.destination_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      continue;
    }

    CallBoundaryMoveInstructionRecord move_record{
        .function_name = context.function.control_flow->function_name,
        .phase = phase,
        .authority_kind = bundle->authority_kind,
        .block_index = bundle->block_index,
        .instruction_index = bundle->instruction_index,
        .source_parallel_copy_predecessor_label =
            bundle->source_parallel_copy_predecessor_label,
        .source_parallel_copy_successor_label =
            bundle->source_parallel_copy_successor_label,
        .move = move,
        .destination_register = *destination,
        .source_bundle = bundle,
        .source_move = &move,
    };
    if (move.source_immediate_i32.has_value()) {
      const auto immediate = make_scalar_call_argument_immediate(
          bir::Value::immediate_i32(static_cast<std::int32_t>(*move.source_immediate_i32)),
          move.from_value_id);
      if (!immediate.has_value()) {
        continue;
      }
      move_record.source_immediate = immediate;
    } else if (const auto* prior_stack_preserved =
                   phase == prepare::PreparedMovePhase::BeforeInstruction
                       ? find_prior_stack_preserved_value_before_instruction(
                             context, move.from_value_id, instruction_index)
                       : nullptr;
               prior_stack_preserved != nullptr) {
      auto source = make_prior_stack_preserved_value_source(
          context, *prior_stack_preserved, instruction_index);
      if (!source.has_value()) {
        continue;
      }
      move_record.source_memory = *source;
    } else if (source_home != nullptr &&
               source_home->kind == prepare::PreparedValueHomeKind::Register &&
               source_home->register_name.has_value()) {
      auto source = make_register_operand_from_prepared_authority(
          source_home->register_name,
          std::nullopt,
          destination_bank,
          RegisterOperandRole::StoragePlan,
          source_home->value_id,
          source_home->value_name,
          1,
          {},
          expected_view,
          diagnostics,
          context,
          instruction_index);
      if (!source.has_value()) {
        continue;
      }
      move_record.source_register = *source;
    } else if (source_home != nullptr &&
               source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
               source_home->offset_bytes.has_value()) {
      move_record.source_memory = MemoryOperand{
          .surface = RecordSurfaceKind::MachineInstructionNode,
          .support = MemoryOperandSupportKind::Prepared,
          .function_name = context.function.control_flow->function_name,
          .block_label = context.control_flow_block->block_label,
          .instruction_index = instruction_index,
          .result_value_id = source_home->value_id,
          .result_value_name = source_home->value_name,
          .base_kind = MemoryBaseKind::FrameSlot,
          .frame_slot_id = source_home->slot_id,
          .byte_offset = static_cast<std::int64_t>(*source_home->offset_bytes),
          .byte_offset_is_prepared_snapshot = true,
          .size_bytes = source_home->size_bytes.value_or(4),
          .align_bytes = source_home->align_bytes.value_or(4),
          .can_use_base_plus_offset = true,
      };
    } else {
      continue;
    }

    lowered.push_back(make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_call_boundary_move_instruction(std::move(move_record))));
    }
  }

  if (phase != prepare::PreparedMovePhase::BlockEntry) {
    return lowered;
  }

  if (context.function.call_plan_lookups == nullptr) {
    return lowered;
  }
  const auto* selected_preservation_effects =
      prepare::indexed_block_entry_republication_effects_for_block(
          *context.function.call_plan_lookups, context.block_index);
  if (selected_preservation_effects == nullptr) {
    return lowered;
  }

  const auto& republication_bundle = bundle != nullptr ? *bundle : synthetic_bundle;
  for (const auto& effect : *selected_preservation_effects) {
    if (auto instruction =
            make_callee_saved_preservation_home_republication_instruction(
                context,
                republication_bundle,
                effect,
                prepare::PreparedMovePhase::BlockEntry,
                context.block_index,
                instruction_index,
                "callee_saved_preservation_home_block_entry_republication",
                diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  return lowered;
}

}  // namespace c4c::backend::aarch64::codegen
