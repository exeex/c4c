#include "pointer_carriers.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace c4c::backend::prepare::regalloc_detail {

[[nodiscard]] const bir::Value* pointer_result_value(const bir::Inst& inst) {
  return std::visit(
      [&](const auto& typed_inst) -> const bir::Value* {
        using InstT = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<InstT, bir::BinaryInst> ||
                      std::is_same_v<InstT, bir::SelectInst> ||
                      std::is_same_v<InstT, bir::CastInst> ||
                      std::is_same_v<InstT, bir::PhiInst> ||
                      std::is_same_v<InstT, bir::LoadLocalInst> ||
                      std::is_same_v<InstT, bir::LoadGlobalInst>) {
          return typed_inst.result.type == bir::TypeKind::Ptr ? &typed_inst.result
                                                              : nullptr;
        } else if constexpr (std::is_same_v<InstT, bir::CallInst>) {
          return typed_inst.result.has_value() &&
                         typed_inst.result->type == bir::TypeKind::Ptr
                     ? &*typed_inst.result
                     : nullptr;
        } else {
          return static_cast<const bir::Value*>(nullptr);
        }
      },
      inst);
}

[[nodiscard]] std::unordered_set<std::string_view> collect_cfg_cycle_block_labels(
    const bir::Function& function) {
  std::unordered_map<std::string_view, std::size_t> block_index_by_label;
  block_index_by_label.reserve(function.blocks.size());
  for (std::size_t index = 0; index < function.blocks.size(); ++index) {
    block_index_by_label.emplace(function.blocks[index].label, index);
  }
  std::unordered_set<std::string_view> cycle_labels;
  const auto add_backedge_range =
      [&](std::size_t source_index, const std::string& target_label) {
        const auto target_it = block_index_by_label.find(target_label);
        if (target_it == block_index_by_label.end() ||
            target_it->second > source_index) {
          return;
        }
        for (std::size_t index = target_it->second; index <= source_index; ++index) {
          cycle_labels.insert(function.blocks[index].label);
        }
      };
  for (std::size_t index = 0; index < function.blocks.size(); ++index) {
    const auto& terminator = function.blocks[index].terminator;
    add_backedge_range(index, terminator.target_label);
    add_backedge_range(index, terminator.false_label);
  }
  return cycle_labels;
}

[[nodiscard]] std::unordered_set<std::string> collect_risky_cycle_pointer_slots(
    const bir::Function& function,
    const std::unordered_set<std::string_view>& cycle_labels) {
  std::unordered_map<std::string, std::size_t> pointer_stores_by_slot;
  for (const auto& block : function.blocks) {
    if (cycle_labels.find(block.label) == cycle_labels.end()) {
      continue;
    }
    for (const auto& inst : block.insts) {
      const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
      if (store == nullptr ||
          store->value.kind != bir::Value::Kind::Named ||
          store->value.type != bir::TypeKind::Ptr ||
          store->address.has_value()) {
        continue;
      }
      ++pointer_stores_by_slot[store->slot_name];
    }
  }
  std::unordered_set<std::string> risky_slots;
  for (const auto& [slot_name, count] : pointer_stores_by_slot) {
    if (count > 1) {
      risky_slots.insert(slot_name);
    }
  }
  return risky_slots;
}

[[nodiscard]] std::optional<LinkNameId> prepared_pointer_symbol_name(
    PreparedNameTables& names,
    const bir::Module& module,
    const bir::Value& value) {
  if (value.pointer_symbol_link_name_id == kInvalidLinkName) {
    return std::nullopt;
  }
  const std::string_view symbol =
      module.names.link_names.spelling(value.pointer_symbol_link_name_id);
  if (symbol.empty()) {
    return std::nullopt;
  }
  return names.link_names.intern(symbol);
}

void update_prepared_pointer_value_access_step(
    std::unordered_map<ValueNameId, std::size_t>& steps,
    ValueNameId value_name,
    std::size_t step_bytes,
    bool* changed) {
  if (value_name == kInvalidValueName || step_bytes == 0) {
    return;
  }
  const auto [it, inserted] = steps.emplace(value_name, step_bytes);
  if (inserted) {
    if (changed != nullptr) {
      *changed = true;
    }
    return;
  }
  if (step_bytes < it->second) {
    it->second = step_bytes;
    if (changed != nullptr) {
      *changed = true;
    }
  }
}

[[nodiscard]] PreparedPointerCarrierState prepared_pointer_value_access_carrier(
    ValueNameId value_name,
    std::size_t step_bytes) {
  return PreparedPointerCarrierState{
      .base_value_name = value_name,
      .byte_delta = 0,
      .step_bytes = step_bytes,
      .authority = PreparedPointerCarrierAuthority::PreparedPointerValueAccess,
  };
}

[[nodiscard]] PreparedPointerCarrierState prepared_frame_address_materialization_carrier(
    ValueNameId value_name) {
  return PreparedPointerCarrierState{
      .base_value_name = value_name,
      .byte_delta = 0,
      .step_bytes = 1,
      .authority = PreparedPointerCarrierAuthority::PreparedFrameAddressMaterialization,
  };
}

[[nodiscard]] PreparedPointerCarrierState bir_pointer_symbol_carrier(
    ValueNameId value_name,
    LinkNameId symbol_name) {
  return PreparedPointerCarrierState{
      .base_value_name = value_name,
      .base_symbol_name = symbol_name,
      .byte_delta = 0,
      .step_bytes = 1,
      .authority = PreparedPointerCarrierAuthority::BirPointerSymbol,
  };
}

[[nodiscard]] std::optional<std::int64_t> immediate_pointer_byte_delta(
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  switch (value.type) {
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
      return value.immediate;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<PreparedPointerCarrierState> resolve_prepared_pointer_carrier_state(
    ValueNameId value_name,
    const PreparedPointerCarrierMap& pointer_carriers,
    const std::unordered_map<ValueNameId, std::size_t>& direct_step_by_value_name,
    std::string_view slot_name,
    const std::unordered_map<std::string, PreparedPointerCarrierState>& slot_pointer_carriers);

[[nodiscard]] std::optional<PreparedPointerCarrierState> bir_pointer_immediate_offset_carrier(
    PreparedNameTables& names,
    const bir::BinaryInst& binary,
    const PreparedPointerCarrierMap& pointer_carriers,
    const std::unordered_map<ValueNameId, std::size_t>& direct_step_by_value_name) {
  if (binary.result.kind != bir::Value::Kind::Named ||
      binary.result.type != bir::TypeKind::Ptr ||
      binary.operand_type != bir::TypeKind::Ptr) {
    return std::nullopt;
  }

  const auto make_candidate =
      [&](const bir::Value& base_value,
          std::int64_t byte_delta) -> std::optional<PreparedPointerCarrierState> {
    if (base_value.kind != bir::Value::Kind::Named ||
        base_value.type != bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    const auto base_value_name = names.value_names.find(base_value.name);
    if (base_value_name == kInvalidValueName) {
      return std::nullopt;
    }
    const auto base_carrier = resolve_prepared_pointer_carrier_state(
        base_value_name, pointer_carriers, direct_step_by_value_name, {}, {});
    if (!base_carrier.has_value() || !has_semantic_pointer_carrier_authority(*base_carrier)) {
      return std::nullopt;
    }
    return PreparedPointerCarrierState{
        .base_value_name = base_carrier->base_value_name,
        .base_symbol_name = base_carrier->base_symbol_name,
        .byte_delta = base_carrier->byte_delta + byte_delta,
        .step_bytes = base_carrier->step_bytes,
        .authority = PreparedPointerCarrierAuthority::BirPointerImmediateOffset,
    };
  };

  if (binary.lhs.type == bir::TypeKind::Ptr) {
    if (const auto delta = immediate_pointer_byte_delta(binary.rhs);
        delta.has_value()) {
      if (binary.opcode == bir::BinaryOpcode::Add) {
        return make_candidate(binary.lhs, *delta);
      }
      if (binary.opcode == bir::BinaryOpcode::Sub) {
        return make_candidate(binary.lhs, -*delta);
      }
    }
  }

  if (binary.opcode == bir::BinaryOpcode::Add &&
      binary.rhs.type == bir::TypeKind::Ptr) {
    if (const auto delta = immediate_pointer_byte_delta(binary.lhs);
        delta.has_value()) {
      return make_candidate(binary.rhs, *delta);
    }
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<PreparedPointerCarrierState> resolve_prepared_pointer_carrier_state(
    ValueNameId value_name,
    const PreparedPointerCarrierMap& pointer_carriers,
    const std::unordered_map<ValueNameId, std::size_t>& direct_step_by_value_name,
    std::string_view slot_name,
    const std::unordered_map<std::string, PreparedPointerCarrierState>& slot_pointer_carriers) {
  if (const auto carrier_it = pointer_carriers.find(value_name); carrier_it != pointer_carriers.end()) {
    return carrier_it->second;
  }
  if (const auto direct_step_it = direct_step_by_value_name.find(value_name);
      direct_step_it != direct_step_by_value_name.end()) {
    return prepared_pointer_value_access_carrier(value_name, direct_step_it->second);
  }
  if (!slot_name.empty()) {
    if (const auto slot_it = slot_pointer_carriers.find(std::string(slot_name));
        slot_it != slot_pointer_carriers.end()) {
      return slot_it->second;
    }
  }
  return std::nullopt;
}

void maybe_update_prepared_pointer_carrier(
    PreparedPointerCarrierMap& pointer_carriers,
    ValueNameId value_name,
    const PreparedPointerCarrierState& candidate,
    bool* changed) {
  if (value_name == kInvalidValueName || candidate.base_value_name == kInvalidValueName ||
      candidate.step_bytes == 0 || !has_semantic_pointer_carrier_authority(candidate)) {
    return;
  }
  const auto [it, inserted] = pointer_carriers.emplace(value_name, candidate);
  if (inserted) {
    if (changed != nullptr) {
      *changed = true;
    }
    return;
  }
  if (it->second.base_value_name == candidate.base_value_name &&
      it->second.base_symbol_name == candidate.base_symbol_name &&
      it->second.byte_delta == candidate.byte_delta &&
      it->second.step_bytes == candidate.step_bytes &&
      it->second.authority == candidate.authority) {
    return;
  }
  if (candidate.base_value_name == value_name && candidate.byte_delta == 0) {
    if (it->second.base_value_name != value_name || it->second.byte_delta != 0 ||
        candidate.step_bytes < it->second.step_bytes || it->second.step_bytes == 0) {
      it->second = candidate;
      if (changed != nullptr) {
        *changed = true;
      }
    }
    return;
  }
  if (it->second.base_value_name == value_name && it->second.byte_delta == 0) {
    it->second = candidate;
    if (changed != nullptr) {
      *changed = true;
    }
  }
}

void maybe_update_slot_pointer_carrier(
    std::unordered_map<std::string, PreparedPointerCarrierState>& slot_pointer_carriers,
    std::string_view slot_name,
    const PreparedPointerCarrierState& candidate,
    bool* changed) {
  if (slot_name.empty() || candidate.base_value_name == kInvalidValueName || candidate.step_bytes == 0) {
    return;
  }
  if (!has_semantic_pointer_carrier_authority(candidate)) {
    return;
  }
  const auto [it, inserted] =
      slot_pointer_carriers.emplace(std::string(slot_name), candidate);
  if (inserted) {
    if (changed != nullptr) {
      *changed = true;
    }
    return;
  }
  if (it->second.base_value_name == candidate.base_value_name &&
      it->second.base_symbol_name == candidate.base_symbol_name &&
      it->second.byte_delta == candidate.byte_delta &&
      it->second.step_bytes == candidate.step_bytes &&
      it->second.authority == candidate.authority) {
    return;
  }
  it->second = candidate;
  if (changed != nullptr) {
    *changed = true;
  }
}

PreparedPointerCarrierMap build_pointer_carrier_map(
    PreparedNameTables& names,
    const bir::Module& module,
    const bir::Function& function,
    const PreparedAddressingFunction* function_addressing) {
  std::unordered_map<ValueNameId, std::size_t> direct_step_by_value_name;
  if (function_addressing != nullptr) {
    for (const auto& access : function_addressing->accesses) {
      if (access.address.base_kind != PreparedAddressBaseKind::PointerValue ||
          !access.address.pointer_value_name.has_value()) {
        continue;
      }
      update_prepared_pointer_value_access_step(
          direct_step_by_value_name, *access.address.pointer_value_name, access.address.size_bytes, nullptr);
    }
  }

  PreparedPointerCarrierMap pointer_carriers;
  for (const auto& [value_name, step_bytes] : direct_step_by_value_name) {
    maybe_update_prepared_pointer_carrier(
        pointer_carriers,
        value_name,
        prepared_pointer_value_access_carrier(value_name, step_bytes),
        nullptr);
  }
  if (function_addressing != nullptr) {
    for (const auto& materialization : function_addressing->address_materializations) {
      if (materialization.kind != PreparedAddressMaterializationKind::FrameSlot ||
          !materialization.result_value_name.has_value() ||
          !materialization.frame_slot_id.has_value()) {
        continue;
      }
      maybe_update_prepared_pointer_carrier(
          pointer_carriers,
          *materialization.result_value_name,
          prepared_frame_address_materialization_carrier(*materialization.result_value_name),
          nullptr);
    }
  }
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* result = pointer_result_value(inst);
      if (result == nullptr || result->kind != bir::Value::Kind::Named) {
        continue;
      }
      const auto value_name = names.value_names.find(result->name);
      const auto symbol_name = prepared_pointer_symbol_name(names, module, *result);
      if (value_name == kInvalidValueName || !symbol_name.has_value()) {
        continue;
      }
      maybe_update_prepared_pointer_carrier(
          pointer_carriers,
          value_name,
          bir_pointer_symbol_carrier(value_name, *symbol_name),
          nullptr);
    }
  }
  const auto cfg_cycle_block_labels = collect_cfg_cycle_block_labels(function);
  const auto risky_cycle_pointer_slots =
      collect_risky_cycle_pointer_slots(function, cfg_cycle_block_labels);
  std::unordered_map<std::string, PreparedPointerCarrierState> slot_pointer_carriers;
  bool changed = true;
  std::size_t remaining_iterations = function.blocks.size() * 4U + 1U;
  while (changed && remaining_iterations-- != 0) {
    changed = false;
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        if (const auto* load = std::get_if<bir::LoadLocalInst>(&inst);
            load != nullptr && load->result.kind == bir::Value::Kind::Named &&
            load->result.type == bir::TypeKind::Ptr) {
          const auto result_name = names.value_names.find(load->result.name);
          if (result_name != kInvalidValueName) {
            if (!load->address.has_value()) {
              const bool slot_allows_pointer_carrier =
                  risky_cycle_pointer_slots.find(load->slot_name) ==
                  risky_cycle_pointer_slots.end();
              if (const auto carrier = resolve_prepared_pointer_carrier_state(
                      result_name,
                      pointer_carriers,
                      direct_step_by_value_name,
                      slot_allows_pointer_carrier ? load->slot_name : std::string_view{},
                      slot_allows_pointer_carrier
                          ? slot_pointer_carriers
                          : std::unordered_map<std::string, PreparedPointerCarrierState>{});
                  carrier.has_value()) {
                maybe_update_prepared_pointer_carrier(pointer_carriers, result_name, *carrier, &changed);
              }
            }
          }
          continue;
        }
        if (const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
            store != nullptr && store->value.kind == bir::Value::Kind::Named &&
            store->value.type == bir::TypeKind::Ptr) {
          const auto stored_value_name = names.value_names.find(store->value.name);
          if (stored_value_name == kInvalidValueName) {
            continue;
          }
          if (const auto stored_value_state = resolve_prepared_pointer_carrier_state(
                  stored_value_name, pointer_carriers, direct_step_by_value_name, {}, slot_pointer_carriers);
              stored_value_state.has_value() && !store->address.has_value()) {
            maybe_update_prepared_pointer_carrier(
                pointer_carriers, stored_value_name, *stored_value_state, &changed);
            if (risky_cycle_pointer_slots.find(store->slot_name) ==
                risky_cycle_pointer_slots.end()) {
              maybe_update_slot_pointer_carrier(
                  slot_pointer_carriers, store->slot_name, *stored_value_state, &changed);
            }
          }
          continue;
        }
        if (const auto* binary = std::get_if<bir::BinaryInst>(&inst);
            binary != nullptr) {
          const auto result_name = names.value_names.find(binary->result.name);
          if (result_name == kInvalidValueName) {
            continue;
          }
          if (const auto carrier = bir_pointer_immediate_offset_carrier(
                  names, *binary, pointer_carriers, direct_step_by_value_name);
              carrier.has_value()) {
            maybe_update_prepared_pointer_carrier(
                pointer_carriers, result_name, *carrier, &changed);
          }
          continue;
        }
      }
    }
  }
  return pointer_carriers;
}

}  // namespace c4c::backend::prepare::regalloc_detail
