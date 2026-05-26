#include "calls.hpp"
#include "dispatch_diagnostics.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;

// Shared frame-slot lookup plus call-argument source helpers. Frame-slot value
// and address source choice is prepared by call plans; this file translates
// complete prepared facts into AArch64 memory operands.

[[nodiscard]] const prepare::PreparedFrameSlot* find_frame_slot_by_id(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedFrameSlotId slot_id) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return &slot;
    }
  }
  return nullptr;
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

[[nodiscard]] std::optional<MemoryOperand> make_frame_slot_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t instruction_index) {
  if (!argument.source_selection.has_value()) {
    return std::nullopt;
  }
  if (auto selected = make_selected_frame_slot_source(
          context,
          argument,
          &source_home,
          *argument.source_selection,
          prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue,
          false,
          instruction_index)) {
    return selected;
  }
  return std::nullopt;
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

[[nodiscard]] std::optional<MemoryOperand> make_frame_slot_call_argument_address_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t instruction_index) {
  if (!argument.source_selection.has_value()) {
    return std::nullopt;
  }
  return make_selected_frame_slot_source(
      context,
      argument,
      &source_home,
      *argument.source_selection,
      prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress,
      true,
      instruction_index);
}

[[nodiscard]] bool local_frame_address_name_matches(std::string_view source_name,
                                                    std::string_view candidate_name) {
  return candidate_name == source_name ||
         (candidate_name.size() == source_name.size() + 2 &&
          candidate_name.compare(0, source_name.size(), source_name) == 0 &&
          candidate_name.substr(source_name.size()) == ".0");
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

[[nodiscard]] std::optional<MemoryOperand> make_local_frame_address_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t instruction_index) {
  if (argument.source_selection.has_value()) {
    if (auto selected = make_selected_local_frame_address_source(
            context, argument, source_home, *argument.source_selection,
            instruction_index)) {
      return selected;
    }
    return std::nullopt;
  }
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      source_home.value_name == c4c::kInvalidValueName ||
      argument.source_encoding != prepare::PreparedStorageEncodingKind::Register ||
      !argument.allows_local_aggregate_address_publication) {
    return std::nullopt;
  }

  const auto source_name =
      prepare::prepared_value_name(context.function.prepared->names,
                                   source_home.value_name);
  if (source_name.empty()) {
    return std::nullopt;
  }

  const prepare::PreparedAddressMaterialization* selected = nullptr;
  if (const auto* addressing = prepare::find_prepared_addressing(
          *context.function.prepared, context.function.control_flow->function_name);
      addressing != nullptr) {
    for (const auto& materialization : addressing->address_materializations) {
      if (materialization.block_label != context.control_flow_block->block_label ||
          materialization.inst_index > instruction_index ||
          materialization.kind != prepare::PreparedAddressMaterializationKind::FrameSlot ||
          !materialization.frame_slot_id.has_value() ||
          !materialization.result_value_name.has_value()) {
        continue;
      }
      const auto materialized_name =
          prepare::prepared_value_name(context.function.prepared->names,
                                       *materialization.result_value_name);
      if (!local_frame_address_name_matches(source_name, materialized_name)) {
        continue;
      }
      if (selected != nullptr && selected->inst_index == materialization.inst_index) {
        return std::nullopt;
      }
      if (selected == nullptr || selected->inst_index < materialization.inst_index) {
        selected = &materialization;
      }
    }
  }
  if (selected != nullptr) {
    return MemoryOperand{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = selected->function_name,
        .block_label = selected->block_label,
        .instruction_index = selected->inst_index,
        .result_value_id = argument.source_value_id,
        .result_value_name = source_home.value_name,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = selected->frame_slot_id,
        .byte_offset = selected->byte_offset,
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = source_home.size_bytes.value_or(8),
        .align_bytes = source_home.align_bytes.value_or(8),
        .address_space = selected->address_space,
        .can_use_base_plus_offset = true,
    };
  }

  return std::nullopt;
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

}  // namespace c4c::backend::aarch64::codegen
