#include "calls.hpp"
#include "constant_materialization.hpp"
#include "dispatch.hpp"
#include "dispatch_lookup.hpp"
#include "effects.hpp"
#include "float_ops.hpp"
#include "f128.hpp"
#include "frame_slot_address.hpp"
#include "machine_printer.hpp"
#include "memory.hpp"
#include "variadic.hpp"

#include "../abi/abi.hpp"
#include "../../query.hpp"
#include "alu.hpp"
#include "comparison.hpp"
#include "dispatch_edge_copies.hpp"
#include "dispatch_publication.hpp"
#include "dispatch_value_materialization.hpp"
#include "fp_value_materialization.hpp"
#include "globals.hpp"
#include "memory_dynamic_stack.hpp"
#include "select_materialization.hpp"

#include <algorithm>
#include <array>
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

namespace {

[[nodiscard]] std::optional<c4c::ValueNameId> prepared_named_value_id(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.prepared == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  return prepare::resolve_prepared_value_name_id(context.function.prepared->names,
                                                 value.name);
}

}  // namespace

[[nodiscard]] std::optional<MemoryOperand> make_selected_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome* source_home,
    const prepare::PreparedCallArgumentSourceSelection& selection,
    prepare::PreparedCallArgumentSourceSelectionKind expected_kind,
    std::size_t instruction_index);

const prepare::PreparedCallPlan* find_prepared_call_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  return prepare::find_indexed_prepared_call_plan(context.function.call_plan_lookups,
                                                  context.function.call_plans,
                                                  context.block_index,
                                                  instruction_index);
}

void append_call_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                            module::ModuleLoweringDiagnosticKind kind,
                            const module::BlockLoweringContext& context,
                            std::size_t instruction_index,
                            std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = kind,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .instruction_family = module::InstructionLoweringFamily::Call,
      .message = std::move(message),
  });
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

struct StackFrameSlotCallOperandOwner {
  [[nodiscard]] static bool frame_slot_direct_offset_is_encodable(
      const MemoryOperand& memory,
      std::size_t load_width_bytes);

  [[nodiscard]] static std::vector<std::string> materialize_frame_slot_address_lines(
      abi::RegisterReference scratch,
      const MemoryOperand& memory);

  [[nodiscard]] static std::string stack_copy_address(std::string_view base,
                                                      std::int64_t offset);

  [[nodiscard]] static std::optional<MemoryOperand> memory_return_storage(
      const module::BlockLoweringContext& context,
      const prepare::PreparedMemoryReturnPlan& memory_return,
      std::size_t instruction_index);

  [[nodiscard]] static std::optional<MemoryOperand> selected_frame_slot_source(
      const module::BlockLoweringContext& context,
      const prepare::PreparedCallArgumentPlan& argument,
      const prepare::PreparedValueHome* source_home,
      const prepare::PreparedCallArgumentSourceSelection& selection,
      prepare::PreparedCallArgumentSourceSelectionKind expected_kind,
      bool materialized_address,
      std::size_t instruction_index);

  [[nodiscard]] static std::optional<MemoryOperand> sret_memory_return_address_source(
      const module::BlockLoweringContext& context,
      const prepare::PreparedCallPlan& call_plan,
      const prepare::PreparedCallArgumentPlan& argument,
      std::size_t instruction_index);

  [[nodiscard]] static std::optional<MemoryOperand> selected_local_frame_address_source(
      const module::BlockLoweringContext& context,
      const prepare::PreparedCallArgumentPlan& argument,
      const prepare::PreparedValueHome& source_home,
      const prepare::PreparedCallArgumentSourceSelection& selection,
      std::size_t instruction_index);

  [[nodiscard]] static std::optional<MemoryOperand> stack_call_argument_destination(
      const module::BlockLoweringContext& context,
      const prepare::PreparedCallArgumentPlan& argument,
      const prepare::PreparedValueHome& source_home,
      const prepare::PreparedMoveResolution& move,
      const prepare::PreparedAbiBinding* binding,
      const MemoryOperand& source,
      std::size_t instruction_index);

  [[nodiscard]] static std::optional<MemoryOperand> aggregate_call_argument_source(
      const module::BlockLoweringContext& context,
      const prepare::PreparedCallArgumentPlan& argument,
      const prepare::PreparedValueHome& source_home,
      const RegisterOperand& source_register,
      std::size_t size_bytes,
      std::int64_t byte_offset,
      std::size_t instruction_index);

  [[nodiscard]] static std::optional<MemoryOperand> preserved_stack_selection_source(
      const module::BlockLoweringContext& context,
      const prepare::PreparedCallArgumentSourceSelection& selection,
      std::size_t instruction_index);

  [[nodiscard]] static bool endpoint_has_stack_storage(
      const prepare::PreparedCallBoundaryEffectEndpoint& endpoint);

  [[nodiscard]] static std::optional<MemoryOperand> frame_slot_memory_from_endpoint(
      const module::BlockLoweringContext& context,
      const prepare::PreparedCallBoundaryEffectEndpoint& endpoint,
      prepare::PreparedValueId value_id,
      ValueNameId value_name,
      std::size_t instruction_index);

  [[nodiscard]] static std::optional<MemoryOperand> prior_stack_preserved_value_source(
      const module::BlockLoweringContext& context,
      const prepare::PreparedCallPreservedValue& preserved,
      std::size_t instruction_index);
};

[[nodiscard]] std::optional<MemoryOperand>
StackFrameSlotCallOperandOwner::memory_return_storage(
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
  return call_plan.outgoing_stack_argument_area.has_value()
             ? align_to(call_plan.outgoing_stack_argument_area->size_bytes,
                        kStackPointerAlignmentBytes)
             : 0;
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
              ? StackFrameSlotCallOperandOwner::memory_return_storage(
                    context,
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

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_f128_stack_call_argument_source(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const MemoryOperand& source,
    const MemoryOperand& destination);

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_fp_stack_call_argument_source(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const MemoryOperand& source,
    const MemoryOperand& destination);

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_byval_stack_call_argument_source(
    const module::BlockLoweringContext& context,
    const MemoryOperand& source,
    const MemoryOperand& destination,
    std::size_t before_instruction_index);

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

struct F128CarrierCallOperandOwner {
  [[nodiscard]] static bool is_complete_full_width(
      const prepare::PreparedF128Carrier* carrier);

  [[nodiscard]] static bool is_complete_constant(
      const prepare::PreparedF128Carrier* carrier);

  [[nodiscard]] static const prepare::PreparedF128Carrier* find_in_module(
      const prepare::PreparedBirModule& prepared,
      prepare::PreparedValueId value_id);

  [[nodiscard]] static std::optional<RegisterOperand> q_register_operand(
      const prepare::PreparedF128Carrier& carrier,
      RegisterOperandRole role,
      std::optional<prepare::PreparedValueId> value_id,
      c4c::ValueNameId value_name,
      module::ModuleLoweringDiagnostics& diagnostics,
      const module::BlockLoweringContext& context,
      std::size_t instruction_index);
};

[[nodiscard]] bool F128CarrierCallOperandOwner::is_complete_full_width(
    const prepare::PreparedF128Carrier* carrier) {
  return carrier != nullptr &&
         carrier->kind == prepare::PreparedF128CarrierKind::FullWidthRegister &&
         carrier->missing_required_facts.empty() &&
         carrier->total_size_bytes == 16 && carrier->total_align_bytes == 16 &&
         carrier->register_bank == prepare::PreparedRegisterBank::Vreg &&
         carrier->register_class == prepare::PreparedRegisterClass::Vector &&
         carrier->contiguous_width == 1 && carrier->register_name.has_value();
}

[[nodiscard]] bool F128CarrierCallOperandOwner::is_complete_constant(
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
F128CarrierCallOperandOwner::find_in_module(
    const prepare::PreparedBirModule& prepared,
    prepare::PreparedValueId value_id) {
  for (const auto& function_carriers : prepared.f128_carriers.functions) {
    if (const auto* carrier =
            prepare::find_prepared_f128_carrier(function_carriers, value_id)) {
      return carrier;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<RegisterOperand>
F128CarrierCallOperandOwner::q_register_operand(
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
        "AArch64 binary128 call-boundary carrier is not an FP/SIMD register");
    return std::nullopt;
  }
  const auto viewed = abi::fp_simd_register(parsed->index, abi::RegisterView::Q);
  if (!viewed.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::RegisterConversionFailed,
        context,
        instruction_index,
        "AArch64 binary128 call-boundary carrier could not be re-viewed as q-register");
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

[[nodiscard]] bool validate_aggregate_register_lane_publication(
    const module::BlockLoweringContext& context,
    const CallBoundaryMoveInstructionRecord& move,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (aggregate_register_lane_publication_view(move).has_value()) {
    return true;
  }
  append_call_diagnostic(
      diagnostics,
      module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
      context,
      instruction_index,
      "AArch64 aggregate register-lane call-argument publication requires prepared source bytes and destination register");
  return false;
}

[[nodiscard]] std::optional<MemoryOperand>
make_byval_register_lane_prepared_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t size_bytes,
    std::size_t instruction_index) {
  if (size_bytes == 0 || size_bytes > 16) {
    return std::nullopt;
  }
  const auto* transport = argument.aggregate_transport.has_value()
                              ? &*argument.aggregate_transport
                              : nullptr;
  if (transport == nullptr ||
      transport->kind != prepare::PreparedAggregateTransportKind::ByvalRegisterLanes ||
      transport->payload_size_bytes != size_bytes ||
      transport->payload_align_bytes == 0 ||
      !transport->source_slot_id.has_value() ||
      transport->chunks.empty() ||
      transport->lanes.empty()) {
    return std::nullopt;
  }

  std::optional<std::size_t> source_base_offset;
  std::size_t covered_chunk_bytes = 0;
  std::array<bool, 16> chunk_coverage{};
  std::array<bool, 16> lane_coverage{};
  for (const auto& chunk : transport->chunks) {
    if (chunk.kind != prepare::PreparedAggregateTransportChunkKind::RequiredPayload ||
        chunk.size_bytes == 0 ||
        chunk.payload_offset_bytes + chunk.size_bytes > size_bytes ||
        chunk.source_offset_bytes < chunk.payload_offset_bytes ||
        chunk.align_bytes == 0) {
      return std::nullopt;
    }
    const auto expected_base = chunk.source_offset_bytes - chunk.payload_offset_bytes;
    if (!source_base_offset.has_value()) {
      source_base_offset = expected_base;
    } else if (*source_base_offset != expected_base) {
      return std::nullopt;
    }
    for (std::size_t offset = chunk.payload_offset_bytes;
         offset < chunk.payload_offset_bytes + chunk.size_bytes;
         ++offset) {
      if (chunk_coverage[offset]) {
        return std::nullopt;
      }
      chunk_coverage[offset] = true;
      ++covered_chunk_bytes;
    }
  }
  if (!source_base_offset.has_value() || covered_chunk_bytes != size_bytes) {
    return std::nullopt;
  }
  for (std::size_t offset = 0; offset < size_bytes; ++offset) {
    if (!chunk_coverage[offset]) {
      return std::nullopt;
    }
  }

  std::size_t covered_lane_bytes = 0;
  for (const auto& lane : transport->lanes) {
    if (lane.lane_size_bytes == 0 ||
        lane.lane_payload_offset_bytes + lane.lane_size_bytes > size_bytes ||
        lane.source_offset_bytes != *source_base_offset + lane.lane_payload_offset_bytes) {
      return std::nullopt;
    }
    const auto chunk_matches_lane =
        std::any_of(transport->chunks.begin(),
                    transport->chunks.end(),
                    [&](const prepare::PreparedAggregateTransportChunk& chunk) {
                      return chunk.chunk_index == lane.chunk_index &&
                             chunk.payload_offset_bytes == lane.lane_payload_offset_bytes &&
                             chunk.size_bytes == lane.lane_size_bytes;
                    });
    if (!chunk_matches_lane) {
      return std::nullopt;
    }
    for (std::size_t offset = lane.lane_payload_offset_bytes;
         offset < lane.lane_payload_offset_bytes + lane.lane_size_bytes;
         ++offset) {
      if (lane_coverage[offset]) {
        return std::nullopt;
      }
      lane_coverage[offset] = true;
      ++covered_lane_bytes;
    }
  }
  if (covered_lane_bytes != size_bytes) {
    return std::nullopt;
  }
  for (std::size_t offset = 0; offset < size_bytes; ++offset) {
    if (!lane_coverage[offset]) {
      return std::nullopt;
    }
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
      .result_value_id = argument.source_value_id.has_value()
                             ? argument.source_value_id
                             : std::optional<prepare::PreparedValueId>{source_home.value_id},
      .result_value_name = std::nullopt,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = transport->source_slot_id,
      .byte_offset = static_cast<std::int64_t>(*source_base_offset),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = size_bytes,
      .align_bytes = transport->payload_align_bytes,
      .can_use_base_plus_offset = true,
  };
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

[[nodiscard]] bool StackFrameSlotCallOperandOwner::frame_slot_direct_offset_is_encodable(
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

std::vector<std::string> StackFrameSlotCallOperandOwner::materialize_frame_slot_address_lines(
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

[[nodiscard]] std::string StackFrameSlotCallOperandOwner::stack_copy_address(
    std::string_view base,
    std::int64_t offset) {
  std::ostringstream out;
  out << "[" << base;
  if (offset != 0) {
    out << ", #" << offset;
  }
  out << "]";
  return out.str();
}

[[nodiscard]] MemoryOperand source_memory_after_outgoing_stack_reservation(
    MemoryOperand source,
    std::size_t outgoing_bytes) {
  if (outgoing_bytes == 0) {
    return source;
  }
  const auto adjustment = static_cast<std::int64_t>(outgoing_bytes);
  if (source.base_register.has_value() &&
      abi::is_stack_pointer(source.base_register->reg)) {
    source.byte_offset += adjustment;
  } else if (source.base_kind == MemoryBaseKind::FrameSlot &&
             !source.uses_frame_pointer_base) {
    source.byte_offset += adjustment;
  }
  return source;
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

[[nodiscard]] std::optional<MemoryOperand>
StackFrameSlotCallOperandOwner::selected_frame_slot_source(
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
      .uses_frame_pointer_base = fixed_slots_use_frame_pointer(context.function),
  };
}

[[nodiscard]] std::optional<MemoryOperand>
StackFrameSlotCallOperandOwner::sret_memory_return_address_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index) {
  const auto routing =
      prepare::find_prepared_call_argument_publication_source_routing(argument);
  if (routing.source_selection != nullptr) {
    if (auto selected = selected_frame_slot_source(
            context,
            argument,
            nullptr,
            *routing.source_selection,
            prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress,
            true,
            instruction_index)) {
      return selected;
    }
    return std::nullopt;
  }
  if (argument.source_selection.has_value()) {
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

[[nodiscard]] std::optional<MemoryOperand>
StackFrameSlotCallOperandOwner::selected_local_frame_address_source(
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
      .uses_frame_pointer_base = fixed_slots_use_frame_pointer(context.function),
  };
}

[[nodiscard]] std::optional<MemoryOperand>
StackFrameSlotCallOperandOwner::stack_call_argument_destination(
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

[[nodiscard]] std::optional<MemoryOperand>
StackFrameSlotCallOperandOwner::aggregate_call_argument_source(
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
  auto source_memory =
      StackFrameSlotCallOperandOwner::preserved_stack_selection_source(
          context, selection, instruction_index);
  if (!source_memory.has_value()) {
    return std::nullopt;
  }
  PreservedCallArgumentSource result;
  result.source_memory = *source_memory;
  return result;
}

[[nodiscard]] std::optional<MemoryOperand>
StackFrameSlotCallOperandOwner::preserved_stack_selection_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentSourceSelection& selection,
    std::size_t instruction_index) {
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

[[nodiscard]] bool StackFrameSlotCallOperandOwner::endpoint_has_stack_storage(
    const prepare::PreparedCallBoundaryEffectEndpoint& endpoint) {
  return endpoint.storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
         endpoint.slot_id.has_value() &&
         endpoint.stack_offset_bytes.has_value() &&
         endpoint.stack_size_bytes.has_value() &&
         *endpoint.stack_size_bytes != 0;
}

[[nodiscard]] std::optional<MemoryOperand>
StackFrameSlotCallOperandOwner::frame_slot_memory_from_endpoint(
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
  if (!StackFrameSlotCallOperandOwner::endpoint_has_stack_storage(
          preserved->preservation_destination)) {
    return nullptr;
  }
  return preserved;
}

[[nodiscard]] std::optional<MemoryOperand>
StackFrameSlotCallOperandOwner::prior_stack_preserved_value_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPreservedValue& preserved,
    std::size_t instruction_index) {
  return frame_slot_memory_from_endpoint(
      context,
      preserved.preservation_destination,
      preserved.value_id,
      preserved.value_name,
      instruction_index);
}

[[nodiscard]] module::MachineInstruction make_outgoing_stack_base_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::string asm_text) {
  const auto scratch = outgoing_stack_argument_base_register();
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
          .inline_asm_template = std::move(asm_text),
      },
  };
  return make_call_boundary_machine_instruction(context, instruction_index, std::move(target));
}

[[nodiscard]] module::MachineInstruction make_outgoing_stack_adjustment_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::string asm_text) {
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
      .side_effects = {MachineSideEffectKind::InlineAssembly},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = std::move(asm_text),
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
    asm_text += StackFrameSlotCallOperandOwner::stack_copy_address(
        source_base, source.byte_offset + static_cast<std::int64_t>(offset));
    asm_text += '\n';
    asm_text += std::string{store_mnemonic};
    asm_text += " ";
    asm_text += std::string{abi::register_name(*scratch)};
    asm_text += ", ";
    asm_text += StackFrameSlotCallOperandOwner::stack_copy_address(
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
  if (!argument.aggregate_transport.has_value() ||
      argument.aggregate_transport->kind !=
          prepare::PreparedAggregateTransportKind::ByvalRegisterLanes ||
      argument.aggregate_transport->payload_size_bytes == 0) {
    return std::nullopt;
  }
  return argument.aggregate_transport->payload_size_bytes;
}

[[nodiscard]] bool has_byval_register_lane_transport(
    const prepare::PreparedCallArgumentPlan& argument) {
  return argument.aggregate_transport.has_value() &&
         argument.aggregate_transport->kind ==
             prepare::PreparedAggregateTransportKind::ByvalRegisterLanes;
}

struct ImmediateScalarCallArgumentPublicationOwner {
  [[nodiscard]] static std::optional<module::MachineInstruction> instruction(
      const module::BlockLoweringContext& context,
      const prepare::PreparedValueHome& source_home,
      const RegisterOperand& destination,
      std::size_t instruction_index);

 private:
  [[nodiscard]] static const bir::CastInst* find_same_block_cast_producer(
      const module::BlockLoweringContext& context,
      c4c::ValueNameId value_name,
      std::size_t before_instruction_index);
  [[nodiscard]] static std::optional<unsigned> integer_width_bits_for_type(
      bir::TypeKind type);
  [[nodiscard]] static std::uint64_t immediate_integer_bits(const bir::Value& value,
                                                            unsigned width_bits);
  [[nodiscard]] static std::optional<abi::RegisterReference> scalar_fp_register_view(
      abi::RegisterReference reg,
      bir::TypeKind type);
  [[nodiscard]] static std::optional<abi::RegisterReference> scalar_gp_register_view(
      abi::RegisterReference reg,
      unsigned width_bits);
  static bool append_materialize_fp_immediate(std::vector<std::string>& lines,
                                              const bir::Value& value,
                                              abi::RegisterReference gp_scratch_base,
                                              abi::RegisterReference fp_scratch_base,
                                              abi::RegisterReference& fp_scratch);
  [[nodiscard]] static std::optional<std::vector<std::string>>
  make_publication_lines(const bir::CastInst& cast,
                         const RegisterOperand& destination);
};

const bir::CastInst* ImmediateScalarCallArgumentPublicationOwner::find_same_block_cast_producer(
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

std::optional<unsigned> ImmediateScalarCallArgumentPublicationOwner::integer_width_bits_for_type(
    bir::TypeKind type) {
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

std::uint64_t ImmediateScalarCallArgumentPublicationOwner::immediate_integer_bits(
    const bir::Value& value,
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

std::optional<abi::RegisterReference>
ImmediateScalarCallArgumentPublicationOwner::scalar_fp_register_view(
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

std::optional<abi::RegisterReference>
ImmediateScalarCallArgumentPublicationOwner::scalar_gp_register_view(
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

bool ImmediateScalarCallArgumentPublicationOwner::append_materialize_fp_immediate(
    std::vector<std::string>& lines,
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

std::optional<std::vector<std::string>>
ImmediateScalarCallArgumentPublicationOwner::make_publication_lines(
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

std::optional<module::MachineInstruction>
ImmediateScalarCallArgumentPublicationOwner::instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& source_home,
    const RegisterOperand& destination,
    std::size_t instruction_index) {
  const auto* cast =
      find_same_block_cast_producer(context, source_home.value_name, instruction_index);
  if (cast == nullptr) {
    return std::nullopt;
  }
  const auto lines = make_publication_lines(*cast, destination);
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

struct PreparedBeforeCallMoveOwnerInputs {
  const prepare::PreparedValueHome* source_home = nullptr;
  prepare::PreparedCallBoundaryMoveClassification classification{};
  const prepare::PreparedF128CarrierFunction* f128_carriers = nullptr;
  const prepare::PreparedF128Carrier* source_f128_carrier = nullptr;
  std::size_t outgoing_stack_argument_bytes = 0;
};

struct BeforeCallMoveLocalOwner {
  [[nodiscard]] static std::optional<module::MachineInstruction> instruction(
      const module::BlockLoweringContext& context,
      const prepare::PreparedCallPlan& call_plan,
      const prepare::PreparedMoveBundle& bundle,
      const prepare::PreparedCallBoundaryEffectPlan& effect,
      const prepare::PreparedMoveResolution& move,
      const PreparedBeforeCallMoveOwnerInputs& prepared,
      std::size_t instruction_index,
      module::ModuleLoweringDiagnostics& diagnostics);
};

std::optional<module::MachineInstruction> BeforeCallMoveLocalOwner::instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedCallBoundaryEffectPlan& effect,
    const prepare::PreparedMoveResolution& move,
    const PreparedBeforeCallMoveOwnerInputs& prepared,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* source_home = prepared.source_home;
  const auto& classification = prepared.classification;
  const bool selected_prepared_register_argument_effect =
      effect.effect_kind == prepare::PreparedCallBoundaryEffectKind::ExplicitMove &&
      effect.phase == prepare::PreparedMovePhase::BeforeCall &&
      effect.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      effect.storage_kind == prepare::PreparedMoveStorageKind::Register &&
      effect.order_index < bundle.moves.size() &&
      &bundle.moves[effect.order_index] == &move;
  const auto* argument = classification.argument_plan;
  const auto* binding = classification.abi_binding;
  const auto* f128_carriers = prepared.f128_carriers;
  const auto* source_f128_carrier = prepared.source_f128_carrier;
  const bool structured_f128_register_argument_move =
      argument != nullptr &&
      (argument->source_register_bank == prepare::PreparedRegisterBank::Vreg ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Fpr) &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Vreg &&
      F128CarrierCallOperandOwner::is_complete_full_width(source_f128_carrier);

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
  if (selected_prepared_register_argument_effect &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      argument != nullptr) {
    const auto routing =
        prepare::find_prepared_call_argument_publication_source_routing(*argument);
    const auto* source_selection = routing.source_selection;
    const bool frame_slot_address_argument =
        source_home != nullptr &&
        argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
        ((source_selection != nullptr &&
          source_selection->kind ==
              prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress) ||
         (source_selection == nullptr &&
          StackFrameSlotCallOperandOwner::sret_memory_return_address_source(
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
          source_selection != nullptr &&
                  source_selection->kind ==
                      prepare::PreparedCallArgumentSourceSelectionKind::
                          PriorPreservation
              ? source_selection
              : nullptr;
      std::optional<PreservedCallArgumentSource> preserved_source;
      std::optional<ValueNameId> preserved_source_value_name;
      if (prior_selection == nullptr && source_selection != nullptr &&
          source_selection->kind ==
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
                         : (effect.destination.register_placement.has_value()
                                ? effect.destination.register_placement
                                : argument->destination_register_placement));
          const auto destination_register_name =
              destination_register_placement.has_value()
                  ? std::optional<std::string>{}
                  : (binding != nullptr && binding->destination_register_name.has_value()
                         ? binding->destination_register_name
                         : (argument->destination_register_name.has_value()
                                ? argument->destination_register_name
                                : (move.destination_register_name.has_value()
                                       ? move.destination_register_name
                                       : effect.destination.register_name)));
          auto destination = make_register_operand_from_prepared_authority(
              destination_register_name,
              destination_register_placement,
              effect.destination.register_bank.has_value()
                  ? effect.destination.register_bank
                  : argument->destination_register_bank,
              RegisterOperandRole::CallAbi,
              move.to_value_id != 0
                  ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                  : argument->source_value_id,
              preserved_source_value_name.value_or(c4c::kInvalidValueName),
              effect.destination.contiguous_width,
              !effect.destination.occupied_register_names.empty()
                  ? effect.destination.occupied_register_names
                  : (binding != nullptr ? binding->destination_occupied_register_names
                                        : argument->destination_occupied_register_names),
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
      selected_prepared_register_argument_effect &&
      argument != nullptr &&
      argument->value_bank == prepare::PreparedRegisterBank::Vreg &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
      argument->source_literal.has_value() &&
      argument->source_literal->type == bir::TypeKind::F128 &&
      argument->source_literal->f128_payload.has_value() &&
      argument->source_value_id.has_value() &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      (effect.destination.register_bank.has_value()
           ? effect.destination.register_bank
           : std::optional<prepare::PreparedRegisterBank>{
                 argument->destination_register_bank}) ==
          std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Vreg} &&
      F128CarrierCallOperandOwner::is_complete_constant(source_f128_carrier) &&
      source_f128_carrier->constant_payload->low_bits ==
          argument->source_literal->f128_payload->low_bits &&
      source_f128_carrier->constant_payload->high_bits ==
          argument->source_literal->f128_payload->high_bits &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register;

  if (selected_prepared_register_argument_effect &&
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

  if (selected_prepared_register_argument_effect &&
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
        effect.destination.register_bank.has_value()
            ? effect.destination.register_bank
            : argument->destination_register_bank,
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
      auto source = F128CarrierCallOperandOwner::q_register_operand(
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

  if (selected_prepared_register_argument_effect &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      (source_home->kind == prepare::PreparedValueHomeKind::Register ||
       source_home->kind ==
           prepare::PreparedValueHomeKind::PointerBasePlusOffset) &&
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
      const auto routing =
          prepare::find_prepared_call_argument_publication_source_routing(*argument);
      const auto* source_selection = routing.source_selection;
      if (source_selection != nullptr) {
        address_source = make_selected_call_argument_source(
            context,
            *argument,
            source_home,
            *source_selection,
            prepare::PreparedCallArgumentSourceSelectionKind::
                LocalFrameAddressMaterialization,
            instruction_index);
      }
      source = address_source;
      if (!source.has_value() && source_selection != nullptr &&
          source_selection->kind ==
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
          effect.destination.register_bank.has_value()
              ? effect.destination.register_bank
              : argument->destination_register_bank,
          RegisterOperandRole::CallAbi,
          move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                                : argument->source_value_id,
          source_home->value_name,
          binding != nullptr ? binding->destination_contiguous_width
                             : effect.destination.contiguous_width,
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
        if (!validate_aggregate_register_lane_publication(
                context, move_record, instruction_index, diagnostics)) {
          return std::nullopt;
        }
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
            ImmediateScalarCallArgumentPublicationOwner::instruction(
                context, *source_home, *move_record.destination_register, instruction_index)) {
      return cast_publication;
    }
  }

  if (selected_prepared_register_argument_effect &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::Register &&
      source_home->register_name.has_value() &&
      argument != nullptr &&
      (argument->source_encoding == prepare::PreparedStorageEncodingKind::Register ||
       argument->source_encoding == prepare::PreparedStorageEncodingKind::ComputedAddress ||
       argument->source_encoding == prepare::PreparedStorageEncodingKind::SymbolAddress) &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      (effect.destination.register_bank.has_value()
           ? effect.destination.register_bank
           : std::optional<prepare::PreparedRegisterBank>{
                 argument->destination_register_bank}) ==
          std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} &&
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
        binding != nullptr && binding->destination_register_placement.has_value()
            ? binding->destination_register_placement
            : (move.destination_register_placement.has_value()
                   ? move.destination_register_placement
                   : (effect.destination.register_placement.has_value()
                          ? effect.destination.register_placement
                          : argument->destination_register_placement));
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : (binding != nullptr && binding->destination_register_name.has_value()
                   ? binding->destination_register_name
                   : (move.destination_register_name.has_value()
                          ? move.destination_register_name
                          : effect.destination.register_name));
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        effect.destination.register_bank.has_value()
            ? effect.destination.register_bank
            : argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        effect.destination.contiguous_width,
        !effect.destination.occupied_register_names.empty()
            ? effect.destination.occupied_register_names
            : (binding != nullptr ? binding->destination_occupied_register_names
                                  : (!move.destination_occupied_register_names.empty()
                                         ? move.destination_occupied_register_names
                                         : argument->destination_occupied_register_names)),
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
    if (!validate_aggregate_register_lane_publication(
            context, move_record, instruction_index, diagnostics)) {
      return std::nullopt;
    }
  }

  if (selected_prepared_register_argument_effect &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      (effect.destination.register_bank.has_value()
           ? effect.destination.register_bank
           : std::optional<prepare::PreparedRegisterBank>{
                 argument->destination_register_bank}) ==
          std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} &&
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
        binding != nullptr && binding->destination_register_placement.has_value()
            ? binding->destination_register_placement
            : (move.destination_register_placement.has_value()
                   ? move.destination_register_placement
                   : (effect.destination.register_placement.has_value()
                          ? effect.destination.register_placement
                          : argument->destination_register_placement));
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : (binding != nullptr && binding->destination_register_name.has_value()
                   ? binding->destination_register_name
                   : (move.destination_register_name.has_value()
                          ? move.destination_register_name
                          : effect.destination.register_name));
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        effect.destination.register_bank.has_value()
            ? effect.destination.register_bank
            : argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        effect.destination.contiguous_width,
        !effect.destination.occupied_register_names.empty()
            ? effect.destination.occupied_register_names
            : (binding != nullptr ? binding->destination_occupied_register_names
                                  : (!move.destination_occupied_register_names.empty()
                                         ? move.destination_occupied_register_names
                                         : argument->destination_occupied_register_names)),
        abi::RegisterView::X,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      return std::nullopt;
    }
    auto source = make_byval_register_lane_prepared_source(
        context, *argument, *source_home, *lane_size, call_plan.instruction_index);
    if (!source.has_value() && has_byval_register_lane_transport(*argument)) {
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
    if (!validate_aggregate_register_lane_publication(
            context, move_record, instruction_index, diagnostics)) {
      return std::nullopt;
    }
  }

  if (selected_prepared_register_argument_effect &&
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
      scalar_fp_view_from_register_name(binding->destination_register_name).has_value()) {
    const auto routing =
        prepare::find_prepared_call_argument_publication_source_routing(*argument);
    auto source = routing.source_selection != nullptr
                      ? make_selected_call_argument_source(
                            context,
                            *argument,
                            source_home,
                            *routing.source_selection,
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
        effect.destination.register_bank.has_value()
            ? effect.destination.register_bank
            : argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        effect.destination.contiguous_width,
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

  if (selected_prepared_register_argument_effect &&
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
    const auto routing =
        prepare::find_prepared_call_argument_publication_source_routing(*argument);
    auto source = routing.source_selection != nullptr
                      ? make_selected_call_argument_source(
                            context,
                            *argument,
                            source_home,
                            *routing.source_selection,
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
        effect.destination.register_bank.has_value()
            ? effect.destination.register_bank
            : argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        effect.destination.contiguous_width,
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

  if (selected_prepared_register_argument_effect &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      !is_aarch64_byval_register_lane_move(move) &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      (argument->source_register_bank == prepare::PreparedRegisterBank::Gpr ||
       argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress) &&
      (effect.destination.register_bank.has_value()
           ? effect.destination.register_bank
           : std::optional<prepare::PreparedRegisterBank>{
                 argument->destination_register_bank}) ==
          std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register)) {
    std::optional<MemoryOperand> address_source;
    std::optional<MemoryOperand> source;
    const auto routing =
        prepare::find_prepared_call_argument_publication_source_routing(*argument);
    const auto* source_selection = routing.source_selection;
    if (source_selection != nullptr) {
      switch (source_selection->kind) {
        case prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress:
          address_source = StackFrameSlotCallOperandOwner::sret_memory_return_address_source(
              context, call_plan, *argument, instruction_index);
          if (!address_source.has_value()) {
            address_source = make_selected_call_argument_source(
                context,
                *argument,
                source_home,
                *source_selection,
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
              *source_selection,
              prepare::PreparedCallArgumentSourceSelectionKind::
                  LocalFrameAddressMaterialization,
              instruction_index);
          break;
        case prepare::PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane:
          if (source_selection->byval_lane_extent_bytes.has_value() &&
              *source_selection->byval_lane_extent_bytes > 16) {
            address_source = StackFrameSlotCallOperandOwner::selected_frame_slot_source(
                context,
                *argument,
                source_home,
                *source_selection,
                prepare::PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane,
                true,
                instruction_index);
          } else if (const auto byval_size = selected_byval_lane_extent_bytes(*argument);
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
              *source_selection,
              prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress,
              instruction_index);
          break;
        case prepare::PreparedCallArgumentSourceSelectionKind::None:
          break;
      }
    } else {
      address_source = StackFrameSlotCallOperandOwner::sret_memory_return_address_source(
          context, call_plan, *argument, instruction_index);
    }
    if (!address_source.has_value() &&
        source_selection != nullptr &&
        (source_selection->kind ==
             prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue ||
         source_selection->kind ==
             prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation)) {
      source = make_selected_call_argument_source(
          context,
          *argument,
          source_home,
          *source_selection,
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
        effect.destination.register_bank.has_value()
            ? effect.destination.register_bank
            : argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        effect.destination.contiguous_width,
        binding != nullptr ? binding->destination_occupied_register_names
                           : effect.destination.occupied_register_names,
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
    move_record.source_memory = source_memory_after_outgoing_stack_reservation(
        *source, prepared.outgoing_stack_argument_bytes);
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
    if (!source.has_value() && has_byval_register_lane_transport(*argument)) {
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
    const auto adjusted_source = source_memory_after_outgoing_stack_reservation(
        *source, prepared.outgoing_stack_argument_bytes);
    const auto destination = StackFrameSlotCallOperandOwner::stack_call_argument_destination(
        context, *argument, *source_home, move, binding, adjusted_source, instruction_index);
    if (!destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack-lane call-argument publication requires a prepared destination stack offset");
      return std::nullopt;
    }
    if (auto materialized = materialize_byval_stack_call_argument_source(
            context, adjusted_source, *destination, instruction_index)) {
      return materialized;
    }
    auto lowered = make_byval_register_lane_stack_publication_instruction(
        context, instruction_index, adjusted_source, *destination);
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
    const auto source = StackFrameSlotCallOperandOwner::aggregate_call_argument_source(
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
    const auto destination = StackFrameSlotCallOperandOwner::stack_call_argument_destination(
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
    const auto adjusted_source = source_memory_after_outgoing_stack_reservation(
        *source, prepared.outgoing_stack_argument_bytes);
    return make_aggregate_stack_copy_instruction(
        context, instruction_index, adjusted_source, *destination);
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
    const auto routing =
        prepare::find_prepared_call_argument_publication_source_routing(*argument);
    const auto source =
        routing.source_selection != nullptr
            ? make_selected_call_argument_source(
                  context,
                  *argument,
                  source_home,
                  *routing.source_selection,
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
    const auto destination = StackFrameSlotCallOperandOwner::stack_call_argument_destination(
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
    const auto adjusted_source = source_memory_after_outgoing_stack_reservation(
        *source, prepared.outgoing_stack_argument_bytes);
    if (auto materialized = materialize_f128_stack_call_argument_source(
            context, instruction_index, adjusted_source, *destination)) {
      return materialized;
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
    const auto routing =
        prepare::find_prepared_call_argument_publication_source_routing(*argument);
    const auto source =
        routing.source_selection != nullptr
            ? make_selected_call_argument_source(
                  context,
                  *argument,
                  source_home,
                  *routing.source_selection,
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
    const auto destination = StackFrameSlotCallOperandOwner::stack_call_argument_destination(
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
    const auto adjusted_source = source_memory_after_outgoing_stack_reservation(
        *source, prepared.outgoing_stack_argument_bytes);
    if (auto materialized = materialize_fp_stack_call_argument_source(
            context, instruction_index, adjusted_source, *destination)) {
      return materialized;
    }
    auto target = make_memory_instruction(MemoryInstructionRecord{
        .memory_kind = MemoryInstructionKind::Store,
        .address = *destination,
        .value = make_memory_operand(adjusted_source),
        .value_type = *value_type,
    });
    // Outgoing stack arguments are call-boundary writes.  Keep the address
    // dependency from the generic store while publishing the x16-relative
    // outgoing argument slot as the destination lifetime.
    target.defs = {local_effect_from_operand(make_memory_operand(*destination))};
    target.uses = {local_effect_from_operand(make_memory_operand(*destination)),
                   local_effect_from_operand(make_memory_operand(adjusted_source))};
    return make_call_boundary_machine_instruction(context, instruction_index,
                                                  std::move(target));
  }

  if (selected_f128_constant_argument_move) {
    auto destination = make_register_operand_from_prepared_authority(
        binding->destination_register_name,
        binding->destination_register_placement,
        effect.destination.register_bank.has_value()
            ? effect.destination.register_bank
            : argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_f128_carrier->value_name,
        effect.destination.contiguous_width,
        !effect.destination.occupied_register_names.empty()
            ? effect.destination.occupied_register_names
            : binding->destination_occupied_register_names,
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

[[nodiscard]] std::optional<module::MachineInstruction> lower_before_call_move(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedCallBoundaryEffectPlan& effect,
    const prepare::PreparedMoveResolution& move,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* source_home =
      context.function.value_locations == nullptr
          ? nullptr
          : prepare::find_indexed_prepared_value_home(
                context.function.value_home_lookups,
                context.function.value_locations,
                move.from_value_id);
  auto classification =
      prepare::classify_prepared_call_boundary_move(call_plan, bundle, move);
  const auto* argument = classification.argument_plan;
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
        F128CarrierCallOperandOwner::find_in_module(*context.function.prepared,
                                                    *argument->source_value_id);
  }

  return BeforeCallMoveLocalOwner::instruction(
      context,
      call_plan,
      bundle,
      effect,
      move,
      PreparedBeforeCallMoveOwnerInputs{
          .source_home = source_home,
          .classification = std::move(classification),
          .f128_carriers = f128_carriers,
          .source_f128_carrier = source_f128_carrier,
          .outgoing_stack_argument_bytes = outgoing_stack_argument_bytes(call_plan),
      },
      instruction_index,
      diagnostics);
}

[[nodiscard]] const prepare::PreparedCallArgumentPlan*
find_immediate_argument_in_call_plan(
    const prepare::PreparedCallPlan& call_plan,
    std::size_t abi_index) {
  for (const auto& argument : call_plan.arguments) {
    if (argument.arg_index == abi_index &&
        argument.source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
        argument.source_literal.has_value()) {
      return &argument;
    }
  }
  return nullptr;
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

  const auto* argument =
      prepare::find_indexed_prepared_immediate_call_argument(
          context.function.call_plan_lookups,
          call_plan.block_index,
          call_plan.instruction_index,
          *binding.destination_abi_index);
  if (argument == nullptr) {
    argument = find_immediate_argument_in_call_plan(
        call_plan, *binding.destination_abi_index);
  }
  if (argument == nullptr ||
      (binding.destination_storage_kind != prepare::PreparedMoveStorageKind::StackSlot &&
       argument->destination_register_bank != prepare::PreparedRegisterBank::Gpr &&
       argument->destination_register_bank != prepare::PreparedRegisterBank::Fpr)) {
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

struct PreparedAfterCallMoveOwnerInputs {
  const prepare::PreparedValueHome* destination_home = nullptr;
  prepare::PreparedCallBoundaryMoveClassification classification{};
  const prepare::PreparedF128CarrierFunction* f128_carriers = nullptr;
  const prepare::PreparedF128Carrier* destination_f128_carrier = nullptr;
};

struct AfterCallMoveLocalOwner {
  [[nodiscard]] static std::optional<module::MachineInstruction> instruction(
      const module::BlockLoweringContext& context,
      const prepare::PreparedMoveBundle& bundle,
      const prepare::PreparedMoveResolution& move,
      const PreparedAfterCallMoveOwnerInputs& prepared,
      std::size_t instruction_index,
      module::ModuleLoweringDiagnostics& diagnostics);
};

std::optional<module::MachineInstruction> AfterCallMoveLocalOwner::instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move,
    const PreparedAfterCallMoveOwnerInputs& prepared,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* destination_home = prepared.destination_home;
  const auto& classification = prepared.classification;
  const auto* result_plan = classification.result_plan;
  const auto* binding = classification.abi_binding;
  const auto* destination_f128_carrier = prepared.destination_f128_carrier;

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
      F128CarrierCallOperandOwner::is_complete_full_width(destination_f128_carrier);

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
    auto destination =
        selected_f128_result_move
            ? F128CarrierCallOperandOwner::q_register_operand(
                  *destination_f128_carrier,
                  RegisterOperandRole::CallAbi,
                  destination_home->value_id,
                  destination_home->value_name,
                  diagnostics,
                  context,
                  instruction_index)
            : make_register_operand_from_prepared_authority(
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
        (result_plan->source_register_bank != prepare::PreparedRegisterBank::Gpr &&
         result_plan->source_register_bank != prepare::PreparedRegisterBank::Fpr &&
         result_plan->source_register_bank != prepare::PreparedRegisterBank::Vreg)) {
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
    const auto source_view =
        (result_plan->source_register_bank == prepare::PreparedRegisterBank::Fpr ||
         result_plan->source_register_bank == prepare::PreparedRegisterBank::Vreg)
            ? scalar_fp_view_from_register_name(result_plan->source_register_name)
            : scalar_view_from_register_name(result_plan->source_register_name);
    const auto width_bytes =
        destination_home->size_bytes.value_or(scalar_size_from_register_view(source_view));
    const auto expected_view =
        result_plan->source_register_bank == prepare::PreparedRegisterBank::Vreg
            ? std::optional<abi::RegisterView>{abi::RegisterView::Q}
        : result_plan->source_register_bank == prepare::PreparedRegisterBank::Fpr
            ? source_view
            : scalar_integer_register_view_from_size(width_bytes);
    const auto value_type =
        result_plan->source_register_bank == prepare::PreparedRegisterBank::Vreg
            ? std::optional<bir::TypeKind>{bir::TypeKind::F128}
        : result_plan->source_register_bank == prepare::PreparedRegisterBank::Fpr
            ? (expected_view == std::optional<abi::RegisterView>{abi::RegisterView::S}
                   ? std::optional<bir::TypeKind>{bir::TypeKind::F32}
               : expected_view == std::optional<abi::RegisterView>{abi::RegisterView::D}
                   ? std::optional<bir::TypeKind>{bir::TypeKind::F64}
                   : std::nullopt)
            : scalar_integer_type_from_size(width_bytes);
    if (!expected_view.has_value() || !value_type.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 stack call-result publication requires a scalar GPR, FPR, or binary128 vector result");
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
          : prepare::find_indexed_prepared_value_home(
                context.function.value_home_lookups,
                context.function.value_locations,
                move.to_value_id);
  auto classification =
      prepare::classify_prepared_call_boundary_move(call_plan, bundle, move);
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

  return AfterCallMoveLocalOwner::instruction(
      context,
      bundle,
      move,
      PreparedAfterCallMoveOwnerInputs{
          .destination_home = destination_home,
          .classification = std::move(classification),
          .f128_carriers = f128_carriers,
          .destination_f128_carrier = destination_f128_carrier,
      },
      instruction_index,
      diagnostics);
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
    if (auto source = StackFrameSlotCallOperandOwner::frame_slot_memory_from_endpoint(
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
      return StackFrameSlotCallOperandOwner::selected_frame_slot_source(
          context,
          argument,
          source_home,
          selection,
          expected_kind,
          false,
          instruction_index);
    case prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress:
      return StackFrameSlotCallOperandOwner::selected_frame_slot_source(
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
      return StackFrameSlotCallOperandOwner::selected_local_frame_address_source(
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

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_local_aggregate_address_payload(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const MemoryOperand& source,
    std::size_t before_instruction_index);

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
    lowered.push_back(make_outgoing_stack_adjustment_instruction(
        context,
        instruction_index,
        "sub sp, sp, #" + std::to_string(outgoing_bytes)));
    lowered.push_back(make_outgoing_stack_base_instruction(
        context,
        instruction_index,
        "mov " + std::string{abi::register_name(outgoing_stack_argument_base_register())} +
            ", sp"));
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
            lower_before_call_move(
                context, call_plan, *bundle, effect, move, instruction_index, diagnostics)) {
      const auto* move_record =
          std::get_if<CallBoundaryMoveInstructionRecord>(&instruction->target.payload);
      const auto classification =
          prepare::classify_prepared_call_boundary_move(call_plan, *bundle, move);
      if (move_record != nullptr &&
          move_record->source_memory.has_value() &&
          move_record->source_memory_materializes_address &&
          classification.argument_plan != nullptr) {
        if (auto payload = materialize_local_aggregate_address_payload(
                context,
                *classification.argument_plan,
                *move_record->source_memory,
                instruction_index)) {
          lowered.push_back(std::move(*payload));
        }
      }
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

  if (call_plan.result.has_value() &&
      call_plan.result->source_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      call_plan.result->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
      call_plan.result->destination_value_id.has_value() &&
      call_plan.result->source_register_name.has_value()) {
    const bool bundle_already_publishes_result =
        bundle != nullptr &&
        std::any_of(bundle->moves.begin(),
                    bundle->moves.end(),
                    [&](const prepare::PreparedMoveResolution& move) {
                      return move.destination_kind ==
                                 prepare::PreparedMoveDestinationKind::CallResultAbi &&
                             move.to_value_id == *call_plan.result->destination_value_id;
                    });
    if (!bundle_already_publishes_result) {
      prepare::PreparedMoveBundle synthetic_result_bundle{
          .function_name = context.function.control_flow != nullptr
                               ? context.function.control_flow->function_name
                               : c4c::kInvalidFunctionName,
          .phase = prepare::PreparedMovePhase::AfterCall,
          .block_index = context.block_index,
          .instruction_index = instruction_index,
      };
      synthetic_result_bundle.abi_bindings.push_back(prepare::PreparedAbiBinding{
          .destination_kind = prepare::PreparedMoveDestinationKind::CallResultAbi,
          .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
          .destination_abi_index = std::nullopt,
          .destination_register_name = call_plan.result->source_register_name,
          .destination_contiguous_width = call_plan.result->source_contiguous_width,
          .destination_occupied_register_names =
              call_plan.result->source_occupied_register_names,
          .destination_register_placement =
              call_plan.result->source_register_placement,
      });
      synthetic_result_bundle.moves.push_back(prepare::PreparedMoveResolution{
          .from_value_id = *call_plan.result->destination_value_id,
          .to_value_id = *call_plan.result->destination_value_id,
          .destination_kind = prepare::PreparedMoveDestinationKind::CallResultAbi,
          .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
          .destination_abi_index = std::nullopt,
          .destination_register_name = call_plan.result->source_register_name,
          .destination_contiguous_width = call_plan.result->source_contiguous_width,
          .destination_occupied_register_names =
              call_plan.result->source_occupied_register_names,
          .block_index = context.block_index,
          .instruction_index = instruction_index,
          .reason = "synthetic_stack_call_result_home_publication",
          .destination_register_placement =
              call_plan.result->source_register_placement,
      });
      if (auto instruction = lower_after_call_move(context,
                                                  call_plan,
                                                  synthetic_result_bundle,
                                                  synthetic_result_bundle.moves.front(),
                                                  instruction_index,
                                                  diagnostics)) {
        lowered.push_back(std::move(*instruction));
      }
    }
  }

  const std::size_t outgoing_bytes = outgoing_stack_argument_bytes(call_plan);
  if (outgoing_bytes > 0) {
    lowered.insert(lowered.begin(),
                   make_outgoing_stack_adjustment_instruction(
                       context,
                       instruction_index,
                       "add sp, sp, #" + std::to_string(outgoing_bytes)));
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
        prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                  context.function.value_locations,
                                                  move.from_value_id);
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
        prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                  context.function.value_locations,
                                                  move.to_value_id);
    if (destination_home == nullptr) {
      continue;
    }
    const auto* source_home =
        prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                  context.function.value_locations,
                                                  move.from_value_id);
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
      auto source = StackFrameSlotCallOperandOwner::prior_stack_preserved_value_source(
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

struct PreparedFrameSlotCallArgumentMove {
  const prepare::PreparedMoveBundle* bundle = nullptr;
  const prepare::PreparedMoveResolution* move = nullptr;
  const prepare::PreparedAbiBinding* binding = nullptr;
};

enum class LocalAggregateAddressMaterializationResult {
  NotRequired,
  Materialized,
  Failed,
};

[[nodiscard]] module::MachineInstruction make_dispatch_bridge_machine_instruction(
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

[[nodiscard]] std::vector<std::string> materialize_local_aggregate_address_lines(
    abi::RegisterReference address_register,
    const module::BlockLoweringContext& context,
    std::int64_t byte_offset) {
  if (byte_offset < 0) {
    return {};
  }
  const auto offset = static_cast<std::uint64_t>(byte_offset);
  const std::string_view base =
      context.function.frame_plan != nullptr &&
              context.function.frame_plan->uses_frame_pointer_for_fixed_slots
          ? "x29"
          : "sp";
  const std::string address_name = std::string{abi::register_name(address_register)};
  if (offset <= 4095U) {
    return {"add " + address_name + ", " + std::string{base} + ", #" +
            std::to_string(offset)};
  }
  auto lines = materialize_integer_constant_lines(address_register, offset, 64);
  if (lines.empty()) {
    return {};
  }
  lines.push_back("add " + address_name + ", " + std::string{base} + ", " +
                  address_name);
  return lines;
}

[[nodiscard]] bool append_global_byte_load_for_prepared_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t address_index,
    std::vector<std::string>& out,
    std::size_t source_byte_offset = 0) {
  if (context.bir_block == nullptr || value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return false;
  }
  for (std::size_t index = 0;
       index < std::min(before_instruction_index, context.bir_block->insts.size());
       ++index) {
    const auto* load =
        std::get_if<bir::LoadLocalInst>(&context.bir_block->insts[index]);
    if (load == nullptr || load->result.name != value.name ||
        !load->address.has_value() ||
        load->address->base_kind != bir::MemoryAddress::BaseKind::GlobalSymbol ||
        load->address->base_name.empty()) {
      continue;
    }
    const auto target = abi::register_name(abi::w_register(target_index));
    const auto address = abi::register_name(abi::x_register(address_index));
    const auto offset = load->address->byte_offset +
                        static_cast<std::int64_t>(source_byte_offset);
    out.push_back("adrp " + std::string{address} + ", " +
                  load->address->base_name);
    out.push_back("add " + std::string{address} + ", " +
                  std::string{address} + ", :lo12:" + load->address->base_name);
    out.push_back("ldrb " + std::string{target} + ", [" +
                  std::string{address} +
                  (offset == 0 ? std::string{}
                               : std::string{", #"} + std::to_string(offset)) +
                  "]");
    return true;
  }
  return false;
}

[[nodiscard]] std::optional<bir::Value> find_prepared_frame_byte_stored_value(
    const module::BlockLoweringContext& context,
    const prepare::PreparedAddressingFunction& addressing,
    std::int64_t stack_byte_offset,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr || context.control_flow_block == nullptr ||
      context.bir_block == nullptr || stack_byte_offset < 0) {
    return std::nullopt;
  }
  const auto wanted_stack_offset = static_cast<std::size_t>(stack_byte_offset);
  for (const auto& access : addressing.accesses) {
    if (access.block_label != context.control_flow_block->block_label ||
        access.inst_index >= before_instruction_index ||
        !access.stored_value_name.has_value() ||
        access.address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
        !access.address.frame_slot_id.has_value() ||
        access.address.byte_offset < 0 || access.address.size_bytes != 1) {
      continue;
    }
    const auto* slot =
        prepare::find_frame_slot_by_id(context.function.prepared->stack_layout,
                                       *access.address.frame_slot_id);
    if (slot == nullptr) {
      continue;
    }
    const auto access_stack_offset =
        slot->offset_bytes + static_cast<std::size_t>(access.address.byte_offset);
    if (access_stack_offset != wanted_stack_offset ||
        access.inst_index >= context.bir_block->insts.size()) {
      continue;
    }
    const auto* store =
        std::get_if<bir::StoreLocalInst>(&context.bir_block->insts[access.inst_index]);
    if (store == nullptr) {
      return std::nullopt;
    }
    const auto stored_name = prepared_named_value_id(context, store->value);
    if (!stored_name.has_value() || *stored_name != *access.stored_value_name) {
      return std::nullopt;
    }
    return store->value;
  }
  return std::nullopt;
}

struct PreparedFrameStoredValue {
  bir::Value value;
  std::size_t instruction_index = 0;
};

struct PreparedFrameByteStoredValue {
  bir::Value value;
  std::size_t instruction_index = 0;
  std::size_t byte_index = 0;
  std::size_t store_size_bytes = 0;
};

[[nodiscard]] std::optional<PreparedFrameStoredValue>
find_prepared_frame_stored_value(
    const module::BlockLoweringContext& context,
    const prepare::PreparedAddressingFunction& addressing,
    std::int64_t stack_byte_offset,
    std::size_t size_bytes,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr || context.control_flow_block == nullptr ||
      context.bir_block == nullptr || stack_byte_offset < 0 || size_bytes == 0) {
    return std::nullopt;
  }
  const auto wanted_stack_offset = static_cast<std::size_t>(stack_byte_offset);
  for (const auto& access : addressing.accesses) {
    if (access.block_label != context.control_flow_block->block_label ||
        access.inst_index >= before_instruction_index ||
        !access.stored_value_name.has_value() ||
        access.address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
        !access.address.frame_slot_id.has_value() ||
        access.address.byte_offset < 0 ||
        access.address.size_bytes != size_bytes) {
      continue;
    }
    const auto* slot =
        prepare::find_frame_slot_by_id(context.function.prepared->stack_layout,
                                       *access.address.frame_slot_id);
    if (slot == nullptr) {
      continue;
    }
    const auto access_stack_offset =
        slot->offset_bytes + static_cast<std::size_t>(access.address.byte_offset);
    if (access_stack_offset != wanted_stack_offset ||
        access.inst_index >= context.bir_block->insts.size()) {
      continue;
    }
    const auto* store =
        std::get_if<bir::StoreLocalInst>(&context.bir_block->insts[access.inst_index]);
    if (store == nullptr) {
      return std::nullopt;
    }
    const auto stored_name = prepared_named_value_id(context, store->value);
    if (!stored_name.has_value() || *stored_name != *access.stored_value_name) {
      return std::nullopt;
    }
    return PreparedFrameStoredValue{
        .value = store->value,
        .instruction_index = access.inst_index,
    };
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<PreparedFrameByteStoredValue>
find_prepared_frame_containing_byte_stored_value(
    const module::BlockLoweringContext& context,
    const prepare::PreparedAddressingFunction& addressing,
    std::int64_t stack_byte_offset,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr || context.control_flow_block == nullptr ||
      context.bir_block == nullptr || stack_byte_offset < 0) {
    return std::nullopt;
  }
  const auto wanted_stack_offset = static_cast<std::size_t>(stack_byte_offset);
  for (const auto& access : addressing.accesses) {
    if (access.block_label != context.control_flow_block->block_label ||
        access.inst_index >= before_instruction_index ||
        !access.stored_value_name.has_value() ||
        access.address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
        !access.address.frame_slot_id.has_value() ||
        access.address.byte_offset < 0 || access.address.size_bytes == 0 ||
        access.address.size_bytes > 8) {
      continue;
    }
    const auto* slot =
        prepare::find_frame_slot_by_id(context.function.prepared->stack_layout,
                                       *access.address.frame_slot_id);
    if (slot == nullptr) {
      continue;
    }
    const auto access_stack_offset =
        slot->offset_bytes + static_cast<std::size_t>(access.address.byte_offset);
    if (wanted_stack_offset < access_stack_offset ||
        wanted_stack_offset >= access_stack_offset + access.address.size_bytes ||
        access.inst_index >= context.bir_block->insts.size()) {
      continue;
    }
    const auto* store =
        std::get_if<bir::StoreLocalInst>(&context.bir_block->insts[access.inst_index]);
    if (store == nullptr) {
      return std::nullopt;
    }
    const auto stored_name = prepared_named_value_id(context, store->value);
    if (!stored_name.has_value() || *stored_name != *access.stored_value_name) {
      return std::nullopt;
    }
    return PreparedFrameByteStoredValue{
        .value = store->value,
        .instruction_index = access.inst_index,
        .byte_index = wanted_stack_offset - access_stack_offset,
        .store_size_bytes = access.address.size_bytes,
    };
  }
  return std::nullopt;
}

[[nodiscard]] bool append_frame_byte_store(std::int64_t stack_byte_offset,
                                           std::uint8_t value_index,
                                           std::uint8_t address_index,
                                           const module::BlockLoweringContext& context,
                                           std::vector<std::string>& out) {
  if (stack_byte_offset < 0) {
    return false;
  }
  const std::string value = std::string{abi::register_name(abi::w_register(value_index))};
  const std::uint64_t offset = static_cast<std::uint64_t>(stack_byte_offset);
  const std::string_view base =
      context.function.frame_plan != nullptr &&
              context.function.frame_plan->uses_frame_pointer_for_fixed_slots
          ? "x29"
          : "sp";
  if (offset <= 4095U) {
    out.push_back("strb " + value + ", [" + std::string{base} +
                  (offset == 0 ? std::string{}
                               : std::string{", #"} + std::to_string(offset)) +
                  "]");
    return true;
  }
  auto address = materialize_local_aggregate_address_lines(
      abi::x_register(address_index), context, stack_byte_offset);
  if (address.empty()) {
    return false;
  }
  out.insert(out.end(), address.begin(), address.end());
  out.push_back("strb " + value + ", [" +
                std::string{abi::register_name(abi::x_register(address_index))} + "]");
  return true;
}

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_local_aggregate_address_payload(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const MemoryOperand& source,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr || context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr || context.bir_block == nullptr ||
      source.byte_offset < 0) {
    return std::nullopt;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  if (addressing == nullptr) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.size() < 2) {
    return std::nullopt;
  }
  const std::uint8_t value_index = scratches.front().index;
  const std::uint8_t address_index = scratches[1].index;

  const auto semantic_byval_size = [&]() -> std::optional<std::size_t> {
    if (before_instruction_index >= context.bir_block->insts.size()) {
      return std::nullopt;
    }
    const auto* call =
        std::get_if<bir::CallInst>(&context.bir_block->insts[before_instruction_index]);
    if (call == nullptr || argument.arg_index >= call->arg_abi.size()) {
      return std::nullopt;
    }
    const auto& abi_info = call->arg_abi[argument.arg_index];
    if (!abi_info.byval_copy || abi_info.size_bytes == 0) {
      return std::nullopt;
    }
    return abi_info.size_bytes;
  }();
  if (!semantic_byval_size.has_value()) {
    return std::nullopt;
  }
  const std::size_t size_bytes = *semantic_byval_size;
  if (size_bytes == 0) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  for (std::size_t byte_offset = 0; byte_offset < size_bytes; ++byte_offset) {
    const auto stored_value =
        find_prepared_frame_containing_byte_stored_value(
            context,
            *addressing,
            source.byte_offset + static_cast<std::int64_t>(byte_offset),
            before_instruction_index);
    if (!stored_value.has_value()) {
      return std::nullopt;
    }
    const auto before_line_count = lines.size();
    const bool emitted_scalar =
        emit_value_publication_to_register(context,
                                           stored_value->value,
                                           stored_value->instruction_index,
                                           value_index,
                                           address_index,
                                           lines,
                                           true);
    if (!emitted_scalar &&
        !append_global_byte_load_for_prepared_value(context,
                                                   stored_value->value,
                                                   stored_value->instruction_index,
                                                   value_index,
                                                   address_index,
                                                   lines,
                                                   stored_value->byte_index)) {
      return std::nullopt;
    }
    if (emitted_scalar && stored_value->byte_index != 0) {
      lines.push_back("lsr " +
                      std::string{abi::register_name(abi::x_register(value_index))} +
                      ", " +
                      std::string{abi::register_name(abi::x_register(value_index))} +
                      ", #" + std::to_string(stored_value->byte_index * 8U));
    }
    if (lines.size() == before_line_count ||
        !append_frame_byte_store(source.byte_offset +
                                     static_cast<std::int64_t>(byte_offset),
                                 value_index,
                                 address_index,
                                 context,
                                 lines)) {
      return std::nullopt;
    }
  }
  if (lines.empty()) {
    return std::nullopt;
  }
  return make_select_chain_materialization_instruction(
      context, before_instruction_index, std::move(lines));
}

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_byval_stack_call_argument_source(
    const module::BlockLoweringContext& context,
    const MemoryOperand& source,
    const MemoryOperand& destination,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr || context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr || context.bir_block == nullptr ||
      source.byte_offset < 0 ||
      destination.base_kind != MemoryBaseKind::Register ||
      !destination.base_register.has_value() ||
      !abi::is_gp_register(destination.base_register->reg) ||
      source.size_bytes == 0 || source.size_bytes > 16) {
    return std::nullopt;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  if (addressing == nullptr) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.size() < 2) {
    return std::nullopt;
  }
  const std::uint8_t value_index = scratches.front().index;
  const std::uint8_t address_index = scratches[1].index;

  std::vector<std::string> lines;
  const auto destination_base = abi::register_name(destination.base_register->reg);
  for (std::size_t byte_offset = 0; byte_offset < source.size_bytes; ++byte_offset) {
    const auto stored_value =
        find_prepared_frame_containing_byte_stored_value(
            context,
            *addressing,
            source.byte_offset + static_cast<std::int64_t>(byte_offset),
            before_instruction_index);
    if (!stored_value.has_value()) {
      return std::nullopt;
    }
    const auto before_line_count = lines.size();
    const bool emitted_scalar =
        emit_value_publication_to_register(context,
                                           stored_value->value,
                                           stored_value->instruction_index,
                                           value_index,
                                           address_index,
                                           lines,
                                           true);
    if (!emitted_scalar &&
        !append_global_byte_load_for_prepared_value(context,
                                                   stored_value->value,
                                                   stored_value->instruction_index,
                                                   value_index,
                                                   address_index,
                                                   lines,
                                                   stored_value->byte_index)) {
      return std::nullopt;
    }
    if (emitted_scalar && stored_value->byte_index != 0) {
      lines.push_back("lsr " +
                      std::string{abi::register_name(abi::x_register(value_index))} +
                      ", " +
                      std::string{abi::register_name(abi::x_register(value_index))} +
                      ", #" + std::to_string(stored_value->byte_index * 8U));
    }
    if (lines.size() == before_line_count) {
      return std::nullopt;
    }
    lines.push_back("strb " + std::string{abi::register_name(abi::w_register(value_index))} +
                    ", " +
                    StackFrameSlotCallOperandOwner::stack_copy_address(
                        destination_base,
                        destination.byte_offset + static_cast<std::int64_t>(byte_offset)));
  }
  if (lines.empty()) {
    return std::nullopt;
  }
  return make_select_chain_materialization_instruction(
      context, before_instruction_index, std::move(lines));
}

[[nodiscard]] LocalAggregateAddressMaterializationResult
materialize_local_aggregate_address_call_argument(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t before_instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics,
    std::vector<module::MachineInstruction>& lowered,
    BlockScalarLoweringState& scalar_state) {
  if (!argument.allows_local_aggregate_address_publication) {
    return LocalAggregateAddressMaterializationResult::NotRequired;
  }
  const auto diagnostic = [&](std::string message) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        before_instruction_index,
        std::move(message));
    return LocalAggregateAddressMaterializationResult::Failed;
  };
  if (value.type != bir::TypeKind::Ptr) {
    return diagnostic(
        "AArch64 local aggregate address call argument requires a pointer argument");
  }
  auto result_register = make_named_prepared_result_register(context, value);
  if (!result_register.has_value() ||
      !abi::is_gp_register(result_register->reg)) {
    return diagnostic(
        "AArch64 local aggregate address call argument requires a prepared GPR result register");
  }
  const auto routing =
      prepare::find_prepared_call_argument_publication_source_routing(argument);
  if (routing.source_selection == nullptr ||
      routing.source_selection->kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::
              LocalFrameAddressMaterialization) {
    return diagnostic(
        "AArch64 local aggregate address call argument requires prepared LocalFrameAddressMaterialization source selection");
  }
  const auto* source_home =
      argument.source_value_id.has_value()
          ? prepare::find_indexed_prepared_value_home(
                context.function.value_home_lookups,
                context.function.value_locations,
                *argument.source_value_id)
          : prepare::find_indexed_prepared_value_home(
                context.function.value_home_lookups,
                context.function.regalloc,
                context.function.value_locations,
                result_register->value_name);
  const auto source = make_selected_call_argument_source(
      context,
      argument,
      source_home,
      *routing.source_selection,
      prepare::PreparedCallArgumentSourceSelectionKind::
          LocalFrameAddressMaterialization,
      before_instruction_index);
  if (!source.has_value()) {
    return diagnostic(
        "AArch64 local aggregate address call argument requires complete prepared LocalFrameAddressMaterialization source selection");
  }
  const auto address_register = abi::x_register(result_register->reg.index);
  result_register->reg = address_register;
  result_register->expected_view = abi::RegisterView::X;
  auto lines = materialize_local_aggregate_address_lines(
      address_register, context, source->byte_offset);
  if (lines.empty()) {
    return diagnostic(
        "AArch64 local aggregate address call argument requires a printable prepared frame address");
  }
  if (auto payload = materialize_local_aggregate_address_payload(
          context, argument, *source, before_instruction_index)) {
    lowered.push_back(std::move(*payload));
  }
  auto instruction =
      make_select_chain_materialization_instruction(
          context, before_instruction_index, std::move(lines));
  if (!instruction.has_value()) {
    return diagnostic(
        "AArch64 local aggregate address call argument requires a materializable prepared frame address");
  }
  record_emitted_scalar_register(scalar_state,
                                 result_register->value_name,
                                 *result_register);
  lowered.push_back(std::move(*instruction));
  return LocalAggregateAddressMaterializationResult::Materialized;
}

[[nodiscard]] const prepare::PreparedCallArgumentPlan* find_prepared_call_argument_plan(
    const prepare::PreparedCallPlan* call_plan,
    std::size_t argument_index) {
  if (call_plan == nullptr) {
    return nullptr;
  }
  for (const auto& argument : call_plan->arguments) {
    if (argument.arg_index == argument_index) {
      return &argument;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<PreparedFrameSlotCallArgumentMove>
find_prepared_frame_slot_call_argument_move(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index) {
  const auto* bundle = find_move_bundle(context,
                                        prepare::PreparedMovePhase::BeforeCall,
                                        context.block_index,
                                        instruction_index);
  if (bundle == nullptr) {
    return std::nullopt;
  }
  const auto* move =
      prepare::find_indexed_prepared_before_call_argument_move(
          context.function.move_bundle_lookups,
          call_plan.block_index,
          call_plan.instruction_index,
          argument.arg_index);
  if (move == nullptr) {
    for (const auto& candidate : bundle->moves) {
      const auto classification =
          prepare::classify_prepared_call_boundary_move(call_plan, *bundle, candidate);
      if (prepare::prepared_call_boundary_move_classification_available(classification) &&
          classification.argument_plan == &argument &&
          classification.destination_kind ==
              prepare::PreparedMoveDestinationKind::CallArgumentAbi) {
        move = &candidate;
        break;
      }
    }
    if (move == nullptr) {
      return std::nullopt;
    }
  }
  const auto classification =
      prepare::classify_prepared_call_boundary_move(call_plan, *bundle, *move);
  if (!prepare::prepared_call_boundary_move_classification_available(classification) ||
      classification.argument_plan != &argument ||
      classification.destination_kind !=
          prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      classification.storage_kind != prepare::PreparedMoveStorageKind::Register ||
      move->op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
      argument.source_value_id !=
          std::optional<prepare::PreparedValueId>{move->from_value_id}) {
    return std::nullopt;
  }
  return PreparedFrameSlotCallArgumentMove{
      .bundle = bundle,
      .move = move,
      .binding = classification.abi_binding,
  };
}

[[nodiscard]] std::optional<prepare::PreparedCallArgumentSourceProducerMaterialization>
find_prepared_scalar_call_argument_source_producer_materialization(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublicationSourceProducerLookups* source_producers,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      context.function.prepared == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  return prepare::find_prepared_call_argument_source_producer_materialization(
      context.function.prepared->names,
      source_producers,
      context.control_flow_block->block_label,
      context.bir_block,
      value,
      before_instruction_index);
}

[[nodiscard]] bool materialize_scalar_call_argument_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    const prepare::PreparedCallArgumentPlan* local_aggregate_address_argument,
    const prepare::PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics,
    std::vector<module::MachineInstruction>& lowered,
    std::vector<std::string_view>& active_values) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return true;
  }
  for (const auto active : active_values) {
    if (active == value.name) {
      return false;
    }
  }
  if (local_aggregate_address_argument != nullptr) {
    const auto local_address =
        materialize_local_aggregate_address_call_argument(
            context,
            value,
            *local_aggregate_address_argument,
            before_instruction_index,
            diagnostics,
            lowered,
            scalar_state);
    switch (local_address) {
      case LocalAggregateAddressMaterializationResult::Materialized:
        return true;
      case LocalAggregateAddressMaterializationResult::Failed:
        return false;
      case LocalAggregateAddressMaterializationResult::NotRequired:
        break;
    }
  }
  if (emitted_scalar_value_available(context, value, scalar_state)) {
    return true;
  }
  const auto materialization =
      find_prepared_scalar_call_argument_source_producer_materialization(
          context, source_producers, value, before_instruction_index);
  if (!materialization.has_value() || context.bir_block == nullptr) {
    if (auto prepared_register = make_named_prepared_result_register(context, value);
        prepared_register.has_value()) {
      record_emitted_scalar_register(scalar_state,
                                     prepared_register->value_name,
                                     *prepared_register);
    }
    return true;
  }
  const auto& producer = materialization->producer.producer;
  if (producer.kind == prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal) {
    return true;
  }
  if (producer.kind != prepare::PreparedEdgePublicationSourceProducerKind::Binary ||
      producer.binary == nullptr) {
    if (auto prepared_register = make_named_prepared_result_register(context, value);
        prepared_register.has_value()) {
      record_emitted_scalar_register(scalar_state,
                                     prepared_register->value_name,
                                     *prepared_register);
    }
    return true;
  }
  const auto producer_index = materialization->producer.instruction_index;
  if (producer_index >= context.bir_block->insts.size()) {
    return false;
  }
  const auto* binary =
      std::get_if<bir::BinaryInst>(&context.bir_block->insts[producer_index]);
  if (binary != producer.binary) {
    if (auto prepared_register = make_named_prepared_result_register(context, value);
        prepared_register.has_value()) {
      record_emitted_scalar_register(scalar_state,
                                     prepared_register->value_name,
                                     *prepared_register);
    }
    return true;
  }
  const auto binary_value_name = prepared_named_value_id(context, binary->result);
  if (!binary_value_name.has_value()) {
    return true;
  }
  if (const auto requested_value_name = prepared_named_value_id(context, value);
      !requested_value_name.has_value() ||
      *requested_value_name != *binary_value_name) {
    return true;
  }

  active_values.push_back(value.name);
  const bool lhs_ready =
      materialize_scalar_call_argument_value(context,
                                             binary->lhs,
                                             producer_index,
                                             nullptr,
                                             source_producers,
                                             scalar_state,
                                             diagnostics,
                                             lowered,
                                             active_values);
  const bool rhs_ready =
      materialize_scalar_call_argument_value(context,
                                             binary->rhs,
                                             producer_index,
                                             nullptr,
                                             source_producers,
                                             scalar_state,
                                             diagnostics,
                                             lowered,
                                             active_values);
  active_values.pop_back();
  if (!lhs_ready || !rhs_ready) {
    return false;
  }
  if (emitted_scalar_value_available(context, value, scalar_state)) {
    return true;
  }
  if (auto instruction = lower_scalar_instruction(context,
                                                  context.bir_block->insts[producer_index],
                                                  producer_index,
                                                  scalar_state,
                                                  diagnostics)) {
    const auto expected_result =
        make_named_prepared_result_register(context, binary->result);
    if (expected_result.has_value()) {
      if (auto* scalar =
              std::get_if<ScalarInstructionRecord>(&instruction->target.payload)) {
        scalar->result_register = *expected_result;
        if (scalar->scalar_alu.has_value()) {
          scalar->scalar_alu->result_register = *expected_result;
          if (const auto lhs_name = prepared_named_value_id(context, binary->lhs);
              lhs_name.has_value()) {
            if (const auto lhs_register =
                find_emitted_scalar_register(scalar_state, *lhs_name);
                lhs_register.has_value()) {
              auto lhs_operand = make_register_operand(*lhs_register);
              scalar->scalar_alu->lhs = lhs_operand;
              if (!scalar->inputs.empty()) {
                scalar->inputs[0] = std::move(lhs_operand);
              }
            }
          }
          if (const auto rhs_name = prepared_named_value_id(context, binary->rhs);
              rhs_name.has_value()) {
            if (const auto rhs_register =
                    find_emitted_scalar_register(scalar_state, *rhs_name);
                rhs_register.has_value()) {
              auto rhs_operand = make_register_operand(*rhs_register);
              scalar->scalar_alu->rhs = rhs_operand;
              if (scalar->inputs.size() > 1) {
                scalar->inputs[1] = std::move(rhs_operand);
              }
            }
          }
        }
        record_emitted_scalar_register(scalar_state,
                                       expected_result->value_name,
                                       *expected_result);
      }
    }
    lowered.push_back(std::move(*instruction));
    return true;
  }
  return false;
}

[[nodiscard]] bool is_inline_variadic_entry_helper_call(
    const prepare::PreparedCallPlan& call_plan) {
  return call_plan.direct_callee_name.has_value() &&
         prepare::prepared_variadic_entry_helper_kind_for_callee(
             *call_plan.direct_callee_name)
             .has_value();
}

[[nodiscard]] std::vector<module::MachineInstruction>
lower_scalar_call_argument_producers(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const std::vector<bir::Value>& arguments,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (is_inline_variadic_entry_helper_call(call_plan)) {
    return lowered;
  }
  std::optional<prepare::PreparedEdgePublicationSourceProducerLookups>
      fallback_source_producers;
  const auto* source_producers =
      context.function.prepared_lookups != nullptr
          ? &context.function.prepared_lookups->edge_publication_source_producers
          : nullptr;
  if (source_producers == nullptr &&
      context.function.prepared != nullptr &&
      context.function.control_flow != nullptr) {
    fallback_source_producers =
        prepare::make_prepared_edge_publication_source_producer_lookups(
            *context.function.prepared,
            *context.function.control_flow);
    source_producers = &*fallback_source_producers;
  }
  for (std::size_t argument_index = 0; argument_index < arguments.size(); ++argument_index) {
    const auto& argument = arguments[argument_index];
    std::vector<std::string_view> active_values;
    const auto* argument_plan =
        find_prepared_call_argument_plan(&call_plan, argument_index);
    if (auto select_chain =
            materialize_direct_global_select_chain_call_argument(context,
                                                                 argument,
                                                                 instruction_index,
                                                                 argument_plan,
                                                                 scalar_state)) {
      lowered.push_back(std::move(*select_chain));
      continue;
    }
    const auto* local_aggregate_address_argument =
        argument_plan != nullptr &&
                argument_plan->allows_local_aggregate_address_publication
            ? argument_plan
            : nullptr;
    if (!materialize_scalar_call_argument_value(context,
                                                argument,
                                                instruction_index,
                                                local_aggregate_address_argument,
                                                source_producers,
                                                scalar_state,
                                                diagnostics,
                                                lowered,
                                                active_values)) {
      return {};
    }
  }
  return lowered;
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_call_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (auto dynamic_stack = lower_dynamic_stack_helper_call(
          context, call_inst, instruction_index, diagnostics);
      dynamic_stack.has_value()) {
    return dynamic_stack;
  }

  const auto variadic_helper = variadic_entry_helper_kind(call_inst.callee);
  const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan = nullptr;
  if (variadic_helper.has_value()) {
    variadic_entry_plan =
        require_prepared_variadic_entry_plan(context, instruction_index, diagnostics);
    if (variadic_entry_plan == nullptr) {
      return std::nullopt;
    }
  }

  const auto* call_plan =
      require_prepared_call_plan(context, instruction_index, diagnostics);
  if (call_plan == nullptr) {
    return std::nullopt;
  }

  const prepare::PreparedVariadicEntryHelperOperandHomes* variadic_helper_operand_homes =
      nullptr;
  if (variadic_entry_plan != nullptr) {
    variadic_helper_operand_homes =
        prepare::find_prepared_variadic_entry_helper_operand_homes(
            *variadic_entry_plan, context.block_index, instruction_index);
    if (variadic_helper_operand_homes == nullptr ||
        !variadic_helper_operand_homes_complete(*variadic_helper_operand_homes)) {
      std::string message =
          "AArch64 variadic entry helper lowering requires prepared helper operand-home facts";
      const auto missing_consumption_fact =
          variadic_helper_missing_consumption_fact_message(*variadic_helper);
      if (!missing_consumption_fact.empty()) {
        message = missing_consumption_fact;
      }
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          std::move(message));
      return std::nullopt;
    }
  }

  return lower_prepared_call_instruction(context,
                                         call_inst,
                                         *call_plan,
                                         instruction_index,
                                         variadic_entry_plan,
                                         variadic_helper_operand_homes,
                                         variadic_helper,
                                         diagnostics);
}











[[nodiscard]] std::optional<bir::Value> prepared_call_boundary_source_value(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name,
    std::size_t before_instruction_index) {
  if (value_name == c4c::kInvalidValueName ||
      context.function.prepared_lookups == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
  }
  const auto* producer =
      prepare::find_indexed_prepared_edge_publication_source_producer(
          &context.function.prepared_lookups->edge_publication_source_producers,
          value_name);
  if (producer == nullptr ||
      producer->block_label != context.control_flow_block->block_label ||
      producer->instruction_index >= before_instruction_index) {
    return std::nullopt;
  }
  switch (producer->kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return producer->load_local != nullptr
                 ? std::optional<bir::Value>{producer->load_local->result}
                 : std::nullopt;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return producer->load_global != nullptr
                 ? std::optional<bir::Value>{producer->load_global->result}
                 : std::nullopt;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      return producer->cast != nullptr
                 ? std::optional<bir::Value>{producer->cast->result}
                 : std::nullopt;
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      return producer->binary != nullptr
                 ? std::optional<bir::Value>{producer->binary->result}
                 : std::nullopt;
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return producer->select != nullptr
                 ? std::optional<bir::Value>{producer->select->result}
                 : std::nullopt;
    case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<bir::Value> call_boundary_source_value_by_name(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name,
    std::size_t before_instruction_index) {
  if (auto source =
          prepared_call_boundary_source_value(context, value_name, before_instruction_index)) {
    return source;
  }
  if (value_name == c4c::kInvalidValueName ||
      context.function.prepared == nullptr ||
      context.bir_block == nullptr) {
    return std::nullopt;
  }
  const auto spelling =
      context.function.prepared->names.value_names.spelling(value_name);
  if (spelling.empty()) {
    return std::nullopt;
  }
  const auto* producer =
      mir::find_same_block_named_producer(context.bir_block, spelling, before_instruction_index);
  if (producer == nullptr) {
    return std::nullopt;
  }
  return std::visit(
      [](const auto& typed_inst) -> std::optional<bir::Value> {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                      std::is_same_v<T, bir::CastInst> ||
                      std::is_same_v<T, bir::SelectInst> ||
                      std::is_same_v<T, bir::LoadLocalInst> ||
                      std::is_same_v<T, bir::LoadGlobalInst>) {
          return typed_inst.result;
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          return typed_inst.result;
        }
        return std::nullopt;
      },
      *producer);
}

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_aggregate_register_lane_sources_to_destination(
    const module::BlockLoweringContext& context,
    const CallBoundaryMoveInstructionRecord& move_record,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  const auto view = aggregate_register_lane_publication_view(move_record);
  if (!view.has_value() || context.function.prepared == nullptr ||
      context.function.control_flow == nullptr || context.control_flow_block == nullptr ||
      context.bir_block == nullptr || view->source_memory->byte_offset < 0) {
    return std::nullopt;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  if (addressing == nullptr) {
    return std::nullopt;
  }

  const auto source_stack_offset =
      static_cast<std::size_t>(view->source_memory->byte_offset);
  const auto scratch_registers = abi::reserved_mir_scratch_gp_registers();
  if (scratch_registers.size() < 2) {
    return std::nullopt;
  }
  auto choose_auxiliary_scratch = [&](std::uint8_t target_index)
      -> std::optional<std::uint8_t> {
    for (const auto scratch : scratch_registers) {
      if (scratch.index != target_index) {
        return scratch.index;
      }
    }
    return std::nullopt;
  };

  std::vector<std::string> lines;
  for (std::size_t byte_offset = 0; byte_offset < view->size_bytes; ++byte_offset) {
    const auto lane_index = byte_offset / 8U;
    const auto lane_byte_offset = byte_offset % 8U;
    const auto lane_register =
        aggregate_register_lane_destination(*view->destination_register, lane_index);
    if (!lane_register.has_value()) {
      return std::nullopt;
    }
    const bool first_lane_byte = lane_byte_offset == 0;
    const std::uint8_t target_index =
        first_lane_byte ? lane_register->index : scratch_registers.front().index;
    const auto auxiliary_scratch = choose_auxiliary_scratch(target_index);
    if (!auxiliary_scratch.has_value()) {
      return std::nullopt;
    }
    const auto stored_value =
        find_prepared_frame_containing_byte_stored_value(
            context,
            *addressing,
            static_cast<std::int64_t>(source_stack_offset + byte_offset),
            instruction_index);
    if (!stored_value.has_value()) {
      if (first_lane_byte) {
        lines.push_back("mov " +
                        std::string{abi::register_name(abi::w_register(target_index))} +
                        ", #0");
      }
      continue;
    }
    const auto before_line_count = lines.size();
    const bool emitted_scalar =
        emit_value_publication_to_register(context,
                                           stored_value->value,
                                           stored_value->instruction_index,
                                           target_index,
                                           *auxiliary_scratch,
                                           lines,
                                           true);
    if (!emitted_scalar &&
        !append_global_byte_load_for_prepared_value(context,
                                                   stored_value->value,
                                                   stored_value->instruction_index,
                                                   target_index,
                                                   *auxiliary_scratch,
                                                   lines,
                                                   stored_value->byte_index)) {
      return std::nullopt;
    }
    if (emitted_scalar && stored_value->byte_index != 0) {
      lines.push_back("lsr " +
                      std::string{abi::register_name(abi::x_register(target_index))} +
                      ", " +
                      std::string{abi::register_name(abi::x_register(target_index))} +
                      ", #" + std::to_string(stored_value->byte_index * 8U));
    }
    if (lines.size() == before_line_count) {
      return std::nullopt;
    }
    if (!first_lane_byte) {
      lines.push_back("orr " + std::string{abi::register_name(*lane_register)} +
                      ", " + std::string{abi::register_name(*lane_register)} +
                      ", " +
                      std::string{abi::register_name(abi::x_register(target_index))} +
                      ", lsl #" + std::to_string(lane_byte_offset * 8U));
    }
  }
  if (lines.empty()) {
    return std::nullopt;
  }
  if (move_record.destination_register.has_value() &&
      move_record.destination_register->value_name != c4c::kInvalidValueName) {
    record_emitted_scalar_register(scalar_state,
                                   move_record.destination_register->value_name,
                                   *move_record.destination_register);
  }
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}

[[nodiscard]] bool emit_fp_call_boundary_value_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    abi::RegisterReference destination,
    std::uint8_t gp_scratch_index,
    std::vector<std::string>& lines,
    unsigned depth = 0) {
  if (depth > 8U) {
    return false;
  }
  const auto destination_view = scalar_fp_register_view(destination, value.type);
  const auto size_bytes =
      value.type == bir::TypeKind::F32 ? std::optional<std::size_t>{4}
      : value.type == bir::TypeKind::F64 ? std::optional<std::size_t>{8}
                                         : std::nullopt;
  const auto* producer =
      value.kind == bir::Value::Kind::Named && context.bir_block != nullptr
          ? mir::find_same_block_named_producer(
                context.bir_block, value.name, before_instruction_index)
          : nullptr;
  const auto* load_local =
      producer != nullptr ? std::get_if<bir::LoadLocalInst>(producer) : nullptr;
  const auto producer_index =
      producer != nullptr ? mir::producer_instruction_index(context.bir_block, producer)
                          : std::nullopt;
  if (destination_view.has_value() && load_local != nullptr &&
      load_local->address.has_value() &&
      load_local->address->base_kind == bir::MemoryAddress::BaseKind::GlobalSymbol &&
      !load_local->address->base_name.empty()) {
    const auto address = abi::x_register(gp_scratch_index);
    const auto offset = load_local->address->byte_offset;
    lines.push_back("adrp " + std::string{abi::register_name(address)} + ", " +
                    load_local->address->base_name);
    lines.push_back("add " + std::string{abi::register_name(address)} + ", " +
                    std::string{abi::register_name(address)} + ", :lo12:" +
                    load_local->address->base_name);
    lines.push_back("ldr " + std::string{abi::register_name(*destination_view)} +
                    ", [" + std::string{abi::register_name(address)} +
                    (offset == 0 ? std::string{}
                                 : std::string{", #"} + std::to_string(offset)) +
                    "]");
    return true;
  }
  const auto source_offset =
      load_local != nullptr && producer_index.has_value()
          ? prepared_local_load_offset(context, *producer_index)
          : std::nullopt;
  const auto* addressing =
      context.function.prepared != nullptr && context.function.control_flow != nullptr
          ? prepare::find_prepared_addressing(
                *context.function.prepared,
                context.function.control_flow->function_name)
          : nullptr;
  if (destination_view.has_value() && size_bytes.has_value() &&
      source_offset.has_value() && addressing != nullptr) {
    if (auto stored =
            find_prepared_frame_stored_value(context,
                                             *addressing,
                                             static_cast<std::int64_t>(*source_offset),
                                             *size_bytes,
                                             *producer_index)) {
      bir::Value stored_value = stored->value;
      stored_value.type = value.type;
      if (emit_fp_call_boundary_value_to_register(context,
                                                  stored_value,
                                                  stored->instruction_index,
                                                  *destination_view,
                                                  gp_scratch_index,
                                                  lines,
                                                  depth + 1)) {
        return true;
      }
    }
  }
  return emit_fp_value_to_register(context,
                                   value,
                                   before_instruction_index,
                                   destination,
                                   gp_scratch_index,
                                   lines);
}

[[nodiscard]] bool emit_f128_call_boundary_value_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    abi::RegisterReference destination,
    std::uint8_t gp_scratch_index,
    std::vector<std::string>& lines,
    unsigned depth = 0) {
  if (depth > 8U || value.type != bir::TypeKind::F128 ||
      !abi::is_fp_simd_register(destination)) {
    return false;
  }
  const auto destination_q =
      abi::fp_simd_register(destination.index, abi::RegisterView::Q);
  if (!destination_q.has_value()) {
    return false;
  }
  const auto* producer =
      value.kind == bir::Value::Kind::Named && context.bir_block != nullptr
          ? mir::find_same_block_named_producer(
                context.bir_block, value.name, before_instruction_index)
          : nullptr;
  const auto producer_index =
      producer != nullptr ? mir::producer_instruction_index(context.bir_block, producer)
                          : std::nullopt;
  const auto* load_local =
      producer != nullptr ? std::get_if<bir::LoadLocalInst>(producer) : nullptr;
  if (load_local != nullptr &&
      load_local->address.has_value() &&
      load_local->address->base_kind == bir::MemoryAddress::BaseKind::GlobalSymbol &&
      !load_local->address->base_name.empty()) {
    const auto address = abi::x_register(gp_scratch_index);
    const auto offset = load_local->address->byte_offset;
    lines.push_back("adrp " + std::string{abi::register_name(address)} + ", " +
                    load_local->address->base_name);
    lines.push_back("add " + std::string{abi::register_name(address)} + ", " +
                    std::string{abi::register_name(address)} + ", :lo12:" +
                    load_local->address->base_name);
    lines.push_back("ldr " + std::string{abi::register_name(*destination_q)} +
                    ", [" + std::string{abi::register_name(address)} +
                    (offset == 0 ? std::string{}
                                 : std::string{", #"} + std::to_string(offset)) +
                    "]");
    return true;
  }
  const auto source_offset =
      load_local != nullptr && producer_index.has_value()
          ? prepared_local_load_offset(context, *producer_index)
          : std::nullopt;
  const auto* addressing =
      context.function.prepared != nullptr && context.function.control_flow != nullptr
          ? prepare::find_prepared_addressing(
                *context.function.prepared,
                context.function.control_flow->function_name)
          : nullptr;
  if (source_offset.has_value() && addressing != nullptr) {
    if (auto stored =
            find_prepared_frame_stored_value(context,
                                             *addressing,
                                             static_cast<std::int64_t>(*source_offset),
                                             16U,
                                             producer_index.value_or(before_instruction_index))) {
      bir::Value stored_value = stored->value;
      stored_value.type = bir::TypeKind::F128;
      if (emit_f128_call_boundary_value_to_register(context,
                                                    stored_value,
                                                    stored->instruction_index,
                                                    *destination_q,
                                                    gp_scratch_index,
                                                    lines,
                                                    depth + 1)) {
        return true;
      }
    }
  }
  const auto* load_global =
      producer != nullptr ? std::get_if<bir::LoadGlobalInst>(producer) : nullptr;
  if (load_global != nullptr) {
    const auto address = abi::x_register(gp_scratch_index);
    if (load_global->global_name.empty()) {
      return false;
    }
    const bir::Global* target_global = find_load_global_target(context, *load_global);
    const auto symbol_label = load_global_symbol_label(context, *load_global, target_global);
    if (symbol_label.empty()) {
      return false;
    }
    if (target_global != nullptr &&
        target_global->address_materialization_policy ==
            bir::GlobalAddressMaterializationPolicy::GotRequired) {
      lines.push_back("adrp " + std::string{abi::register_name(address)} + ", :got:" +
                      symbol_label);
      lines.push_back("ldr " + std::string{abi::register_name(address)} + ", [" +
                      std::string{abi::register_name(address)} + ", :got_lo12:" +
                      symbol_label + "]");
      lines.push_back("ldr " + std::string{abi::register_name(*destination_q)} + ", " +
                      register_indirect_address(abi::register_name(address),
                                                load_global->byte_offset));
      return true;
    }
    const auto symbol = relocation_operand(symbol_label, load_global->byte_offset);
    lines.push_back("adrp " + std::string{abi::register_name(address)} + ", " + symbol);
    lines.push_back("add " + std::string{abi::register_name(address)} + ", " +
                    std::string{abi::register_name(address)} + ", :lo12:" + symbol);
    lines.push_back("ldr " + std::string{abi::register_name(*destination_q)} + ", [" +
                    std::string{abi::register_name(address)} + "]");
    return true;
  }
  const auto* home = prepared_value_home_for_value(context, value);
  if (home == nullptr) {
    return false;
  }
  if (home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      home->offset_bytes.has_value()) {
    lines.push_back("ldr " + std::string{abi::register_name(*destination_q)} +
                    ", " + frame_slot_address(context.function, *home->offset_bytes));
    return true;
  }
  if (home->kind == prepare::PreparedValueHomeKind::Register &&
      home->register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
    if (!parsed.has_value() || !abi::is_fp_simd_register(*parsed)) {
      return false;
    }
    const auto source = abi::fp_simd_register(parsed->index, abi::RegisterView::Q);
    if (!source.has_value()) {
      return false;
    }
    const auto source_v = abi::fp_simd_register(source->index, abi::RegisterView::V);
    const auto destination_v =
        abi::fp_simd_register(destination_q->index, abi::RegisterView::V);
    if (!source_v.has_value() || !destination_v.has_value()) {
      return false;
    }
    const std::string source_name = abi::register_name(*source_v);
    const std::string destination_name = abi::register_name(*destination_v);
    if (source_v->index != destination_v->index) {
      lines.push_back("mov " + destination_name + ".16b, " + source_name + ".16b");
    }
    return true;
  }
  return false;
}

[[nodiscard]] std::optional<std::vector<std::string>>
f128_call_boundary_register_move_lines(abi::RegisterReference source,
                                       abi::RegisterReference destination) {
  if (!abi::is_fp_simd_register(source) || !abi::is_fp_simd_register(destination)) {
    return std::nullopt;
  }
  const auto source_v = abi::fp_simd_register(source.index, abi::RegisterView::V);
  const auto destination_v =
      abi::fp_simd_register(destination.index, abi::RegisterView::V);
  if (!source_v.has_value() || !destination_v.has_value()) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  if (source_v->index != destination_v->index) {
    lines.push_back("mov " + std::string{abi::register_name(*destination_v)} +
                    ".16b, " + std::string{abi::register_name(*source_v)} +
                    ".16b");
  }
  return lines;
}

[[nodiscard]] bool f128_call_boundary_value_traces_to_global(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    unsigned depth = 0) {
  if (depth > 8U || value.type != bir::TypeKind::F128 ||
      value.kind != bir::Value::Kind::Named || context.bir_block == nullptr) {
    return false;
  }
  const auto* producer =
      mir::find_same_block_named_producer(
          context.bir_block, value.name, before_instruction_index);
  const auto producer_index =
      producer != nullptr ? mir::producer_instruction_index(context.bir_block, producer)
                          : std::nullopt;
  if (producer == nullptr || !producer_index.has_value()) {
    return false;
  }
  if (std::get_if<bir::LoadGlobalInst>(producer) != nullptr) {
    return true;
  }
  const auto* load_local = std::get_if<bir::LoadLocalInst>(producer);
  if (load_local != nullptr &&
      load_local->address.has_value() &&
      load_local->address->base_kind == bir::MemoryAddress::BaseKind::GlobalSymbol &&
      !load_local->address->base_name.empty()) {
    return true;
  }
  const auto source_offset =
      load_local != nullptr ? prepared_local_load_offset(context, *producer_index)
                            : std::nullopt;
  const auto* addressing =
      context.function.prepared != nullptr && context.function.control_flow != nullptr
          ? prepare::find_prepared_addressing(
                *context.function.prepared,
                context.function.control_flow->function_name)
          : nullptr;
  if (!source_offset.has_value() || addressing == nullptr) {
    return false;
  }
  const auto stored = find_prepared_frame_stored_value(
      context,
      *addressing,
      static_cast<std::int64_t>(*source_offset),
      16U,
      *producer_index);
  return stored.has_value() &&
         f128_call_boundary_value_traces_to_global(context,
                                                   stored->value,
                                                   stored->instruction_index,
                                                   depth + 1);
}

[[nodiscard]] bool fp_call_boundary_value_traces_to_global(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    unsigned depth = 0) {
  const auto size_bytes =
      value.type == bir::TypeKind::F32 ? std::optional<std::size_t>{4}
      : value.type == bir::TypeKind::F64 ? std::optional<std::size_t>{8}
                                         : std::nullopt;
  if (depth > 8U || !size_bytes.has_value() ||
      value.kind != bir::Value::Kind::Named || context.bir_block == nullptr) {
    return false;
  }
  const auto* producer =
      mir::find_same_block_named_producer(
          context.bir_block, value.name, before_instruction_index);
  const auto producer_index =
      producer != nullptr ? mir::producer_instruction_index(context.bir_block, producer)
                          : std::nullopt;
  if (producer == nullptr || !producer_index.has_value()) {
    return false;
  }
  if (std::get_if<bir::LoadGlobalInst>(producer) != nullptr) {
    return true;
  }
  const auto* load_local = std::get_if<bir::LoadLocalInst>(producer);
  if (load_local != nullptr &&
      load_local->address.has_value() &&
      load_local->address->base_kind == bir::MemoryAddress::BaseKind::GlobalSymbol &&
      !load_local->address->base_name.empty()) {
    return true;
  }
  const auto source_offset =
      load_local != nullptr ? prepared_local_load_offset(context, *producer_index)
                            : std::nullopt;
  const auto* addressing =
      context.function.prepared != nullptr && context.function.control_flow != nullptr
          ? prepare::find_prepared_addressing(
                *context.function.prepared,
                context.function.control_flow->function_name)
          : nullptr;
  if (!source_offset.has_value() || addressing == nullptr) {
    return false;
  }
  const auto stored = find_prepared_frame_stored_value(
      context,
      *addressing,
      static_cast<std::int64_t>(*source_offset),
      *size_bytes,
      *producer_index);
  if (!stored.has_value()) {
    return false;
  }
  bir::Value stored_value = stored->value;
  stored_value.type = value.type;
  return fp_call_boundary_value_traces_to_global(context,
                                                 stored_value,
                                                 stored->instruction_index,
                                                 depth + 1);
}

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_fp_stack_call_argument_source(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const MemoryOperand& source,
    const MemoryOperand& destination) {
  if (!source.result_value_name.has_value() ||
      destination.base_kind != MemoryBaseKind::Register ||
      !destination.base_register.has_value() ||
      !abi::is_gp_register(destination.base_register->reg)) {
    return std::nullopt;
  }
  const auto source_value =
      call_boundary_source_value_by_name(context, *source.result_value_name, instruction_index);
  if (!source_value.has_value() ||
      (source_value->type != bir::TypeKind::F32 &&
       source_value->type != bir::TypeKind::F64)) {
    return std::nullopt;
  }
  const auto fp_scratches = abi::reserved_mir_scratch_fp_simd_registers();
  const auto gp_scratches = abi::reserved_mir_scratch_gp_registers();
  if (fp_scratches.empty() || gp_scratches.empty()) {
    return std::nullopt;
  }
  const auto view = source_value->type == bir::TypeKind::F32 ? abi::RegisterView::S
                                                            : abi::RegisterView::D;
  const auto fp_scratch =
      abi::fp_simd_register(fp_scratches.front().index, view);
  if (!fp_scratch.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  if (!emit_fp_call_boundary_value_to_register(context,
                                               *source_value,
                                               instruction_index,
                                               *fp_scratch,
                                               gp_scratches.front().index,
                                               lines) ||
      lines.empty()) {
    return std::nullopt;
  }
  lines.push_back("str " + std::string{abi::register_name(*fp_scratch)} + ", " +
                  StackFrameSlotCallOperandOwner::stack_copy_address(
                      abi::register_name(destination.base_register->reg),
                      destination.byte_offset));

  std::string asm_text;
  for (const auto& line : lines) {
    if (!asm_text.empty()) {
      asm_text += '\n';
    }
    asm_text += line;
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
materialize_f128_stack_call_argument_source(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const MemoryOperand& source,
    const MemoryOperand& destination) {
  if (!source.result_value_name.has_value() ||
      destination.base_kind != MemoryBaseKind::Register ||
      !destination.base_register.has_value() ||
      !abi::is_gp_register(destination.base_register->reg)) {
    return std::nullopt;
  }
  const auto source_value =
      call_boundary_source_value_by_name(context, *source.result_value_name, instruction_index);
  if (!source_value.has_value() || source_value->type != bir::TypeKind::F128) {
    return std::nullopt;
  }
  const auto fp_scratches = abi::reserved_mir_scratch_fp_simd_registers();
  const auto gp_scratches = abi::reserved_mir_scratch_gp_registers();
  if (fp_scratches.empty() || gp_scratches.empty()) {
    return std::nullopt;
  }
  const auto q_scratch =
      abi::fp_simd_register(fp_scratches.front().index, abi::RegisterView::Q);
  if (!q_scratch.has_value()) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  if (!emit_f128_call_boundary_value_to_register(context,
                                                 *source_value,
                                                 instruction_index,
                                                 *q_scratch,
                                                 gp_scratches.front().index,
                                                 lines) ||
      lines.empty()) {
    return std::nullopt;
  }
  lines.push_back("str " + std::string{abi::register_name(*q_scratch)} + ", " +
                  StackFrameSlotCallOperandOwner::stack_copy_address(
                      abi::register_name(destination.base_register->reg),
                      destination.byte_offset));
  std::string asm_text;
  for (const auto& line : lines) {
    if (!asm_text.empty()) {
      asm_text += '\n';
    }
    asm_text += line;
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
materialize_call_boundary_source_to_destination(
    const module::BlockLoweringContext& context,
    module::MachineInstruction& instruction,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record != nullptr) {
    if (auto aggregate_materialized =
            materialize_aggregate_register_lane_sources_to_destination(
                context, *move_record, instruction_index, scalar_state)) {
      return aggregate_materialized;
    }
  }
  if (move_record != nullptr &&
      move_record->phase == prepare::PreparedMovePhase::BeforeCall &&
      move_record->move.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move_record->source_register.has_value() &&
      move_record->destination_register.has_value() &&
      move_record->source_register->prepared_bank == prepare::PreparedRegisterBank::Fpr &&
      move_record->destination_register->prepared_bank ==
          prepare::PreparedRegisterBank::Fpr &&
      abi::is_fp_simd_register(move_record->destination_register->reg)) {
    const auto source_value =
        call_boundary_source_value_by_name(
            context, move_record->source_register->value_name, instruction_index);
    if (move_record->source_register->value_name != c4c::kInvalidValueName) {
      const auto emitted =
          find_emitted_scalar_register(scalar_state,
                                       move_record->source_register->value_name);
      if (emitted.has_value() &&
          emitted->reg.bank == abi::RegisterBank::FpSimd &&
          register_operands_share_physical_register(
              *emitted, *move_record->source_register) &&
          (!source_value.has_value() ||
           !fp_call_boundary_value_traces_to_global(
               context, *source_value, instruction_index))) {
        return std::nullopt;
      }
    }
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    std::vector<std::string> lines;
    if (source_value.has_value() && !scratches.empty() &&
        emit_fp_call_boundary_value_to_register(context,
                                                *source_value,
                                                instruction_index,
                                                move_record->destination_register->reg,
                                                scratches.front().index,
                                                lines) &&
        !lines.empty()) {
      record_emitted_scalar_register(scalar_state,
                                     move_record->destination_register->value_name,
                                     *move_record->destination_register);
      return make_select_chain_materialization_instruction(
          context, instruction_index, std::move(lines));
    }
  }
  if (move_record != nullptr &&
      move_record->phase == prepare::PreparedMovePhase::BeforeCall &&
      move_record->move.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move_record->source_register.has_value() &&
      move_record->destination_register.has_value() &&
      move_record->source_register->prepared_bank == prepare::PreparedRegisterBank::Vreg &&
      move_record->destination_register->prepared_bank ==
          prepare::PreparedRegisterBank::Vreg &&
      move_record->destination_register->expected_view == abi::RegisterView::Q &&
      move_record->source_f128_carrier != nullptr &&
      abi::is_fp_simd_register(move_record->destination_register->reg)) {
    const auto source_value =
        call_boundary_source_value_by_name(
            context, move_record->source_register->value_name, instruction_index);
    if (move_record->source_f128_carrier->kind ==
            prepare::PreparedF128CarrierKind::FullWidthRegister &&
        move_record->source_f128_carrier->missing_required_facts.empty() &&
        move_record->source_f128_carrier->total_size_bytes == 16 &&
        move_record->source_f128_carrier->total_align_bytes == 16 &&
        abi::is_fp_simd_register(move_record->source_register->reg)) {
      if (!source_value.has_value() ||
          !f128_call_boundary_value_traces_to_global(
              context, *source_value, instruction_index)) {
        if (move_record->source_f128_carrier->register_name.has_value()) {
          const auto carrier_register = abi::parse_aarch64_register_name(
              *move_record->source_f128_carrier->register_name);
          if (carrier_register.has_value() &&
              carrier_register->view != abi::RegisterView::Q) {
            if (auto lines = f128_call_boundary_register_move_lines(
                    move_record->source_register->reg,
                    move_record->destination_register->reg);
                lines.has_value() && !lines->empty()) {
              record_emitted_scalar_register(
                  scalar_state,
                  move_record->destination_register->value_name,
                  *move_record->destination_register);
              return make_select_chain_materialization_instruction(
                  context, instruction_index, std::move(*lines));
            }
          }
        }
        return std::nullopt;
      }
    }
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    std::vector<std::string> lines;
    if (source_value.has_value() && !scratches.empty() &&
        emit_f128_call_boundary_value_to_register(context,
                                                  *source_value,
                                                  instruction_index,
                                                  move_record->destination_register->reg,
                                                  scratches.front().index,
                                                  lines) &&
        !lines.empty()) {
      record_emitted_scalar_register(scalar_state,
                                     move_record->destination_register->value_name,
                                     *move_record->destination_register);
      return make_select_chain_materialization_instruction(
          context, instruction_index, std::move(lines));
    }
  }
  if (move_record != nullptr &&
      move_record->phase == prepare::PreparedMovePhase::BeforeCall &&
      move_record->move.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move_record->source_memory.has_value() &&
      !move_record->source_memory_materializes_address &&
      move_record->destination_register.has_value() &&
      move_record->destination_register->prepared_bank ==
          prepare::PreparedRegisterBank::Fpr &&
      abi::is_fp_simd_register(move_record->destination_register->reg)) {
    const auto source_value_name =
        move_record->source_memory->result_value_name.has_value()
            ? *move_record->source_memory->result_value_name
            : move_record->destination_register->value_name;
    const auto source_value =
        call_boundary_source_value_by_name(context, source_value_name, instruction_index);
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    std::vector<std::string> lines;
    if (source_value.has_value() && !scratches.empty() &&
        emit_fp_call_boundary_value_to_register(context,
                                                *source_value,
                                                instruction_index,
                                                move_record->destination_register->reg,
                                                scratches.front().index,
                                                lines) &&
        !lines.empty()) {
      record_emitted_scalar_register(scalar_state,
                                     move_record->destination_register->value_name,
                                     *move_record->destination_register);
      return make_select_chain_materialization_instruction(
          context, instruction_index, std::move(lines));
    }
  }
  if (move_record != nullptr &&
      move_record->phase == prepare::PreparedMovePhase::BeforeCall &&
      move_record->move.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move_record->source_memory.has_value() &&
      !move_record->source_memory_materializes_address &&
      move_record->destination_register.has_value() &&
      move_record->destination_register->prepared_bank ==
          prepare::PreparedRegisterBank::Vreg &&
      move_record->destination_register->expected_view == abi::RegisterView::Q &&
      abi::is_fp_simd_register(move_record->destination_register->reg)) {
    const auto source_value_name =
        move_record->source_memory->result_value_name.has_value()
            ? *move_record->source_memory->result_value_name
            : move_record->destination_register->value_name;
    const auto source_value =
        call_boundary_source_value_by_name(context, source_value_name, instruction_index);
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    std::vector<std::string> lines;
    if (source_value.has_value() && !scratches.empty() &&
        emit_f128_call_boundary_value_to_register(context,
                                                  *source_value,
                                                  instruction_index,
                                                  move_record->destination_register->reg,
                                                  scratches.front().index,
                                                  lines) &&
        !lines.empty()) {
      record_emitted_scalar_register(scalar_state,
                                     move_record->destination_register->value_name,
                                     *move_record->destination_register);
      return make_select_chain_materialization_instruction(
          context, instruction_index, std::move(lines));
    }
  }
  if (move_record == nullptr ||
      !move_record->source_memory.has_value() ||
      move_record->source_memory_materializes_address ||
      !move_record->destination_register.has_value() ||
      !move_record->source_memory->result_value_name.has_value() ||
      move_record->source_memory->base_kind != MemoryBaseKind::FrameSlot ||
      find_emitted_scalar_register(
          scalar_state, *move_record->source_memory->result_value_name).has_value() ||
      !abi::is_gp_register(move_record->destination_register->reg)) {
    return std::nullopt;
  }

  const auto source_value =
      prepared_call_boundary_source_value(
          context, *move_record->source_memory->result_value_name, instruction_index);
  if (!source_value.has_value()) {
    return std::nullopt;
  }

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  const std::uint8_t target_index = move_record->destination_register->reg.index;
  const std::uint8_t scratch_index =
      scratches.front().index == target_index && scratches.size() > 1
          ? scratches[1].index
          : scratches.front().index;
  if (scratch_index == target_index) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  if (!emit_value_publication_to_register(context,
                                          *source_value,
                                          instruction_index,
                                          target_index,
                                          scratch_index,
                                          lines) ||
      lines.empty()) {
    return std::nullopt;
  }
  RegisterOperand emitted = *move_record->destination_register;
  emitted.value_id = move_record->source_memory->result_value_id;
  emitted.value_name = *move_record->source_memory->result_value_name;
  record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));

}

void retarget_call_boundary_source_to_emitted_scalar(
    module::MachineInstruction& instruction,
    const BlockScalarLoweringState& scalar_state) {
  auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr) {
    return;
  }
  std::optional<c4c::ValueNameId> source_value_name;
  std::optional<prepare::PreparedValueId> source_value_id;
  if (move_record->source_memory.has_value() &&
      !move_record->source_memory_materializes_address &&
      move_record->source_memory->result_value_name.has_value()) {
    source_value_name = *move_record->source_memory->result_value_name;
    source_value_id = move_record->source_memory->result_value_id;
  } else if (move_record->source_register.has_value() &&
             move_record->source_register->value_name != c4c::kInvalidValueName) {
    source_value_name = move_record->source_register->value_name;
    source_value_id = move_record->source_register->value_id;
  } else if (move_record->source_register.has_value()) {
    source_value_id = move_record->source_register->value_id;
  }
  if (!source_value_name.has_value() && !source_value_id.has_value()) {
    return;
  }
  auto emitted = source_value_name.has_value()
                     ? find_emitted_scalar_register(scalar_state, *source_value_name)
                     : std::nullopt;
  if (!emitted.has_value() && source_value_id.has_value()) {
    const bool floating_preserved_source =
        move_record->source_register.has_value() &&
        move_record->source_register->reg.bank == abi::RegisterBank::FpSimd;
    if (floating_preserved_source) {
      for (const auto& [_, candidate] : scalar_state.emitted_registers) {
        if (candidate.value_id == source_value_id &&
            candidate.reg.bank == abi::RegisterBank::FpSimd) {
          emitted = candidate;
          break;
        }
      }
    }
  }
  if (!emitted.has_value()) {
    return;
  }
  move_record->source_register = *emitted;
  move_record->source_memory.reset();
  if (move_record->destination_register.has_value() &&
      emitted->reg.bank == abi::RegisterBank::GeneralPurpose &&
      move_record->destination_register->reg.bank == abi::RegisterBank::GeneralPurpose &&
      emitted->expected_view.has_value()) {
    const auto retargeted_destination =
        abi::gp_register(move_record->destination_register->reg.index,
                         *emitted->expected_view);
    if (retargeted_destination.has_value()) {
      move_record->destination_register->reg = *retargeted_destination;
      move_record->destination_register->expected_view = emitted->expected_view;
    }
  }
}

void record_call_boundary_destination(
    const module::MachineInstruction& instruction,
    BlockScalarLoweringState& scalar_state) {
  const auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record != nullptr && move_record->destination_register.has_value()) {
    record_emitted_scalar_register(scalar_state,
                                   move_record->destination_register->value_name,
                                   *move_record->destination_register);
  }
}

[[nodiscard]] bool call_boundary_register_source_in_destination_available(
    const CallBoundaryMoveInstructionRecord& move_record) {
  if (!move_record.source_register.has_value() ||
      !move_record.destination_register.has_value() ||
      !move_record.source_register->value_id.has_value()) {
    return false;
  }

  const prepare::PreparedCallResultPlan result{
      .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_value_id = *move_record.source_register->value_id,
      .source_register_name =
          std::string(abi::register_name(move_record.source_register->reg)),
      .source_register_bank = move_record.source_register->prepared_bank,
      .destination_register_name =
          std::string(abi::register_name(move_record.destination_register->reg)),
      .destination_register_bank =
          move_record.destination_register->prepared_bank,
  };
  return prepare::find_prepared_call_result_late_publication(result)
      .source_in_destination_alias_available;
}

void record_call_boundary_source_in_destination(
    const module::MachineInstruction& instruction,
    BlockScalarLoweringState& scalar_state) {
  const auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr || !move_record->destination_register.has_value()) {
    return;
  }
  std::optional<prepare::PreparedValueId> source_value_id;
  c4c::ValueNameId source_value_name = c4c::kInvalidValueName;
  if (move_record->source_memory.has_value() &&
      move_record->source_memory->result_value_name.has_value()) {
    source_value_id = move_record->source_memory->result_value_id;
    source_value_name = *move_record->source_memory->result_value_name;
  } else if (move_record->source_register.has_value() &&
             move_record->source_register->value_name != c4c::kInvalidValueName) {
    if (move_record->source_register->value_id.has_value() &&
        move_record->source_register->prepared_bank !=
            prepare::PreparedRegisterBank::None &&
        move_record->destination_register->prepared_bank !=
            prepare::PreparedRegisterBank::None &&
        !call_boundary_register_source_in_destination_available(*move_record)) {
      return;
    }
    source_value_id = move_record->source_register->value_id;
    source_value_name = move_record->source_register->value_name;
  }
  if (source_value_name == c4c::kInvalidValueName) {
    return;
  }
  auto source_alias = *move_record->destination_register;
  source_alias.value_id = source_value_id;
  source_alias.value_name = source_value_name;
  record_emitted_scalar_register(scalar_state, source_value_name, source_alias);
}

[[nodiscard]] bool call_boundary_move_reloads_prepared_stack_source(
    const module::MachineInstruction& instruction) {
  const auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  return move_record != nullptr &&
         move_record->phase == prepare::PreparedMovePhase::BeforeInstruction &&
         move_record->source_memory.has_value() &&
         move_record->source_memory->support == MemoryOperandSupportKind::Prepared &&
         move_record->source_memory->base_kind == MemoryBaseKind::FrameSlot &&
         move_record->source_memory->byte_offset_is_prepared_snapshot;
}

[[nodiscard]] bool source_value_is_materialized_address(
    const CallBoundaryMoveInstructionRecord& move_record,
    const std::vector<module::MachineInstruction>& materialized_addresses) {
  for (const auto& materialized : materialized_addresses) {
    const auto* address_record =
        std::get_if<AddressMaterializationRecord>(&materialized.target.payload);
    if (address_record == nullptr) {
      continue;
    }
    if (address_record->result_value_id.has_value() &&
        *address_record->result_value_id == move_record.move.from_value_id) {
      return true;
    }
    if (move_record.source_register.has_value() &&
        address_record->result_value_name != c4c::kInvalidValueName &&
        move_record.source_register->value_name == address_record->result_value_name) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool source_register_conflicts_with_materialized_address(
    const module::MachineInstruction& instruction,
    const std::vector<module::MachineInstruction>& materialized_addresses) {
  const auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr ||
      !move_record->source_register.has_value() ||
      source_value_is_materialized_address(*move_record, materialized_addresses)) {
    return false;
  }
  for (const auto& materialized : materialized_addresses) {
    const auto* address_record =
        std::get_if<AddressMaterializationRecord>(&materialized.target.payload);
    if (address_record == nullptr || !address_record->result_register.has_value()) {
      continue;
    }
    if (registers_alias(*move_record->source_register,
                        *address_record->result_register)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::vector<std::uint8_t> indirect_callee_materialization_scratch_indices() {
  std::vector<std::uint8_t> indices;
  for (const auto scratch : abi::reserved_mir_scratch_gp_registers()) {
    indices.push_back(scratch.index);
  }
  for (const auto scratch : abi::indirect_call_scratch_registers()) {
    indices.push_back(scratch.index);
  }
  return indices;
}

[[nodiscard]] bool scratch_index_is_available(
    std::uint8_t candidate,
    std::uint8_t target_index,
    const std::vector<std::uint8_t>& occupied_indices) {
  return candidate != target_index &&
         std::find(occupied_indices.begin(), occupied_indices.end(), candidate) ==
             occupied_indices.end();
}

[[nodiscard]] std::optional<std::uint8_t> choose_scratch_index(
    const std::vector<std::uint8_t>& scratch_indices,
    std::uint8_t target_index,
    const std::vector<std::uint8_t>& occupied_indices) {
  for (const auto candidate : scratch_indices) {
    if (scratch_index_is_available(candidate, target_index, occupied_indices)) {
      return candidate;
    }
  }
  return std::nullopt;
}

[[nodiscard]] const bir::Value* prepared_indirect_callee_source_producer_result(
    const prepare::PreparedEdgePublicationSourceProducer& producer) {
  switch (producer.kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return producer.load_local != nullptr ? &producer.load_local->result : nullptr;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return producer.load_global != nullptr ? &producer.load_global->result : nullptr;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      return producer.cast != nullptr ? &producer.cast->result : nullptr;
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      return producer.binary != nullptr ? &producer.binary->result : nullptr;
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return producer.select != nullptr ? &producer.select->result : nullptr;
    case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      return nullptr;
  }
  return nullptr;
}

[[nodiscard]] const prepare::PreparedEdgePublicationSourceProducer*
find_prepared_indirect_callee_source_producer(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublicationSourceProducerLookups* source_producers,
    c4c::ValueNameId value_name,
    std::size_t before_instruction_index) {
  if (source_producers == nullptr ||
      context.function.prepared == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  const auto* producer =
      prepare::find_indexed_prepared_edge_publication_source_producer(
          source_producers, value_name);
  if (producer == nullptr ||
      producer->block_label != context.control_flow_block->block_label ||
      producer->instruction_index >= before_instruction_index ||
      producer->instruction_index >= context.bir_block->insts.size()) {
    return nullptr;
  }
  const auto* result = prepared_indirect_callee_source_producer_result(*producer);
  if (result == nullptr ||
      result->kind != bir::Value::Kind::Named ||
      result->name.empty()) {
    return nullptr;
  }
  const auto resolved =
      context.function.prepared->names.value_names.find(result->name);
  return resolved == value_name ? producer : nullptr;
}

[[nodiscard]] const prepare::PreparedEdgePublicationSourceProducer*
find_prepared_indirect_callee_source_producer(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublicationSourceProducerLookups* source_producers,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return nullptr;
  }
  const auto value_name =
      context.function.prepared->names.value_names.find(value.name);
  return find_prepared_indirect_callee_source_producer(
      context, source_producers, value_name, before_instruction_index);
}

[[nodiscard]] prepare::PreparedDirectGlobalSelectChainDependency
find_prepared_indirect_callee_direct_global_select_chain(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublicationSourceProducerLookups* source_producers,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr ||
      context.control_flow_block == nullptr) {
    return {};
  }
  return prepare::find_prepared_direct_global_select_chain_dependency(
      context.function.prepared->names,
      source_producers,
      context.control_flow_block->block_label,
      context.bir_block,
      value,
      before_instruction_index);
}

[[nodiscard]] std::optional<prepare::PreparedSameBlockLoadLocalStoredValueSource>
find_prepared_indirect_callee_stored_value_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublicationSourceProducerLookups* source_producers,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  return prepare::find_prepared_same_block_load_local_stored_value_source(
      context.function.prepared->names,
      context.function.prepared->stack_layout,
      addressing,
      source_producers,
      context.control_flow_block->block_label,
      context.bir_block,
      value,
      before_instruction_index);
}

[[nodiscard]] bool emit_indirect_callee_value_to_register_with_csel(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublicationSourceProducerLookups* source_producers,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t target_index,
    const std::vector<std::uint8_t>& scratch_indices,
    std::vector<std::uint8_t> occupied_indices,
    std::vector<std::string>& lines,
    unsigned depth = 0) {
  if (depth > 16U) {
    return false;
  }
  const auto* producer =
      find_prepared_indirect_callee_source_producer(
          context, source_producers, value, before_instruction_index);
  const auto* select =
      producer != nullptr &&
              producer->kind ==
                  prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization
          ? producer->select
          : nullptr;
  if (select == nullptr) {
    const auto scratch_index =
        choose_scratch_index(scratch_indices, target_index, occupied_indices);
    return scratch_index.has_value() &&
           emit_value_publication_to_register(context,
                                              value,
                                              before_instruction_index,
                                              target_index,
                                              *scratch_index,
                                              lines,
                                              true);
  }

  const auto condition = branch_condition_suffix(select->predicate);
  const auto compare_view = scalar_view_for_type(select->compare_type);
  const auto result_view = scalar_view_for_type(value.type);
  if (!condition.has_value() || !compare_view.has_value() ||
      !result_view.has_value()) {
    return false;
  }

  const auto producer_index = producer->instruction_index;
  const auto true_index =
      choose_scratch_index(scratch_indices, target_index, occupied_indices);
  if (!true_index.has_value()) {
    return false;
  }

  auto false_value = select->false_value;
  false_value.type = value.type;
  auto true_value = select->true_value;
  true_value.type = value.type;
  if (!emit_indirect_callee_value_to_register_with_csel(context,
                                                        source_producers,
                                                        false_value,
                                                        producer_index,
                                                        target_index,
                                                        scratch_indices,
                                                        occupied_indices,
                                                        lines,
                                                        depth + 1)) {
    return false;
  }

  auto true_occupied = occupied_indices;
  true_occupied.push_back(target_index);
  if (!emit_indirect_callee_value_to_register_with_csel(context,
                                                        source_producers,
                                                        true_value,
                                                        producer_index,
                                                        *true_index,
                                                        scratch_indices,
                                                        true_occupied,
                                                        lines,
                                                        depth + 1)) {
    return false;
  }

  auto compare_occupied = occupied_indices;
  compare_occupied.push_back(target_index);
  compare_occupied.push_back(*true_index);
  const auto lhs_index =
      choose_scratch_index(scratch_indices, target_index, compare_occupied);
  if (!lhs_index.has_value()) {
    return false;
  }
  compare_occupied.push_back(*lhs_index);
  const auto rhs_index =
      choose_scratch_index(scratch_indices, target_index, compare_occupied);
  if (!rhs_index.has_value()) {
    return false;
  }

  auto lhs = select->lhs;
  lhs.type = select->compare_type;
  if (!emit_value_publication_to_register(context,
                                          lhs,
                                          producer_index,
                                          *lhs_index,
                                          *rhs_index,
                                          lines,
                                          true)) {
    return false;
  }
  std::string rhs_name;
  if (select->rhs.kind == bir::Value::Kind::Immediate &&
      is_cmp_immediate_encodable(select->rhs.immediate)) {
    rhs_name = "#" + std::to_string(select->rhs.immediate);
  } else {
    auto rhs = select->rhs;
    rhs.type = select->compare_type;
    if (!emit_value_publication_to_register(context,
                                            rhs,
                                            producer_index,
                                            *rhs_index,
                                            *lhs_index,
                                            lines,
                                            true)) {
      return false;
    }
    const auto rhs_register = abi::gp_register(*rhs_index, *compare_view);
    if (!rhs_register.has_value()) {
      return false;
    }
    rhs_name = abi::register_name(*rhs_register);
  }

  const auto lhs_register = abi::gp_register(*lhs_index, *compare_view);
  const auto target_register = abi::gp_register(target_index, *result_view);
  const auto true_register = abi::gp_register(*true_index, *result_view);
  if (!lhs_register.has_value() || !target_register.has_value() ||
      !true_register.has_value()) {
    return false;
  }
  lines.push_back("cmp " + std::string{abi::register_name(*lhs_register)} + ", " +
                  rhs_name);
  lines.push_back("csel " + std::string{abi::register_name(*target_register)} + ", " +
                  std::string{abi::register_name(*true_register)} + ", " +
                  std::string{abi::register_name(*target_register)} + ", " +
                  std::string{*condition});
  return true;
}

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_indirect_call_callee_to_prepared_register(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  if (!call.is_indirect || !call.callee_value.has_value() ||
      !call_plan.is_indirect || !call_plan.indirect_callee.has_value()) {
    return std::nullopt;
  }
  const auto& callee = *call_plan.indirect_callee;
  if (callee.value_name == c4c::kInvalidValueName ||
      callee.encoding != prepare::PreparedStorageEncodingKind::Register ||
      callee.bank != prepare::PreparedRegisterBank::Gpr ||
      !callee.register_name.has_value()) {
    return std::nullopt;
  }

  std::optional<prepare::PreparedEdgePublicationSourceProducerLookups>
      fallback_source_producers;
  const auto* source_producers =
      context.function.prepared_lookups != nullptr
          ? &context.function.prepared_lookups->edge_publication_source_producers
          : nullptr;
  if (source_producers == nullptr &&
      context.function.prepared != nullptr &&
      context.function.control_flow != nullptr) {
    fallback_source_producers =
        prepare::make_prepared_edge_publication_source_producer_lookups(
            *context.function.prepared,
            *context.function.control_flow);
    source_producers = &*fallback_source_producers;
  }
  const auto* callee_producer =
      find_prepared_indirect_callee_source_producer(
          context, source_producers, callee.value_name, instruction_index);
  const auto* callee_source = callee_producer != nullptr
                                  ? prepared_indirect_callee_source_producer_result(
                                        *callee_producer)
                                  : nullptr;
  if (callee_source == nullptr) {
    return std::nullopt;
  }

  bir::Value source_value = *callee_source;
  std::size_t source_before_index = instruction_index;
  if (const auto stored = find_prepared_indirect_callee_stored_value_source(
          context, source_producers, source_value, instruction_index);
      stored.has_value()) {
    const auto direct_global_dependency =
        find_prepared_indirect_callee_direct_global_select_chain(
            context,
            source_producers,
            stored->stored_value,
            stored->store_instruction_index);
    if (direct_global_dependency.contains_direct_global_load) {
      source_value = stored->stored_value;
      source_before_index = stored->store_instruction_index;
    }
  }

  const auto target_register = abi::parse_aarch64_register_name(*callee.register_name);
  if (!target_register.has_value() ||
      target_register->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  const auto result_register = abi::gp_register(target_register->index, abi::RegisterView::X);
  if (!result_register.has_value()) {
    return std::nullopt;
  }
  const auto scratches = indirect_callee_materialization_scratch_indices();
  std::vector<std::string> lines;
  if (!emit_indirect_callee_value_to_register_with_csel(context,
                                                        source_producers,
                                                        source_value,
                                                        source_before_index,
                                                        result_register->index,
                                                        scratches,
                                                        {},
                                                        lines) ||
      lines.empty()) {
    return std::nullopt;
  }

  RegisterOperand emitted{
      .reg = *result_register,
      .role = RegisterOperandRole::CallAbi,
      .value_id = callee.value_id,
      .value_name = callee.value_name,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = abi::RegisterView::X,
      .contiguous_width = 1,
      .occupied_register_references = {*result_register},
      .occupied_registers = {abi::register_name(*result_register)},
  };
  record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));

}

void record_call_result_source_register(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const prepare::PreparedCallPlan& call_plan,
    BlockScalarLoweringState& scalar_state,
    bool result_lanes_only) {
  const auto result_late_publication =
      call_plan.result.has_value()
          ? prepare::find_prepared_call_result_late_publication(
                *call_plan.result)
          : prepare::PreparedCallResultLatePublicationFact{};
  if (result_lanes_only ||
      !result_late_publication.source_register_publication_available) {
    if (context.bir_block == nullptr ||
        instruction_index >= context.bir_block->insts.size()) {
      return;
    }
  } else {
    const auto& result = *call_plan.result;
    const auto* home =
        prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                  context.function.value_locations,
                                                  *result.destination_value_id);
    if (home != nullptr && home->value_name != c4c::kInvalidValueName) {
      const auto parsed = abi::parse_aarch64_register_name(*result.source_register_name);
      if (parsed.has_value()) {
        prepare::PreparedRegisterClass prepared_class =
            prepare::PreparedRegisterClass::None;
        abi::RegisterBank expected_bank = parsed->bank;
        switch (result.source_register_bank.value_or(
            prepare::PreparedRegisterBank::None)) {
          case prepare::PreparedRegisterBank::Gpr:
            prepared_class = prepare::PreparedRegisterClass::General;
            expected_bank = abi::RegisterBank::GeneralPurpose;
            break;
          case prepare::PreparedRegisterBank::Fpr:
            prepared_class = prepare::PreparedRegisterClass::Float;
            expected_bank = abi::RegisterBank::FpSimd;
            break;
          case prepare::PreparedRegisterBank::Vreg:
            prepared_class = prepare::PreparedRegisterClass::Vector;
            expected_bank = abi::RegisterBank::FpSimd;
            break;
          case prepare::PreparedRegisterBank::None:
            break;
        }
        if (prepared_class != prepare::PreparedRegisterClass::None &&
            parsed->bank == expected_bank) {
          abi::PreparedRegisterConversionResult converted;
          if (result.source_register_placement.has_value()) {
            converted =
                abi::convert_prepared_register(*result.source_register_placement,
                                               prepared_class,
                                               parsed->view);
          } else {
            converted =
                abi::convert_prepared_register(*result.source_register_name,
                                               result.source_register_bank,
                                               prepared_class,
                                               parsed->view);
          }
          if (converted.reg.has_value() && converted.reg->bank == expected_bank) {
            record_emitted_scalar_register(
                scalar_state,
                home->value_name,
                RegisterOperand{
                    .reg = *converted.reg,
                    .role = RegisterOperandRole::CallAbi,
                    .value_id = home->value_id,
                    .value_name = home->value_name,
                    .prepared_class = prepared_class,
                    .prepared_bank = *result.source_register_bank,
                    .expected_view = converted.reg->view,
                    .contiguous_width = result.source_contiguous_width,
                    .occupied_register_references = {*converted.reg},
                    .occupied_registers =
                        result.source_occupied_register_names.empty()
                            ? std::vector<std::string_view>{
                                  abi::register_name(*converted.reg)}
                            : std::vector<std::string_view>(
                                  result.source_occupied_register_names.begin(),
                                  result.source_occupied_register_names.end()),
                });
          }
        }
      }
    }
  }
  if (context.bir_block == nullptr ||
      instruction_index >= context.bir_block->insts.size()) {
    return;
  }
  const auto* call = std::get_if<bir::CallInst>(
      &context.bir_block->insts[instruction_index]);
  if (call == nullptr) {
    return;
  }

  auto record_lane_binding = [&](const bir::Value& value) {
    const auto value_name = prepared_named_value_id(context, value);
    if (!value_name.has_value()) {
      return;
    }
    const auto* home =
        prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                  context.function.regalloc,
                                                  context.function.value_locations,
                                                  *value_name);
    if (home == nullptr || home->value_id == prepare::PreparedValueId{0}) {
      return;
    }
    const auto* binding =
        module::find_prepared_after_call_result_lane_binding(context.function,
                                                             context.block_index,
                                                             instruction_index,
                                                             home->value_id);
    if (binding == nullptr || binding->abi_binding == nullptr ||
        binding->abi_binding->destination_kind !=
            prepare::PreparedMoveDestinationKind::CallResultAbi ||
        binding->abi_binding->destination_storage_kind !=
            prepare::PreparedMoveStorageKind::Register) {
      return;
    }

    const auto& abi_binding = *binding->abi_binding;
    std::optional<prepare::PreparedRegisterBank> prepared_bank;
    std::optional<prepare::PreparedRegisterClass> prepared_class;
    std::optional<abi::RegisterView> expected_view;
    if (abi_binding.destination_register_placement.has_value()) {
      prepared_bank = abi_binding.destination_register_placement->bank;
      switch (value.type) {
        case bir::TypeKind::F32:
          expected_view = abi::RegisterView::S;
          break;
        case bir::TypeKind::F64:
          expected_view = abi::RegisterView::D;
          break;
        case bir::TypeKind::F128:
          expected_view = abi::RegisterView::Q;
          break;
        default:
          expected_view = scalar_register_view(value.type);
          break;
      }
    } else if (abi_binding.destination_register_name.has_value()) {
      const auto parsed =
          abi::parse_aarch64_register_name(*abi_binding.destination_register_name);
      if (!parsed.has_value()) {
        return;
      }
      prepared_bank = parsed->bank == abi::RegisterBank::FpSimd
                          ? prepare::PreparedRegisterBank::Fpr
                          : prepare::PreparedRegisterBank::Gpr;
      expected_view = parsed->view;
    } else {
      return;
    }
    switch (*prepared_bank) {
      case prepare::PreparedRegisterBank::Gpr:
        prepared_class = prepare::PreparedRegisterClass::General;
        break;
      case prepare::PreparedRegisterBank::Fpr:
        prepared_class = prepare::PreparedRegisterClass::Float;
        break;
      case prepare::PreparedRegisterBank::Vreg:
        prepared_class = prepare::PreparedRegisterClass::Vector;
        break;
      case prepare::PreparedRegisterBank::None:
      case prepare::PreparedRegisterBank::AggregateAddress:
        return;
    }

    abi::PreparedRegisterConversionResult converted;
    if (abi_binding.destination_register_placement.has_value()) {
      converted =
          abi::convert_prepared_register(*abi_binding.destination_register_placement,
                                         prepared_class,
                                         expected_view);
    } else {
      converted =
          abi::convert_prepared_register(*abi_binding.destination_register_name,
                                         prepared_bank,
                                         prepared_class,
                                         expected_view);
    }
    if (!converted.reg.has_value()) {
      return;
    }
    record_emitted_scalar_register(
        scalar_state,
        *value_name,
        RegisterOperand{
            .reg = *converted.reg,
            .role = RegisterOperandRole::CallAbi,
            .value_id = binding->value_id,
            .value_name = *value_name,
            .prepared_class = *prepared_class,
            .prepared_bank = *prepared_bank,
            .expected_view = converted.reg->view,
            .contiguous_width = abi_binding.destination_contiguous_width,
            .occupied_register_references = {*converted.reg},
            .occupied_registers =
                abi_binding.destination_occupied_register_names.empty()
                    ? std::vector<std::string_view>{
                          abi::register_name(*converted.reg)}
                    : std::vector<std::string_view>(
                          abi_binding.destination_occupied_register_names.begin(),
                          abi_binding.destination_occupied_register_names.end()),
        });
  };

  if (!result_lanes_only && call->result.has_value()) {
    record_lane_binding(*call->result);
  }
  for (const auto& lane : call->result_lanes) {
    record_lane_binding(lane);
  }
}

[[nodiscard]] bool call_result_store_value_retarget_available(
    const RegisterOperand& emitted) {
  if (!emitted.value_id.has_value() ||
      emitted.prepared_bank == prepare::PreparedRegisterBank::None) {
    return false;
  }
  const prepare::PreparedCallResultPlan result{
      .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_value_id = *emitted.value_id,
      .source_register_name = std::string(abi::register_name(emitted.reg)),
      .source_register_bank = emitted.prepared_bank,
  };
  return prepare::find_prepared_call_result_late_publication(result)
      .fpr_or_vreg_store_value_retarget_available;
}

void retarget_fpr_call_result_store_value_to_emitted_scalar(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    module::MachineInstruction& instruction) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      store->value.kind != bir::Value::Kind::Named ||
      (store->value.type != bir::TypeKind::F32 &&
       store->value.type != bir::TypeKind::F64 &&
       store->value.type != bir::TypeKind::F128)) {
    return;
  }
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      memory_record->value_type != store->value.type) {
    return;
  }
  const auto value_name = prepared_named_value_id(context, store->value);
  if (!value_name.has_value()) {
    return;
  }
  const auto emitted = find_emitted_scalar_register(scalar_state, *value_name);
  if (!emitted.has_value() ||
      emitted->role != RegisterOperandRole::CallAbi ||
      (emitted->prepared_bank != prepare::PreparedRegisterBank::Fpr &&
       emitted->prepared_bank != prepare::PreparedRegisterBank::Vreg) ||
      emitted->reg.bank != abi::RegisterBank::FpSimd) {
    return;
  }
  if (emitted->value_id.has_value() &&
      emitted->prepared_bank != prepare::PreparedRegisterBank::None &&
      !call_result_store_value_retarget_available(*emitted)) {
    return;
  }
  memory_record->value = make_register_operand(*emitted);
}

[[nodiscard]] std::vector<module::MachineInstruction>
materialize_missing_frame_slot_call_arguments(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr) {
    return lowered;
  }
  for (const auto& argument : call_plan.arguments) {
    const auto publication_need =
        prepare::find_prepared_missing_frame_slot_call_argument_publication_need(
            argument);
    if (!publication_need.available ||
        publication_need.source_selection == nullptr) {
      continue;
    }
    const auto* home =
        prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                  context.function.value_locations,
                                                  publication_need.source_value_id);
    if (home == nullptr ||
        home->kind != prepare::PreparedValueHomeKind::StackSlot ||
        find_emitted_scalar_register(scalar_state, home->value_name).has_value()) {
      continue;
    }
    const auto prepared_move =
        find_prepared_frame_slot_call_argument_move(
            context, call_plan, argument, instruction_index);
    if (!prepared_move.has_value()) {
      continue;
    }
    std::optional<MemoryOperand> address_source;
    std::optional<MemoryOperand> source;
    const auto& source_selection = *publication_need.source_selection;
    {
      if (source_selection.kind ==
          prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress) {
        address_source = make_selected_call_argument_source(
            context,
            argument,
            home,
            source_selection,
            prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress,
            instruction_index);
        source = address_source;
      } else if (source_selection.kind ==
                 prepare::PreparedCallArgumentSourceSelectionKind::
                     LocalFrameAddressMaterialization) {
        address_source = make_selected_call_argument_source(
            context,
            argument,
            home,
            source_selection,
            prepare::PreparedCallArgumentSourceSelectionKind::
                LocalFrameAddressMaterialization,
            instruction_index);
        source = address_source;
      } else if (source_selection.kind ==
                     prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue ||
                 source_selection.kind ==
                     prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation) {
        address_source = make_selected_call_argument_source(
            context,
            argument,
            home,
            source_selection,
            prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress,
            instruction_index);
        source =
            address_source.has_value()
                ? address_source
                : make_selected_call_argument_source(
                      context,
                      argument,
                      home,
                      source_selection,
                      prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue,
                      instruction_index);
      }
    }
    if (!source.has_value()) {
      continue;
    }
    const auto expected_view =
        address_source.has_value()
            ? std::optional<abi::RegisterView>{abi::RegisterView::X}
            : scalar_integer_register_view_from_size(source->size_bytes);
    if (!expected_view.has_value()) {
      continue;
    }
    const auto& move = *prepared_move->move;
    const auto* binding = prepared_move->binding;
    const auto destination_register_placement =
        binding != nullptr && binding->destination_register_placement.has_value()
            ? binding->destination_register_placement
            : (move.destination_register_placement.has_value()
                   ? move.destination_register_placement
                   : argument.destination_register_placement);
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : (binding != nullptr && binding->destination_register_name.has_value()
                   ? binding->destination_register_name
                   : move.destination_register_name);
    const auto prepared_class = prepare::PreparedRegisterClass::General;
    abi::PreparedRegisterConversionResult converted;
    if (destination_register_placement.has_value()) {
      converted =
          abi::convert_prepared_register(*destination_register_placement,
                                         prepared_class,
                                         expected_view);
    } else if (destination_register_name.has_value()) {
      converted =
          abi::convert_prepared_register(*destination_register_name,
                                         argument.destination_register_bank,
                                         prepared_class,
                                         expected_view);
    } else {
      continue;
    }
    if (!converted.reg.has_value()) {
      continue;
    }
    RegisterOperand destination{
        .reg = *converted.reg,
        .role = RegisterOperandRole::CallAbi,
        .value_id = move.to_value_id != 0
                        ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                        : argument.source_value_id,
        .value_name = home->value_name,
        .prepared_class = prepared_class,
        .prepared_bank = argument.destination_register_bank.value_or(
            prepare::PreparedRegisterBank::None),
        .expected_view = *expected_view,
        .contiguous_width = binding != nullptr ? binding->destination_contiguous_width
                                               : move.destination_contiguous_width,
        .occupied_register_references = {*converted.reg},
        .occupied_registers =
            binding != nullptr && !binding->destination_occupied_register_names.empty()
                ? std::vector<std::string_view>(
                      binding->destination_occupied_register_names.begin(),
                      binding->destination_occupied_register_names.end())
                : (!move.destination_occupied_register_names.empty()
                       ? std::vector<std::string_view>(
                             move.destination_occupied_register_names.begin(),
                             move.destination_occupied_register_names.end())
                       : std::vector<std::string_view>{abi::register_name(*converted.reg)}),
    };
    CallBoundaryMoveInstructionRecord move_record{
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .phase = prepared_move->bundle->phase,
        .authority_kind = prepared_move->bundle->authority_kind,
        .block_index = prepared_move->bundle->block_index,
        .instruction_index = prepared_move->bundle->instruction_index,
        .source_parallel_copy_predecessor_label =
            prepared_move->bundle->source_parallel_copy_predecessor_label,
        .source_parallel_copy_successor_label =
            prepared_move->bundle->source_parallel_copy_successor_label,
        .move = move,
        .source_memory = *source,
        .source_memory_materializes_address = address_source.has_value(),
        .destination_register = destination,
        .source_bundle = prepared_move->bundle,
        .source_move = prepared_move->move,
    };
    if (address_source.has_value()) {
      if (auto payload = materialize_local_aggregate_address_payload(
              context, argument, *address_source, instruction_index)) {
        lowered.push_back(std::move(*payload));
      }
    }
    record_emitted_scalar_register(scalar_state, destination.value_name, destination);
    lowered.push_back(make_dispatch_bridge_machine_instruction(
        context,
        instruction_index,
        make_call_boundary_move_instruction(std::move(move_record))));
  }
  return lowered;
}

[[nodiscard]] std::vector<module::MachineInstruction>
publish_stack_preserved_call_values(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    const BlockScalarLoweringState& scalar_state) {
  std::vector<module::MachineInstruction> lowered;
  const auto* call_plans =
      context.function.call_plans != nullptr
          ? context.function.call_plans
          : (context.function.prepared != nullptr &&
                     context.function.control_flow != nullptr
                 ? prepare::find_prepared_call_plans(
                       *context.function.prepared,
                       context.function.control_flow->function_name)
                 : nullptr);
  if (call_plans == nullptr || context.function.call_plan_lookups == nullptr) {
    return lowered;
  }
  const auto* first_stack_values =
      prepare::first_indexed_stack_preserved_values_for_call(
          *context.function.call_plan_lookups, *call_plans, call_plan);
  if (first_stack_values == nullptr) {
    return lowered;
  }
  for (const auto* preserved_ptr : *first_stack_values) {
    if (preserved_ptr == nullptr) {
      continue;
    }
    const auto& preserved = *preserved_ptr;
    if (preserved.route != prepare::PreparedCallPreservationRoute::StackSlot ||
        preserved.value_name == c4c::kInvalidValueName ||
        !preserved.slot_id.has_value() ||
        !preserved.stack_offset_bytes.has_value() ||
        !preserved.stack_size_bytes.has_value() ||
        *preserved.stack_size_bytes == 0) {
      continue;
    }
    const auto emitted =
        find_emitted_scalar_register(scalar_state, preserved.value_name);
    std::optional<abi::RegisterView> expected_view;
    if (*preserved.stack_size_bytes == 8) {
      expected_view = abi::RegisterView::X;
    } else if (*preserved.stack_size_bytes == 4 ||
               *preserved.stack_size_bytes == 2 ||
               *preserved.stack_size_bytes == 1) {
      expected_view = abi::RegisterView::W;
    } else {
      continue;
    }
    std::optional<abi::RegisterReference> source_register;
    if (emitted.has_value() && abi::is_gp_register(emitted->reg)) {
      source_register = emitted->reg;
    } else if (context.function.value_locations != nullptr) {
      const auto* home =
          prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                    context.function.value_locations,
                                                    preserved.value_id);
      if (home != nullptr &&
          home->kind == prepare::PreparedValueHomeKind::Register &&
          home->register_name.has_value()) {
        const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
        if (parsed.has_value() &&
            parsed->bank == abi::RegisterBank::GeneralPurpose) {
          source_register = *parsed;
        }
      }
    }
    if (!source_register.has_value()) {
      continue;
    }
    const auto source_reg = abi::gp_register(source_register->index, *expected_view);
    if (!source_reg.has_value()) {
      continue;
    }

    std::vector<std::string> lines;
    lines.push_back("str " + std::string(abi::register_name(*source_reg)) + ", " +
                    frame_slot_address(context.function, *preserved.stack_offset_bytes));
    if (auto published =
            make_select_chain_materialization_instruction(
                context, instruction_index, std::move(lines))) {
      lowered.push_back(std::move(*published));
    }
  }
  return lowered;
}

}  // namespace c4c::backend::aarch64::codegen
