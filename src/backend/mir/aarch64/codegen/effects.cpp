#include "effects.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace abi = c4c::backend::aarch64::abi;

thread_local bool g_publish_prepared_call_preserve_effects = true;

std::string_view machine_effect_resource_kind_name(MachineEffectResourceKind kind) {
  switch (kind) {
    case MachineEffectResourceKind::PreparedValue:
      return "prepared_value";
    case MachineEffectResourceKind::Register:
      return "register";
    case MachineEffectResourceKind::Memory:
      return "memory";
    case MachineEffectResourceKind::FrameSlot:
      return "frame_slot";
    case MachineEffectResourceKind::Symbol:
      return "symbol";
    case MachineEffectResourceKind::BranchTarget:
      return "branch_target";
    case MachineEffectResourceKind::Flags:
      return "flags";
  }
  return "unknown";
}

std::string_view machine_side_effect_kind_name(MachineSideEffectKind kind) {
  switch (kind) {
    case MachineSideEffectKind::ControlFlowTransfer:
      return "control_flow_transfer";
    case MachineSideEffectKind::MemoryRead:
      return "memory_read";
    case MachineSideEffectKind::MemoryWrite:
      return "memory_write";
    case MachineSideEffectKind::VolatileMemoryAccess:
      return "volatile_memory_access";
    case MachineSideEffectKind::AtomicMemoryAccess:
      return "atomic_memory_access";
    case MachineSideEffectKind::Call:
      return "call";
    case MachineSideEffectKind::Return:
      return "return";
    case MachineSideEffectKind::FrameSetup:
      return "frame_setup";
    case MachineSideEffectKind::FrameTeardown:
      return "frame_teardown";
    case MachineSideEffectKind::InlineAssembly:
      return "inline_assembly";
    case MachineSideEffectKind::ObjectEmission:
      return "object_emission";
  }
  return "unknown";
}

std::string_view register_display_name(abi::RegisterReference reg) {
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

std::vector<std::string_view> occupied_register_views(abi::RegisterReference reg) {
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

}  // namespace

MachineEffectResource machine_effect_from_operand(const OperandRecord& operand) {
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

MachineEffectResource machine_prepared_value_def(
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name) {
  return MachineEffectResource{
      .kind = MachineEffectResourceKind::PreparedValue,
      .value_id = value_id,
      .value_name = value_name,
  };
}

std::vector<MachineEffectResource> machine_effects_from_operands(
    const std::vector<OperandRecord>& operands) {
  std::vector<MachineEffectResource> effects;
  effects.reserve(operands.size());
  for (const auto& operand : operands) {
    effects.push_back(machine_effect_from_operand(operand));
  }
  return effects;
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

bool prepared_call_preserve_effect_publication_enabled() {
  return g_publish_prepared_call_preserve_effects;
}

ScopedPreparedCallPreserveEffectPublication::ScopedPreparedCallPreserveEffectPublication(
    bool enabled)
    : previous_enabled_(g_publish_prepared_call_preserve_effects) {
  g_publish_prepared_call_preserve_effects = enabled;
}

ScopedPreparedCallPreserveEffectPublication::~ScopedPreparedCallPreserveEffectPublication() {
  g_publish_prepared_call_preserve_effects = previous_enabled_;
}

}  // namespace c4c::backend::aarch64::codegen
