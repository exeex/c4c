#include "pointer_carriers.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
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
              if (const auto carrier = resolve_prepared_pointer_carrier_state(
                      result_name, pointer_carriers, direct_step_by_value_name, load->slot_name,
                      slot_pointer_carriers);
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
            maybe_update_slot_pointer_carrier(
                slot_pointer_carriers, store->slot_name, *stored_value_state, &changed);
          }
          continue;
        }
      }
    }
  }
  return pointer_carriers;
}

}  // namespace c4c::backend::prepare::regalloc_detail
