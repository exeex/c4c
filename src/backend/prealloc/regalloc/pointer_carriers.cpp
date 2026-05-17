#include "pointer_carriers.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace c4c::backend::prepare::regalloc_detail {

[[nodiscard]] std::optional<ValueNameId> prepare_inst_result_value_name(
    PreparedNameTables& names,
    const bir::Inst& inst) {
  return std::visit(
      [&](const auto& typed_inst) -> std::optional<ValueNameId> {
        using InstT = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<InstT, bir::BinaryInst> || std::is_same_v<InstT, bir::SelectInst> ||
                      std::is_same_v<InstT, bir::CastInst> || std::is_same_v<InstT, bir::PhiInst> ||
                      std::is_same_v<InstT, bir::LoadLocalInst> ||
                      std::is_same_v<InstT, bir::LoadGlobalInst>) {
          return prepared_named_value_id(names, typed_inst.result);
        } else if constexpr (std::is_same_v<InstT, bir::CallInst>) {
          if (!typed_inst.result.has_value()) {
            return std::nullopt;
          }
          return prepared_named_value_id(names, *typed_inst.result);
        } else {
          return std::nullopt;
        }
      },
      inst);
}

void update_prepared_pointer_step(std::unordered_map<ValueNameId, std::size_t>& steps,
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
    return PreparedPointerCarrierState{
        .base_value_name = value_name,
        .byte_delta = 0,
        .step_bytes = direct_step_it->second,
    };
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
      candidate.step_bytes == 0) {
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
      it->second.byte_delta == candidate.byte_delta && it->second.step_bytes == candidate.step_bytes) {
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
  const auto [it, inserted] =
      slot_pointer_carriers.emplace(std::string(slot_name), candidate);
  if (inserted) {
    if (changed != nullptr) {
      *changed = true;
    }
    return;
  }
  if (it->second.base_value_name == candidate.base_value_name &&
      it->second.byte_delta == candidate.byte_delta && it->second.step_bytes == candidate.step_bytes) {
    return;
  }
  it->second = candidate;
  if (changed != nullptr) {
    *changed = true;
  }
}

PreparedPointerCarrierMap build_pointer_carrier_map(
    PreparedNameTables& names,
    const bir::Function& function,
    const PreparedAddressingFunction* function_addressing) {
  std::unordered_set<ValueNameId> explicit_pointer_defs;
  explicit_pointer_defs.reserve(function.params.size() + function.blocks.size() * 4U);
  for (const auto& param : function.params) {
    if (param.type != bir::TypeKind::Ptr) {
      continue;
    }
    if (const auto value_name = names.value_names.find(param.name); value_name != kInvalidValueName) {
      explicit_pointer_defs.insert(value_name);
    }
  }
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto value_name = prepare_inst_result_value_name(names, inst);
      if (!value_name.has_value()) {
        continue;
      }
      const auto* result_type = std::visit(
          [&](const auto& typed_inst) -> const bir::TypeKind* {
            using InstT = std::decay_t<decltype(typed_inst)>;
            if constexpr (std::is_same_v<InstT, bir::BinaryInst> || std::is_same_v<InstT, bir::SelectInst> ||
                          std::is_same_v<InstT, bir::CastInst> || std::is_same_v<InstT, bir::PhiInst> ||
                          std::is_same_v<InstT, bir::LoadLocalInst> ||
                          std::is_same_v<InstT, bir::LoadGlobalInst>) {
              return &typed_inst.result.type;
            } else if constexpr (std::is_same_v<InstT, bir::CallInst>) {
              return typed_inst.result.has_value() ? &typed_inst.result->type : nullptr;
            } else {
              return static_cast<const bir::TypeKind*>(nullptr);
            }
          },
          inst);
      if (result_type != nullptr && *result_type == bir::TypeKind::Ptr) {
        explicit_pointer_defs.insert(*value_name);
      }
    }
  }

  std::unordered_map<ValueNameId, std::size_t> direct_step_by_value_name;
  if (function_addressing != nullptr) {
    for (const auto& access : function_addressing->accesses) {
      if (access.address.base_kind != PreparedAddressBaseKind::PointerValue ||
          !access.address.pointer_value_name.has_value()) {
        continue;
      }
      update_prepared_pointer_step(
          direct_step_by_value_name, *access.address.pointer_value_name, access.address.size_bytes, nullptr);
    }
  }

  PreparedPointerCarrierMap pointer_carriers;
  for (const auto& [value_name, step_bytes] : direct_step_by_value_name) {
    maybe_update_prepared_pointer_carrier(
        pointer_carriers,
        value_name,
        PreparedPointerCarrierState{
            .base_value_name = value_name,
            .byte_delta = 0,
            .step_bytes = step_bytes,
        },
        nullptr);
  }
  std::unordered_map<std::string, PreparedPointerCarrierState> slot_pointer_carriers;
  bool changed = true;
  std::size_t remaining_iterations = function.blocks.size() * 4U + 1U;
  while (changed && remaining_iterations-- != 0) {
    changed = false;
    for (const auto& block : function.blocks) {
      std::unordered_map<std::string, PreparedPointerCarrierState> last_loaded_pointer_by_slot;
      std::unordered_map<std::string, ValueNameId> last_loaded_pointer_name_by_slot;
      std::optional<PreparedPointerCarrierState> last_loaded_pointer_state;
      std::optional<ValueNameId> last_loaded_pointer_name;
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
                last_loaded_pointer_by_slot[load->slot_name] = *carrier;
                last_loaded_pointer_name_by_slot[load->slot_name] = result_name;
                last_loaded_pointer_state = *carrier;
                last_loaded_pointer_name = result_name;
              }
            } else if (const auto carrier = resolve_prepared_pointer_carrier_state(
                           result_name, pointer_carriers, direct_step_by_value_name, {}, slot_pointer_carriers);
                       carrier.has_value()) {
              last_loaded_pointer_state = *carrier;
              last_loaded_pointer_name = result_name;
            }
          }
          continue;
        }
        if (const auto* load = std::get_if<bir::LoadGlobalInst>(&inst);
            load != nullptr && load->result.kind == bir::Value::Kind::Named &&
            load->result.type == bir::TypeKind::Ptr) {
          const auto result_name = names.value_names.find(load->result.name);
          if (result_name != kInvalidValueName) {
            if (const auto carrier = resolve_prepared_pointer_carrier_state(
                    result_name, pointer_carriers, direct_step_by_value_name, {}, slot_pointer_carriers);
                carrier.has_value()) {
              last_loaded_pointer_state = *carrier;
              last_loaded_pointer_name = result_name;
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
          if (explicit_pointer_defs.count(stored_value_name) != 0 ||
              resolve_prepared_pointer_carrier_state(
                  stored_value_name, pointer_carriers, direct_step_by_value_name, {}, slot_pointer_carriers)
                  .has_value()) {
            continue;
          }
          if (!store->address.has_value()) {
            const auto base_it = last_loaded_pointer_by_slot.find(store->slot_name);
            const auto base_name_it = last_loaded_pointer_name_by_slot.find(store->slot_name);
            if (base_it == last_loaded_pointer_by_slot.end() ||
                base_name_it == last_loaded_pointer_name_by_slot.end()) {
              continue;
            }
            auto derived_state = base_it->second;
            derived_state.base_value_name = base_name_it->second;
            derived_state.byte_delta = static_cast<std::int64_t>(derived_state.step_bytes);
            maybe_update_prepared_pointer_carrier(
                pointer_carriers, stored_value_name, derived_state, &changed);
            maybe_update_slot_pointer_carrier(
                slot_pointer_carriers, store->slot_name, derived_state, &changed);
            continue;
          }
          if (store->address->base_kind != bir::MemoryAddress::BaseKind::PointerValue ||
              !last_loaded_pointer_state.has_value() || !last_loaded_pointer_name.has_value()) {
            continue;
          }
          auto derived_state = *last_loaded_pointer_state;
          derived_state.base_value_name = *last_loaded_pointer_name;
          derived_state.byte_delta = -static_cast<std::int64_t>(derived_state.step_bytes);
          maybe_update_prepared_pointer_carrier(
              pointer_carriers, stored_value_name, derived_state, &changed);
          continue;
        }
        if (const auto* store = std::get_if<bir::StoreGlobalInst>(&inst);
            store != nullptr && store->value.kind == bir::Value::Kind::Named &&
            store->value.type == bir::TypeKind::Ptr) {
          const auto stored_value_name = names.value_names.find(store->value.name);
          if (stored_value_name == kInvalidValueName ||
              explicit_pointer_defs.count(stored_value_name) != 0 || !store->address.has_value() ||
              store->address->base_kind != bir::MemoryAddress::BaseKind::PointerValue ||
              !last_loaded_pointer_state.has_value() || !last_loaded_pointer_name.has_value()) {
            continue;
          }
          auto derived_state = *last_loaded_pointer_state;
          derived_state.base_value_name = *last_loaded_pointer_name;
          derived_state.byte_delta = -static_cast<std::int64_t>(derived_state.step_bytes);
          maybe_update_prepared_pointer_carrier(
              pointer_carriers, stored_value_name, derived_state, &changed);
        }
      }
    }
  }
  return pointer_carriers;
}

}  // namespace c4c::backend::prepare::regalloc_detail
