#include "calls.hpp"
#include "dispatch_diagnostics.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;

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
  if (argument.source_selection.has_value()) {
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
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      argument.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      source_home.value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }

  const auto* addressing = prepare::find_prepared_addressing(
      *context.function.prepared, context.function.control_flow->function_name);
  if (addressing == nullptr) {
    return std::nullopt;
  }

  const prepare::PreparedMemoryAccess* source_access = nullptr;
  for (const auto& access : addressing->accesses) {
    if (access.result_value_name == std::optional<c4c::ValueNameId>{source_home.value_name}) {
      if (source_access != nullptr) {
        return std::nullopt;
      }
      source_access = &access;
    }
  }
  if (source_access == nullptr) {
    const auto source_offset = source_home.offset_bytes.has_value()
                                   ? source_home.offset_bytes
                                   : argument.source_stack_offset_bytes;
    if (!source_offset.has_value()) {
      return std::nullopt;
    }
    return MemoryOperand{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = context.function.control_flow->function_name,
        .block_label = context.control_flow_block->block_label,
        .instruction_index = instruction_index,
        .result_value_id = argument.source_value_id,
        .result_value_name = source_home.value_name,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = source_home.slot_id.has_value() ? source_home.slot_id
                                                         : argument.source_slot_id,
        .byte_offset = static_cast<std::int64_t>(*source_offset),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = source_home.size_bytes.value_or(4),
        .align_bytes = source_home.align_bytes.value_or(4),
        .can_use_base_plus_offset = true,
    };
  }
  if (source_access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !source_access->address.frame_slot_id.has_value()) {
    const auto source_offset = source_home.offset_bytes.has_value()
                                   ? source_home.offset_bytes
                                   : argument.source_stack_offset_bytes;
    if (source_offset.has_value()) {
      return MemoryOperand{
          .surface = RecordSurfaceKind::MachineInstructionNode,
          .support = MemoryOperandSupportKind::Prepared,
          .function_name = context.function.control_flow->function_name,
          .block_label = context.control_flow_block->block_label,
          .instruction_index = instruction_index,
          .result_value_id = argument.source_value_id,
          .result_value_name = source_home.value_name,
          .base_kind = MemoryBaseKind::FrameSlot,
          .frame_slot_id = source_home.slot_id.has_value() ? source_home.slot_id
                                                           : argument.source_slot_id,
          .byte_offset = static_cast<std::int64_t>(*source_offset),
          .byte_offset_is_prepared_snapshot = true,
          .size_bytes = source_home.size_bytes.value_or(4),
          .align_bytes = source_home.align_bytes.value_or(4),
          .can_use_base_plus_offset = true,
      };
    }
    return std::nullopt;
  }

  const auto* slot = find_frame_slot_by_id(context.function.prepared->stack_layout,
                                           *source_access->address.frame_slot_id);
  if (slot == nullptr) {
    return std::nullopt;
  }

  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = source_access->function_name,
      .block_label = source_access->block_label,
      .instruction_index = source_access->inst_index,
      .result_value_id = argument.source_value_id,
      .result_value_name = source_home.value_name,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = source_access->address.frame_slot_id,
      .byte_offset = static_cast<std::int64_t>(slot->offset_bytes) +
                     source_access->address.byte_offset,
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = source_access->address.size_bytes,
      .align_bytes = source_access->address.align_bytes,
      .address_space = source_access->address_space,
      .is_volatile = source_access->is_volatile,
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
    if (argument.source_selection->kind ==
        prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress) {
      return std::nullopt;
    }
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
  if (argument.source_selection.has_value()) {
    if (auto selected = make_selected_frame_slot_source(
            context,
            argument,
            &source_home,
            *argument.source_selection,
            prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress,
            true,
            instruction_index)) {
      return selected;
    }
    return std::nullopt;
  }
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      argument.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      source_home.value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto* addressing = prepare::find_prepared_addressing(
      *context.function.prepared, context.function.control_flow->function_name);
  const prepare::PreparedAddressMaterialization* selected = nullptr;
  if (addressing != nullptr) {
    for (const auto& materialization : addressing->address_materializations) {
      if (materialization.block_label == context.control_flow_block->block_label &&
          materialization.inst_index <= instruction_index &&
          materialization.kind == prepare::PreparedAddressMaterializationKind::FrameSlot &&
          materialization.result_value_name == source_home.value_name &&
          materialization.frame_slot_id.has_value()) {
        if (selected != nullptr && selected->inst_index == materialization.inst_index) {
          return std::nullopt;
        }
        if (selected != nullptr && selected->inst_index > materialization.inst_index) {
          continue;
        }
        selected = &materialization;
      }
    }
  }
  if (selected == nullptr) {
    if (!argument.allows_local_aggregate_address_publication) {
      return std::nullopt;
    }
    const auto source_name =
        prepare::prepared_value_name(context.function.prepared->names,
                                     source_home.value_name);
    if (source_name.empty()) {
      return std::nullopt;
    }
    const std::string first_lane_name = std::string{source_name} + ".0";
    const prepare::PreparedStackObject* selected_object = nullptr;
    const prepare::PreparedFrameSlot* selected_slot = nullptr;
    for (const auto& object : context.function.prepared->stack_layout.objects) {
      if (object.function_name != context.function.control_flow->function_name ||
          object.source_kind != "local_slot") {
        continue;
      }
      const auto object_name =
          prepare::prepared_stack_object_name(context.function.prepared->names, object);
      if (object_name != first_lane_name) {
        continue;
      }
      for (const auto& slot : context.function.prepared->stack_layout.frame_slots) {
        if (slot.object_id != object.object_id) {
          continue;
        }
        if (selected_slot == nullptr || slot.offset_bytes < selected_slot->offset_bytes) {
          selected_object = &object;
          selected_slot = &slot;
        }
      }
    }
    if (selected_object == nullptr || selected_slot == nullptr) {
      return std::nullopt;
    }
    return MemoryOperand{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = context.function.control_flow->function_name,
        .block_label = context.control_flow_block->block_label,
        .instruction_index = instruction_index,
        .result_value_id = argument.source_value_id,
        .result_value_name = source_home.value_name,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = selected_slot->slot_id,
        .byte_offset = static_cast<std::int64_t>(selected_slot->offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = selected_object->size_bytes == 0 ? std::size_t{8}
                                                       : selected_object->size_bytes,
        .align_bytes = selected_object->align_bytes == 0 ? std::size_t{8}
                                                         : selected_object->align_bytes,
        .can_use_base_plus_offset = true,
    };
  }
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

  const prepare::PreparedStackObject* selected_object = nullptr;
  const prepare::PreparedFrameSlot* selected_slot = nullptr;
  for (const auto& object : context.function.prepared->stack_layout.objects) {
    if (object.function_name != context.function.control_flow->function_name ||
        object.source_kind != "local_slot") {
      continue;
    }
    const auto object_name =
        prepare::prepared_stack_object_name(context.function.prepared->names, object);
    if (!local_frame_address_name_matches(source_name, object_name)) {
      continue;
    }
    const prepare::PreparedFrameSlot* object_slot = nullptr;
    for (const auto& slot : context.function.prepared->stack_layout.frame_slots) {
      if (slot.object_id == object.object_id) {
        object_slot = &slot;
        break;
      }
    }
    if (object_slot == nullptr) {
      continue;
    }
    if (selected_slot == nullptr ||
        object_name == source_name ||
        object_slot->offset_bytes < selected_slot->offset_bytes) {
      selected_object = &object;
      selected_slot = object_slot;
      if (object_name == source_name) {
        break;
      }
    }
  }
  if (selected_object == nullptr || selected_slot == nullptr) {
    return std::nullopt;
  }
  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow->function_name,
      .block_label = context.control_flow_block->block_label,
      .instruction_index = instruction_index,
      .result_value_id = argument.source_value_id,
      .result_value_name = source_home.value_name,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = selected_slot->slot_id,
      .byte_offset = static_cast<std::int64_t>(selected_slot->offset_bytes),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = selected_object->size_bytes == 0 ? std::size_t{8}
                                                     : selected_object->size_bytes,
      .align_bytes = selected_object->align_bytes == 0 ? std::size_t{8}
                                                       : selected_object->align_bytes,
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
    const prepare::PreparedCallPreservedValue& preserved,
    const prepare::PreparedValueHome* source_home,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto source_view =
      source_home != nullptr && source_home->size_bytes.has_value()
          ? scalar_integer_register_view_from_size(*source_home->size_bytes)
          : scalar_view_from_register_name(preserved.register_name);
  PreservedCallArgumentSource result{.preserved = &preserved};
  if (preserved.route == prepare::PreparedCallPreservationRoute::CalleeSavedRegister) {
    auto source = make_register_operand_from_prepared_authority(
        preserved.register_name,
        preserved.register_placement,
        preserved.register_bank,
        RegisterOperandRole::CallAbi,
        preserved.value_id,
        preserved.value_name,
        preserved.contiguous_width,
        preserved.occupied_register_names,
        source_view,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value()) {
      return std::nullopt;
    }
    source->value_name = c4c::kInvalidValueName;
    result.source_register = *source;
    return result;
  }

  if (preserved.route == prepare::PreparedCallPreservationRoute::StackSlot &&
      preserved.slot_id.has_value() && preserved.stack_offset_bytes.has_value() &&
      preserved.stack_size_bytes.has_value() && *preserved.stack_size_bytes != 0 &&
      preserved.stack_align_bytes.has_value() && *preserved.stack_align_bytes != 0) {
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
        .result_value_id = preserved.value_id,
        .result_value_name = std::nullopt,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = preserved.slot_id,
        .byte_offset = static_cast<std::int64_t>(*preserved.stack_offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = *preserved.stack_size_bytes,
        .align_bytes = *preserved.stack_align_bytes,
        .can_use_base_plus_offset = true,
    };
    return result;
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<PreservedCallArgumentSource>
make_prior_preserved_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentSourceSelection& selection,
    const prepare::PreparedValueHome* source_home,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  (void)diagnostics;
  if (selection.kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation ||
      selection.preservation_route !=
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
  (void)source_home;
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
