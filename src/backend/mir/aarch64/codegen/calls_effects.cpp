#include "calls.hpp"
#include "memory.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace abi = c4c::backend::aarch64::abi;

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

MachineEffectResource call_effect_from_operand(const OperandRecord& operand) {
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

MachineEffectResource call_prepared_value_def(
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name) {
  return MachineEffectResource{
      .kind = MachineEffectResourceKind::PreparedValue,
      .value_id = value_id,
      .value_name = value_name,
  };
}

std::vector<MachineEffectResource> call_effects_from_operands(
    const std::vector<OperandRecord>& operands) {
  std::vector<MachineEffectResource> effects;
  effects.reserve(operands.size());
  for (const auto& operand : operands) {
    effects.push_back(call_effect_from_operand(operand));
  }
  return effects;
}

}  // namespace c4c::backend::aarch64::codegen
