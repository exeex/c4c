#include "calls.hpp"

#include <cstdint>
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

namespace {

prepare::PreparedRegisterClass register_class_from_bank(
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

std::string_view register_display_name(
    abi::RegisterReference reg) {
  static constexpr std::string_view x_names[] = {
      "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
      "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
      "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
      "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"};
  static constexpr std::string_view w_names[] = {
      "w0",  "w1",  "w2",  "w3",  "w4",  "w5",  "w6",  "w7",
      "w8",  "w9",  "w10", "w11", "w12", "w13", "w14", "w15",
      "w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23",
      "w24", "w25", "w26", "w27", "w28", "w29", "w30", "w31"};
  static constexpr std::string_view v_names[] = {
      "v0",  "v1",  "v2",  "v3",  "v4",  "v5",  "v6",  "v7",
      "v8",  "v9",  "v10", "v11", "v12", "v13", "v14", "v15",
      "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
      "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31"};
  static constexpr std::string_view s_names[] = {
      "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
      "s8",  "s9",  "s10", "s11", "s12", "s13", "s14", "s15",
      "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
      "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31"};
  static constexpr std::string_view d_names[] = {
      "d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
      "d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
      "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
      "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"};
  static constexpr std::string_view q_names[] = {
      "q0",  "q1",  "q2",  "q3",  "q4",  "q5",  "q6",  "q7",
      "q8",  "q9",  "q10", "q11", "q12", "q13", "q14", "q15",
      "q16", "q17", "q18", "q19", "q20", "q21", "q22", "q23",
      "q24", "q25", "q26", "q27", "q28", "q29", "q30", "q31"};
  if (reg.index >= 32U) {
    return {};
  }
  switch (reg.view) {
    case abi::RegisterView::X:
      return x_names[reg.index];
    case abi::RegisterView::W:
      return w_names[reg.index];
    case abi::RegisterView::V:
      return v_names[reg.index];
    case abi::RegisterView::S:
      return s_names[reg.index];
    case abi::RegisterView::D:
      return d_names[reg.index];
    case abi::RegisterView::Q:
      return q_names[reg.index];
    case abi::RegisterView::Sp:
      return "sp";
  }
  return {};
}

std::vector<std::string_view> occupied_register_views(
    abi::RegisterReference reg) {
  const auto display_name = register_display_name(reg);
  if (display_name.empty()) {
    return {};
  }
  return {display_name};
}

std::vector<std::string_view> occupied_register_views(
    const std::vector<abi::RegisterReference>& regs) {
  std::vector<std::string_view> views;
  views.reserve(regs.size());
  for (const auto reg : regs) {
    const auto display_name = register_display_name(reg);
    if (display_name.empty()) {
      return {};
    }
    views.push_back(display_name);
  }
  return views;
}

std::optional<abi::RegisterView> prepared_clobber_expected_view(
    prepare::PreparedRegisterBank bank) {
  switch (bank) {
    case prepare::PreparedRegisterBank::Gpr:
    case prepare::PreparedRegisterBank::AggregateAddress:
      return abi::RegisterView::X;
    case prepare::PreparedRegisterBank::Fpr:
    case prepare::PreparedRegisterBank::Vreg:
    case prepare::PreparedRegisterBank::None:
      return std::nullopt;
  }
  return std::nullopt;
}

}  // namespace

std::optional<MachineEffectResource> effect_from_prepared_call_clobber(
    const prepare::PreparedClobberedRegister& clobber) {
  if (clobber.register_name.empty() || clobber.contiguous_width == 0 ||
      clobber.bank == prepare::PreparedRegisterBank::None) {
    return std::nullopt;
  }

  const auto prepared_class = register_class_from_bank(clobber.bank);
  const auto expected_view = prepared_clobber_expected_view(clobber.bank);
  const auto converted_primary = abi::convert_prepared_register(
      clobber.register_name, clobber.bank, prepared_class, expected_view);
  if (!converted_primary.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> occupied_names = clobber.occupied_register_names;
  if (occupied_names.empty() && clobber.contiguous_width == 1) {
    occupied_names.push_back(clobber.register_name);
  }
  if (occupied_names.size() != clobber.contiguous_width) {
    return std::nullopt;
  }

  std::vector<abi::RegisterReference> occupied_refs;
  occupied_refs.reserve(occupied_names.size());
  for (const auto& occupied_name : occupied_names) {
    const auto converted_occupied = abi::convert_prepared_register(
        occupied_name, clobber.bank, prepared_class, expected_view);
    if (!converted_occupied.has_value()) {
      return std::nullopt;
    }
    occupied_refs.push_back(*converted_occupied.reg);
  }
  if (occupied_refs.empty() || occupied_refs.front() != *converted_primary.reg) {
    return std::nullopt;
  }

  const auto occupied_views = occupied_register_views(occupied_refs);
  if (occupied_views.size() != occupied_refs.size()) {
    return std::nullopt;
  }

  const OperandRecord operand = make_register_operand(RegisterOperand{
      .reg = *converted_primary.reg,
      .role = RegisterOperandRole::CallAbi,
      .prepared_class = prepared_class,
      .prepared_bank = clobber.bank,
      .expected_view = expected_view,
      .contiguous_width = clobber.contiguous_width,
      .occupied_register_references = occupied_refs,
      .occupied_registers = occupied_views,
  });
  return MachineEffectResource{
      .kind = MachineEffectResourceKind::Register,
      .operand = operand,
      .reg = *converted_primary.reg,
  };
}

std::vector<MachineEffectResource> effects_from_prepared_call_clobbers(
    const std::vector<prepare::PreparedClobberedRegister>& clobbers) {
  std::vector<MachineEffectResource> effects;
  effects.reserve(clobbers.size());
  for (const auto& clobber : clobbers) {
    if (auto effect = effect_from_prepared_call_clobber(clobber)) {
      effects.push_back(std::move(*effect));
    }
  }
  return effects;
}

std::optional<MachineEffectResource> effect_from_prepared_call_preserved_value(
    const prepare::PreparedCallPreservedValue& preserved) {
  if (preserved.route == prepare::PreparedCallPreservationRoute::StackSlot) {
    if (!preserved.slot_id.has_value() ||
        !preserved.stack_offset_bytes.has_value() ||
        !preserved.stack_size_bytes.has_value() ||
        *preserved.stack_size_bytes == 0 ||
        !preserved.stack_align_bytes.has_value() ||
        *preserved.stack_align_bytes == 0) {
      return std::nullopt;
    }

    const OperandRecord operand = make_memory_operand(MemoryOperand{
        .support = MemoryOperandSupportKind::Prepared,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = preserved.slot_id,
        .byte_offset = static_cast<std::int64_t>(*preserved.stack_offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = *preserved.stack_size_bytes,
        .align_bytes = *preserved.stack_align_bytes,
        .can_use_base_plus_offset = true,
    });
    return MachineEffectResource{
        .kind = MachineEffectResourceKind::Memory,
        .operand = operand,
        .value_id = preserved.value_id,
        .value_name = preserved.value_name,
        .frame_slot_id = preserved.slot_id,
    };
  }
  if (preserved.route != prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      !preserved.register_name.has_value() || !preserved.register_bank.has_value() ||
      preserved.register_name->empty() ||
      *preserved.register_bank == prepare::PreparedRegisterBank::None ||
      preserved.contiguous_width == 0) {
    return std::nullopt;
  }

  const auto prepared_class = register_class_from_bank(*preserved.register_bank);
  const auto expected_view = prepared_clobber_expected_view(*preserved.register_bank);
  const auto converted_primary = abi::convert_prepared_register(
      *preserved.register_name, *preserved.register_bank, prepared_class, expected_view);
  if (!converted_primary.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> occupied_names = preserved.occupied_register_names;
  if (occupied_names.empty() && preserved.contiguous_width == 1) {
    occupied_names.push_back(*preserved.register_name);
  }
  if (occupied_names.size() != preserved.contiguous_width) {
    return std::nullopt;
  }

  std::vector<abi::RegisterReference> occupied_refs;
  occupied_refs.reserve(occupied_names.size());
  for (const auto& occupied_name : occupied_names) {
    const auto converted_occupied = abi::convert_prepared_register(
        occupied_name, *preserved.register_bank, prepared_class, expected_view);
    if (!converted_occupied.has_value()) {
      return std::nullopt;
    }
    occupied_refs.push_back(*converted_occupied.reg);
  }
  if (occupied_refs.empty() || occupied_refs.front() != *converted_primary.reg) {
    return std::nullopt;
  }

  const auto occupied_views = occupied_register_views(occupied_refs);
  if (occupied_views.size() != occupied_refs.size()) {
    return std::nullopt;
  }

  const OperandRecord operand = make_register_operand(RegisterOperand{
      .reg = *converted_primary.reg,
      .role = RegisterOperandRole::CallAbi,
      .value_id = preserved.value_id,
      .value_name = preserved.value_name,
      .prepared_class = prepared_class,
      .prepared_bank = *preserved.register_bank,
      .expected_view = expected_view,
      .contiguous_width = preserved.contiguous_width,
      .occupied_register_references = occupied_refs,
      .occupied_registers = occupied_views,
  });
  return MachineEffectResource{
      .kind = MachineEffectResourceKind::Register,
      .operand = operand,
      .value_id = preserved.value_id,
      .value_name = preserved.value_name,
      .reg = *converted_primary.reg,
  };
}

std::vector<MachineEffectResource> effects_from_prepared_call_preserved_values(
    const std::vector<prepare::PreparedCallPreservedValue>& preserved_values) {
  std::vector<MachineEffectResource> effects;
  effects.reserve(preserved_values.size());
  for (const auto& preserved : preserved_values) {
    if (auto effect = effect_from_prepared_call_preserved_value(preserved)) {
      effects.push_back(std::move(*effect));
    }
  }
  return effects;
}

namespace {

MachineEffectResource effect_from_operand(const OperandRecord& operand) {
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

MachineEffectResource prepared_value_def(
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name) {
  return MachineEffectResource{
      .kind = MachineEffectResourceKind::PreparedValue,
      .value_id = value_id,
      .value_name = value_name,
  };
}

std::vector<MachineEffectResource> effects_from_operands(
    const std::vector<OperandRecord>& operands) {
  std::vector<MachineEffectResource> effects;
  effects.reserve(operands.size());
  for (const auto& operand : operands) {
    effects.push_back(effect_from_operand(operand));
  }
  return effects;
}

MachineNodeStatusRecord call_boundary_move_selection_status(
    const CallBoundaryMoveInstructionRecord& instruction) {
  if (instruction.source_bundle == nullptr || instruction.source_move == nullptr ||
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
  if (!selected_register_argument_move && !selected_register_result_move) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node is outside the selected register call-boundary move subset"};
  }
  if (!instruction.source_register.has_value() ||
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
  if (instruction.source_register->prepared_bank == prepare::PreparedRegisterBank::Gpr &&
      instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Gpr) {
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
            "call-boundary move node requires prepared GPR registers or structured f128 q-register authority"};
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

std::optional<VariadicVaStartRecord> make_variadic_va_start_record(
    const prepare::PreparedVariadicEntryPlanFunction& entry,
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  if (!homes.destination_va_list.has_value() ||
      !entry.named_register_counts.gp.has_value() ||
      !entry.named_register_counts.fp.has_value() ||
      !entry.va_list_layout.size_bytes.has_value() ||
      !entry.va_list_layout.align_bytes.has_value() ||
      entry.va_list_layout.fields.empty() ||
      !entry.register_save_area.slot_id.has_value() ||
      !entry.register_save_area.stack_offset_bytes.has_value() ||
      !entry.register_save_area.size_bytes.has_value() ||
      !entry.register_save_area.align_bytes.has_value() ||
      !entry.register_save_area.gp_offset_bytes.has_value() ||
      !entry.register_save_area.fp_offset_bytes.has_value() ||
      !entry.register_save_area.gp_slot_size_bytes.has_value() ||
      !entry.register_save_area.fp_slot_size_bytes.has_value() ||
      !entry.register_save_area.saved_gp_register_count.has_value() ||
      !entry.register_save_area.saved_fp_register_count.has_value() ||
      !entry.register_save_area.initial_gp_offset_bytes.has_value() ||
      !entry.register_save_area.initial_fp_offset_bytes.has_value() ||
      !entry.overflow_area.base_slot_id.has_value() ||
      !entry.overflow_area.base_stack_offset_bytes.has_value() ||
      !entry.overflow_area.align_bytes.has_value() ||
      !entry.helper_resources.scratch_register_count.has_value() ||
      !entry.helper_resources.scratch_stack_bytes.has_value()) {
    return std::nullopt;
  }

  return VariadicVaStartRecord{
      .destination_va_list = *homes.destination_va_list,
      .named_gp_register_count = *entry.named_register_counts.gp,
      .named_fp_register_count = *entry.named_register_counts.fp,
      .va_list_size_bytes = *entry.va_list_layout.size_bytes,
      .va_list_align_bytes = *entry.va_list_layout.align_bytes,
      .va_list_fields = entry.va_list_layout.fields,
      .register_save_area_slot_id = *entry.register_save_area.slot_id,
      .register_save_area_stack_offset_bytes =
          *entry.register_save_area.stack_offset_bytes,
      .register_save_area_size_bytes = *entry.register_save_area.size_bytes,
      .register_save_area_align_bytes = *entry.register_save_area.align_bytes,
      .register_save_area_gp_offset_bytes = *entry.register_save_area.gp_offset_bytes,
      .register_save_area_fp_offset_bytes = *entry.register_save_area.fp_offset_bytes,
      .register_save_area_gp_slot_size_bytes =
          *entry.register_save_area.gp_slot_size_bytes,
      .register_save_area_fp_slot_size_bytes =
          *entry.register_save_area.fp_slot_size_bytes,
      .saved_gp_register_count = *entry.register_save_area.saved_gp_register_count,
      .saved_fp_register_count = *entry.register_save_area.saved_fp_register_count,
      .initial_gp_offset_bytes = *entry.register_save_area.initial_gp_offset_bytes,
      .initial_fp_offset_bytes = *entry.register_save_area.initial_fp_offset_bytes,
      .overflow_area_base_slot_id = *entry.overflow_area.base_slot_id,
      .overflow_area_base_stack_offset_bytes =
          *entry.overflow_area.base_stack_offset_bytes,
      .overflow_area_align_bytes = *entry.overflow_area.align_bytes,
      .scratch_register_count = *entry.helper_resources.scratch_register_count,
      .scratch_stack_bytes = *entry.helper_resources.scratch_stack_bytes,
  };
}

std::optional<VariadicScalarVaArgRecord> make_variadic_scalar_va_arg_record(
    const prepare::PreparedVariadicEntryPlanFunction& entry,
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  if (!homes.source_va_list.has_value() || !homes.scalar_result.has_value() ||
      !homes.scalar_access_plan.has_value()) {
    return std::nullopt;
  }
  const auto& plan = *homes.scalar_access_plan;
  if (plan.source_class == prepare::PreparedVariadicScalarVaArgSourceClass::Unknown ||
      plan.value_type == bir::TypeKind::Void ||
      plan.value_size_bytes == 0 ||
      plan.value_align_bytes == 0 ||
      !plan.result_home.has_value() ||
      !plan.source_field.has_value() ||
      !plan.source_field_offset_bytes.has_value() ||
      !plan.source_slot_size_bytes.has_value() ||
      !plan.progression_field.has_value() ||
      !plan.progression_field_offset_bytes.has_value() ||
      !plan.progression_stride_bytes.has_value() ||
      !plan.overflow_source_field.has_value() ||
      !plan.overflow_source_field_offset_bytes.has_value() ||
      !plan.overflow_stride_bytes.has_value() ||
      !entry.register_save_area.slot_id.has_value() ||
      !entry.register_save_area.stack_offset_bytes.has_value() ||
      !entry.register_save_area.size_bytes.has_value() ||
      !entry.register_save_area.align_bytes.has_value() ||
      !entry.register_save_area.gp_offset_bytes.has_value() ||
      !entry.register_save_area.fp_offset_bytes.has_value() ||
      !entry.register_save_area.gp_slot_size_bytes.has_value() ||
      !entry.register_save_area.fp_slot_size_bytes.has_value() ||
      !entry.overflow_area.base_slot_id.has_value() ||
      !entry.overflow_area.base_stack_offset_bytes.has_value() ||
      !entry.overflow_area.align_bytes.has_value() ||
      !entry.helper_resources.scratch_register_count.has_value() ||
      !entry.helper_resources.scratch_stack_bytes.has_value()) {
    return std::nullopt;
  }

  return VariadicScalarVaArgRecord{
      .source_class = plan.source_class,
      .value_type = plan.value_type,
      .value_size_bytes = plan.value_size_bytes,
      .value_align_bytes = plan.value_align_bytes,
      .source_va_list = *homes.source_va_list,
      .result_home = *plan.result_home,
      .source_field = *plan.source_field,
      .source_field_offset_bytes = *plan.source_field_offset_bytes,
      .source_slot_size_bytes = *plan.source_slot_size_bytes,
      .progression_field = *plan.progression_field,
      .progression_field_offset_bytes = *plan.progression_field_offset_bytes,
      .progression_stride_bytes = *plan.progression_stride_bytes,
      .overflow_source_field = *plan.overflow_source_field,
      .overflow_source_field_offset_bytes = *plan.overflow_source_field_offset_bytes,
      .overflow_stride_bytes = *plan.overflow_stride_bytes,
      .register_save_area_slot_id = *entry.register_save_area.slot_id,
      .register_save_area_stack_offset_bytes =
          *entry.register_save_area.stack_offset_bytes,
      .register_save_area_size_bytes = *entry.register_save_area.size_bytes,
      .register_save_area_align_bytes = *entry.register_save_area.align_bytes,
      .register_save_area_gp_offset_bytes = *entry.register_save_area.gp_offset_bytes,
      .register_save_area_fp_offset_bytes = *entry.register_save_area.fp_offset_bytes,
      .register_save_area_gp_slot_size_bytes =
          *entry.register_save_area.gp_slot_size_bytes,
      .register_save_area_fp_slot_size_bytes =
          *entry.register_save_area.fp_slot_size_bytes,
      .overflow_area_base_slot_id = *entry.overflow_area.base_slot_id,
      .overflow_area_base_stack_offset_bytes =
          *entry.overflow_area.base_stack_offset_bytes,
      .overflow_area_align_bytes = *entry.overflow_area.align_bytes,
      .scratch_register_count = *entry.helper_resources.scratch_register_count,
      .scratch_stack_bytes = *entry.helper_resources.scratch_stack_bytes,
  };
}

std::optional<VariadicAggregateVaArgRecord> make_variadic_aggregate_va_arg_record(
    const prepare::PreparedVariadicEntryPlanFunction& entry,
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  if (!prepare::has_complete_prepared_variadic_aggregate_va_arg_access_plan(homes)) {
    return std::nullopt;
  }
  const auto& plan = *homes.aggregate_access_plan;
  if (!entry.register_save_area.slot_id.has_value() ||
      !entry.register_save_area.stack_offset_bytes.has_value() ||
      !entry.register_save_area.size_bytes.has_value() ||
      !entry.register_save_area.align_bytes.has_value() ||
      !entry.register_save_area.gp_offset_bytes.has_value() ||
      !entry.register_save_area.fp_offset_bytes.has_value() ||
      !entry.register_save_area.gp_slot_size_bytes.has_value() ||
      !entry.register_save_area.fp_slot_size_bytes.has_value() ||
      !entry.overflow_area.base_slot_id.has_value() ||
      !entry.overflow_area.base_stack_offset_bytes.has_value() ||
      !entry.overflow_area.align_bytes.has_value() ||
      !entry.helper_resources.scratch_register_count.has_value() ||
      !entry.helper_resources.scratch_stack_bytes.has_value()) {
    return std::nullopt;
  }

  return VariadicAggregateVaArgRecord{
      .source_class = plan.source_class,
      .payload_size_bytes = plan.payload_size_bytes,
      .payload_align_bytes = plan.payload_align_bytes,
      .source_va_list = *homes.source_va_list,
      .destination_payload_home = *plan.destination_payload_home,
      .source_field = *plan.source_field,
      .source_field_offset_bytes = *plan.source_field_offset_bytes,
      .source_payload_offset_bytes = *plan.source_payload_offset_bytes,
      .source_slot_size_bytes = *plan.source_slot_size_bytes,
      .copy_size_bytes = *plan.copy_size_bytes,
      .copy_align_bytes = *plan.copy_align_bytes,
      .progression_field = *plan.progression_field,
      .progression_field_offset_bytes = *plan.progression_field_offset_bytes,
      .progression_stride_bytes = *plan.progression_stride_bytes,
      .register_save_area_slot_id = *entry.register_save_area.slot_id,
      .register_save_area_stack_offset_bytes =
          *entry.register_save_area.stack_offset_bytes,
      .register_save_area_size_bytes = *entry.register_save_area.size_bytes,
      .register_save_area_align_bytes = *entry.register_save_area.align_bytes,
      .register_save_area_gp_offset_bytes = *entry.register_save_area.gp_offset_bytes,
      .register_save_area_fp_offset_bytes = *entry.register_save_area.fp_offset_bytes,
      .register_save_area_gp_slot_size_bytes =
          *entry.register_save_area.gp_slot_size_bytes,
      .register_save_area_fp_slot_size_bytes =
          *entry.register_save_area.fp_slot_size_bytes,
      .overflow_area_base_slot_id = *entry.overflow_area.base_slot_id,
      .overflow_area_base_stack_offset_bytes =
          *entry.overflow_area.base_stack_offset_bytes,
      .overflow_area_align_bytes = *entry.overflow_area.align_bytes,
      .scratch_register_count = *entry.helper_resources.scratch_register_count,
      .scratch_stack_bytes = *entry.helper_resources.scratch_stack_bytes,
  };
}

std::optional<VariadicVaCopyRecord> make_variadic_va_copy_record(
    const prepare::PreparedVariadicEntryPlanFunction& entry,
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  if (!homes.destination_va_list.has_value() ||
      !homes.source_va_list.has_value() ||
      !entry.va_list_layout.size_bytes.has_value() ||
      !entry.va_list_layout.align_bytes.has_value() ||
      entry.va_list_layout.fields.empty() ||
      !entry.helper_resources.scratch_register_count.has_value() ||
      !entry.helper_resources.scratch_stack_bytes.has_value()) {
    return std::nullopt;
  }

  std::vector<VariadicVaCopyFieldRecord> field_copies;
  field_copies.reserve(entry.va_list_layout.fields.size());
  for (const auto& field : entry.va_list_layout.fields) {
    if (field.size_bytes == 0) {
      return std::nullopt;
    }
    field_copies.push_back(VariadicVaCopyFieldRecord{
        .kind = field.kind,
        .source_offset_bytes = field.offset_bytes,
        .destination_offset_bytes = field.offset_bytes,
        .size_bytes = field.size_bytes,
    });
  }

  return VariadicVaCopyRecord{
      .destination_va_list = *homes.destination_va_list,
      .source_va_list = *homes.source_va_list,
      .va_list_size_bytes = *entry.va_list_layout.size_bytes,
      .va_list_align_bytes = *entry.va_list_layout.align_bytes,
      .field_copies = std::move(field_copies),
      .scratch_register_count = *entry.helper_resources.scratch_register_count,
      .scratch_stack_bytes = *entry.helper_resources.scratch_stack_bytes,
  };
}

MachineNodeStatusRecord call_selection_status(const CallInstructionRecord& instruction) {
  if (instruction.variadic_entry_helper.has_value()) {
    if (instruction.source_variadic_entry == nullptr) {
      return MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::MissingRequiredFacts,
          .diagnostic =
              "variadic entry helper node is missing prepared entry provenance"};
    }
    if (instruction.source_variadic_helper_operand_homes == nullptr) {
      return MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::MissingRequiredFacts,
          .diagnostic =
              "variadic entry helper node is missing prepared operand-home provenance"};
    }
    if (*instruction.variadic_entry_helper ==
        prepare::PreparedVariadicEntryHelperKind::VaStart) {
      if (!instruction.variadic_va_start.has_value()) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic =
                "va_start helper node is missing structured prepared va_start record"};
      }
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    if (*instruction.variadic_entry_helper ==
        prepare::PreparedVariadicEntryHelperKind::VaArg) {
      if (!instruction.variadic_scalar_va_arg.has_value()) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic =
                "scalar va_arg machine-node lowering requires complete prepared fact "
                "helper_operand_homes.va_arg.scalar_access_plan"};
      }
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    if (*instruction.variadic_entry_helper ==
        prepare::PreparedVariadicEntryHelperKind::VaArgAggregate) {
      if (!instruction.variadic_aggregate_va_arg.has_value()) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic =
                "aggregate va_arg machine-node lowering requires complete prepared fact "
                "helper_operand_homes.va_arg_aggregate.aggregate_access_plan"};
      }
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    if (*instruction.variadic_entry_helper ==
        prepare::PreparedVariadicEntryHelperKind::VaCopy) {
      if (!instruction.variadic_va_copy.has_value()) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic =
                "va_copy machine-node lowering requires complete prepared source and "
                "destination va_list homes plus va_list_layout field facts"};
      }
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "variadic entry helper machine-node lowering is outside the selected va_start subset"};
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
    defs.push_back(effect_from_operand(destination));
  } else if (instruction.move.to_value_id != 0) {
    defs.push_back(prepared_value_def(instruction.move.to_value_id, c4c::kInvalidValueName));
  }
  if (instruction.source_register.has_value()) {
    const auto source = make_register_operand(*instruction.source_register);
    operands.push_back(source);
    uses.push_back(effect_from_operand(source));
  } else if (instruction.move.from_value_id != 0) {
    uses.push_back(prepared_value_def(instruction.move.from_value_id, c4c::kInvalidValueName));
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
  if (instruction.variadic_entry_helper ==
          std::optional<prepare::PreparedVariadicEntryHelperKind>{
              prepare::PreparedVariadicEntryHelperKind::VaStart} &&
      !instruction.variadic_va_start.has_value() &&
      instruction.source_variadic_entry != nullptr &&
      instruction.source_variadic_helper_operand_homes != nullptr) {
    instruction.variadic_va_start =
        make_variadic_va_start_record(*instruction.source_variadic_entry,
                                      *instruction.source_variadic_helper_operand_homes);
  }
  if (instruction.variadic_entry_helper ==
          std::optional<prepare::PreparedVariadicEntryHelperKind>{
              prepare::PreparedVariadicEntryHelperKind::VaArg} &&
      !instruction.variadic_scalar_va_arg.has_value() &&
      instruction.source_variadic_entry != nullptr &&
      instruction.source_variadic_helper_operand_homes != nullptr) {
    instruction.variadic_scalar_va_arg =
        make_variadic_scalar_va_arg_record(
            *instruction.source_variadic_entry,
            *instruction.source_variadic_helper_operand_homes);
  }
  if (instruction.variadic_entry_helper ==
          std::optional<prepare::PreparedVariadicEntryHelperKind>{
              prepare::PreparedVariadicEntryHelperKind::VaArgAggregate} &&
      !instruction.variadic_aggregate_va_arg.has_value() &&
      instruction.source_variadic_entry != nullptr &&
      instruction.source_variadic_helper_operand_homes != nullptr) {
    instruction.variadic_aggregate_va_arg =
        make_variadic_aggregate_va_arg_record(
            *instruction.source_variadic_entry,
            *instruction.source_variadic_helper_operand_homes);
  }
  if (instruction.variadic_entry_helper ==
          std::optional<prepare::PreparedVariadicEntryHelperKind>{
              prepare::PreparedVariadicEntryHelperKind::VaCopy} &&
      !instruction.variadic_va_copy.has_value() &&
      instruction.source_variadic_entry != nullptr &&
      instruction.source_variadic_helper_operand_homes != nullptr) {
    instruction.variadic_va_copy =
        make_variadic_va_copy_record(*instruction.source_variadic_entry,
                                     *instruction.source_variadic_helper_operand_homes);
  }

  std::vector<OperandRecord> operands = instruction.arguments;
  if (instruction.indirect_callee.has_value()) {
    operands.insert(operands.begin(), *instruction.indirect_callee);
  } else if (instruction.direct_callee.has_value()) {
    operands.insert(operands.begin(), make_symbol_operand(*instruction.direct_callee));
  }
  std::vector<MachineEffectResource> defs;
  if (instruction.result.has_value()) {
    defs.push_back(effect_from_operand(*instruction.result));
  }
  std::vector<MachineEffectResource> uses = effects_from_operands(operands);
  std::vector<MachineSideEffectKind> side_effects = {MachineSideEffectKind::Call};
  if (instruction.memory_return_storage.has_value()) {
    defs.push_back(effect_from_operand(make_memory_operand(*instruction.memory_return_storage)));
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_va_start.has_value()) {
    defs.push_back(prepared_value_def(
        instruction.variadic_va_start->destination_va_list.value_id,
        instruction.variadic_va_start->destination_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_scalar_va_arg.has_value()) {
    defs.push_back(prepared_value_def(
        instruction.variadic_scalar_va_arg->result_home.value_id,
        instruction.variadic_scalar_va_arg->result_home.value_name));
    uses.push_back(prepared_value_def(
        instruction.variadic_scalar_va_arg->source_va_list.value_id,
        instruction.variadic_scalar_va_arg->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_aggregate_va_arg.has_value()) {
    defs.push_back(prepared_value_def(
        instruction.variadic_aggregate_va_arg->destination_payload_home.value_id,
        instruction.variadic_aggregate_va_arg->destination_payload_home.value_name));
    uses.push_back(prepared_value_def(
        instruction.variadic_aggregate_va_arg->source_va_list.value_id,
        instruction.variadic_aggregate_va_arg->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_va_copy.has_value()) {
    defs.push_back(prepared_value_def(
        instruction.variadic_va_copy->destination_va_list.value_id,
        instruction.variadic_va_copy->destination_va_list.value_name));
    uses.push_back(prepared_value_def(
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
      .operands = operands,
      .defs = defs,
      .uses = uses,
      .clobbers = effects_from_prepared_call_clobbers(instruction.clobbered_registers),
      .preserves = effects_from_prepared_call_preserved_values(instruction.preserved_values),
      .side_effects = std::move(side_effects),
      .payload = instruction,
  };
}


}  // namespace c4c::backend::aarch64::codegen
