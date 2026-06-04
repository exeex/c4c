#include "value_homes.hpp"

#include "../control_flow.hpp"
#include "../frame.hpp"
#include "../names.hpp"
#include "../target_register_profile.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace c4c::backend::prepare::regalloc_detail {

namespace {

[[nodiscard]] bool same_aarch64_formal_register_bank(const bir::CallArgAbiInfo& lhs,
                                                     const bir::CallArgAbiInfo& rhs) {
  const auto is_float_bank = [](const bir::CallArgAbiInfo& abi) {
    return abi.primary_class == bir::AbiValueClass::Sse ||
           abi.primary_class == bir::AbiValueClass::X87;
  };
  if (is_float_bank(lhs) || is_float_bank(rhs)) {
    return is_float_bank(lhs) == is_float_bank(rhs);
  }
  return lhs.primary_class == bir::AbiValueClass::Integer &&
         rhs.primary_class == bir::AbiValueClass::Integer;
}

[[nodiscard]] std::optional<std::size_t> fixed_formal_abi_register_index(
    const c4c::TargetProfile& target_profile,
    const bir::Function& function,
    std::size_t param_index) {
  if (param_index >= function.params.size()) {
    return std::nullopt;
  }
  const auto& param = function.params[param_index];
  if (!param.abi.has_value() || !param.abi->passed_in_register) {
    if (target_profile.arch == c4c::TargetArch::Aarch64 &&
        param.abi.has_value() &&
        param.abi->type == bir::TypeKind::Ptr &&
        param.abi->sret_pointer) {
      return 0;
    }
    return std::nullopt;
  }
  if (target_profile.arch != c4c::TargetArch::Aarch64) {
    return param_index;
  }
  if (param.abi->type == bir::TypeKind::Ptr && param.abi->sret_pointer) {
    return 0;
  }

  std::size_t register_index = 0;
  for (std::size_t candidate_index = 0; candidate_index < param_index; ++candidate_index) {
    const auto& candidate = function.params[candidate_index];
    if (!candidate.abi.has_value() || !candidate.abi->passed_in_register) {
      continue;
    }
    if (candidate.abi->type == bir::TypeKind::Ptr && candidate.abi->sret_pointer) {
      continue;
    }
    if (same_aarch64_formal_register_bank(*candidate.abi, *param.abi)) {
      ++register_index;
    }
  }
  return register_index;
}

[[nodiscard]] const PreparedFrameSlot* find_frame_slot_by_id(
    const PreparedStackLayout& stack_layout,
    PreparedFrameSlotId slot_id) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return &slot;
    }
  }
  return nullptr;
}

[[nodiscard]] const PreparedFrameSlot* find_stack_passed_f128_formal_local_home(
    const PreparedStackLayout* stack_layout,
    const PreparedAddressingFunction* function_addressing,
    ValueNameId value_name) {
  if (stack_layout == nullptr || function_addressing == nullptr ||
      value_name == kInvalidValueName) {
    return nullptr;
  }

  const PreparedFrameSlot* matched_slot = nullptr;
  for (const auto& access : function_addressing->accesses) {
    if (access.stored_value_name != value_name ||
        access.address.base_kind != PreparedAddressBaseKind::FrameSlot ||
        !access.address.frame_slot_id.has_value()) {
      continue;
    }
    const auto* slot = find_frame_slot_by_id(*stack_layout, *access.address.frame_slot_id);
    if (slot == nullptr) {
      continue;
    }
    if (matched_slot != nullptr && matched_slot->slot_id != slot->slot_id) {
      return nullptr;
    }
    matched_slot = slot;
  }
  return matched_slot;
}

[[nodiscard]] PreparedComputedValueLookup make_prepared_computed_value_lookup(
    PreparedNameTables& names,
    const c4c::backend::bir::Function* function) {
  PreparedComputedValueLookup lookup;
  lookup.bir_names.import_link_names(names.texts, names.link_names);
  if (function == nullptr) {
    return lookup;
  }
  for (const auto& block : function->blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* binary = std::get_if<bir::BinaryInst>(&inst);
          binary != nullptr && binary->result.kind == bir::Value::Kind::Named &&
          !binary->result.name.empty()) {
        if (const auto result_name = prepared_named_value_id(names, binary->result);
            result_name.has_value()) {
          lookup.binaries_by_value_name.emplace(*result_name, binary);
        }
      } else if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(&inst);
                 load_global != nullptr &&
                 load_global->result.kind == bir::Value::Kind::Named &&
                 !load_global->result.name.empty()) {
        if (const auto result_name = prepared_named_value_id(names, load_global->result);
            result_name.has_value()) {
          lookup.global_loads_by_value_name.emplace(*result_name, load_global);
        }
      }
    }
  }
  return lookup;
}

[[nodiscard]] std::unordered_set<std::string_view> collect_cfg_cycle_block_labels(
    const bir::Function* function) {
  if (function == nullptr) {
    return {};
  }
  std::unordered_map<std::string_view, std::size_t> block_index_by_label;
  block_index_by_label.reserve(function->blocks.size());
  for (std::size_t index = 0; index < function->blocks.size(); ++index) {
    block_index_by_label.emplace(function->blocks[index].label, index);
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
          cycle_labels.insert(function->blocks[index].label);
        }
      };
  for (std::size_t index = 0; index < function->blocks.size(); ++index) {
    const auto& terminator = function->blocks[index].terminator;
    add_backedge_range(index, terminator.target_label);
    add_backedge_range(index, terminator.false_label);
  }
  return cycle_labels;
}

[[nodiscard]] std::unordered_set<ValueNameId> collect_local_pointer_load_results(
    PreparedNameTables& names,
    const c4c::backend::bir::Function* function,
    const PreparedAddressingFunction* function_addressing,
    const std::unordered_set<std::string_view>& cycle_block_labels) {
  std::unordered_set<ValueNameId> results;
  if (function == nullptr) {
    return results;
  }
  std::unordered_set<ValueNameId> pointer_memory_base_values;
  if (function_addressing != nullptr) {
    for (const auto& access : function_addressing->accesses) {
      if (access.address.base_kind == PreparedAddressBaseKind::PointerValue &&
          access.address.pointer_value_name.has_value()) {
        pointer_memory_base_values.insert(*access.address.pointer_value_name);
      }
    }
  }
  std::unordered_map<std::string, std::size_t> pointer_stores_by_slot;
  for (const auto& block : function->blocks) {
    if (cycle_block_labels.find(block.label) == cycle_block_labels.end()) {
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
  for (const auto& block : function->blocks) {
    const bool is_cycle_block =
        cycle_block_labels.find(block.label) != cycle_block_labels.end();
    for (const auto& inst : block.insts) {
      const auto* load = std::get_if<bir::LoadLocalInst>(&inst);
      if (load == nullptr ||
          load->result.kind != bir::Value::Kind::Named ||
          load->result.type != bir::TypeKind::Ptr) {
        continue;
      }
      if (const auto value_name = prepared_named_value_id(names, load->result);
          value_name.has_value()) {
        const bool risky_cycle_load =
            is_cycle_block && risky_slots.find(load->slot_name) != risky_slots.end();
        const bool memory_base_load =
            pointer_memory_base_values.find(*value_name) != pointer_memory_base_values.end();
        if (risky_cycle_load || memory_base_load) {
          results.insert(*value_name);
        }
      }
    }
  }
  results.insert(pointer_memory_base_values.begin(), pointer_memory_base_values.end());
  return results;
}

}  // namespace

PreparedValueHome classify_prepared_value_home(
    PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const c4c::backend::bir::Function* function,
    const PreparedStackLayout* stack_layout,
    const PreparedAddressingFunction* function_addressing,
    const PreparedPointerCarrierMap& pointer_carriers,
    const PreparedComputedValueLookup& computed_values,
    const std::unordered_set<ValueNameId>& local_pointer_load_results,
    const PreparedRegallocValue& value) {
  PreparedValueHome home{
      .value_id = value.value_id,
      .function_name = value.function_name,
      .value_name = value.value_name,
      .kind = PreparedValueHomeKind::None,
      .register_name = std::nullopt,
      .slot_id = std::nullopt,
      .offset_bytes = std::nullopt,
      .size_bytes = std::nullopt,
      .align_bytes = std::nullopt,
      .immediate_i32 = std::nullopt,
      .immediate_f128 = std::nullopt,
      .pointer_base_value_name = std::nullopt,
      .pointer_byte_delta = std::nullopt,
  };
  if (value.constant_f128_payload.has_value()) {
    home.kind = PreparedValueHomeKind::RematerializableImmediate;
    home.immediate_f128 = value.constant_f128_payload;
    return home;
  }
  if (function != nullptr && value.value_kind == PreparedValueKind::Parameter) {
    const std::string_view value_name = prepared_value_name(names, value.value_name);
    for (std::size_t param_index = 0; param_index < function->params.size(); ++param_index) {
      const auto& param = function->params[param_index];
      const bool aarch64_sret_pointer_param =
          target_profile.arch == c4c::TargetArch::Aarch64 &&
          param.is_sret &&
          param.abi.has_value() &&
          param.abi->type == bir::TypeKind::Ptr &&
          param.abi->sret_pointer;
      const bool aarch64_indirect_byval_param =
          target_profile.arch == c4c::TargetArch::Aarch64 &&
          param.is_byval &&
          param.abi.has_value() &&
          param.abi->type == bir::TypeKind::Ptr &&
          param.abi->byval_copy &&
          param.size_bytes > 16;
      if (param.name != value_name || !param.abi.has_value() || param.is_varargs ||
          (param.is_sret && !aarch64_sret_pointer_param) ||
          (param.is_byval && !aarch64_indirect_byval_param)) {
        continue;
      }
      if (function->is_variadic && value.assigned_stack_slot.has_value()) {
        home.kind = PreparedValueHomeKind::StackSlot;
        home.slot_id = value.assigned_stack_slot->slot_id;
        home.offset_bytes = value.assigned_stack_slot->offset_bytes;
        home.size_bytes = value.assigned_stack_slot->size_bytes;
        home.align_bytes = value.assigned_stack_slot->align_bytes;
        return home;
      }
      if (function->is_variadic && value.assigned_register.has_value()) {
        home.kind = PreparedValueHomeKind::Register;
        home.register_name = value.assigned_register->register_name;
        return home;
      }
      if (param.abi->passed_on_stack && param.type == bir::TypeKind::F128) {
        if (const auto* slot = find_stack_passed_f128_formal_local_home(
                stack_layout, function_addressing, value.value_name);
            slot != nullptr) {
          home.kind = PreparedValueHomeKind::StackSlot;
          home.slot_id = slot->slot_id;
          home.offset_bytes = slot->offset_bytes;
          home.size_bytes = slot->size_bytes;
          home.align_bytes = slot->align_bytes;
          return home;
        }
      }
      const auto abi_register_index =
          fixed_formal_abi_register_index(target_profile, *function, param_index);
      if (const auto register_name =
              abi_register_index.has_value()
                  ? call_arg_destination_register_name(target_profile, *param.abi, *abi_register_index)
                  : std::nullopt;
          register_name.has_value()) {
        home.kind = PreparedValueHomeKind::Register;
        home.register_name = *register_name;
      }
      return home;
    }
  }
  if (function != nullptr && value.type == bir::TypeKind::I32) {
    const bir::Value named_value =
        bir::Value::named(value.type, std::string(prepared_value_name(names, value.value_name)));
    if (const auto computed_value =
            classify_computed_value(names,
                                    computed_values.bir_names,
                                    named_value,
                                    *function,
                                    computed_values.binaries_by_value_name,
                                    computed_values.global_loads_by_value_name);
        computed_value.has_value() &&
        computed_value->base.kind == PreparedComputedBaseKind::ImmediateI32) {
      auto current_value = static_cast<std::int32_t>(computed_value->base.immediate);
      bool supported = true;
      for (const auto& operation : computed_value->operations) {
        const auto immediate = static_cast<std::int32_t>(operation.immediate);
        switch (operation.opcode) {
          case bir::BinaryOpcode::Add:
            current_value = static_cast<std::int32_t>(current_value + immediate);
            break;
          case bir::BinaryOpcode::Mul:
            current_value = static_cast<std::int32_t>(current_value * immediate);
            break;
          case bir::BinaryOpcode::And:
            current_value = static_cast<std::int32_t>(current_value & immediate);
            break;
          case bir::BinaryOpcode::Or:
            current_value = static_cast<std::int32_t>(current_value | immediate);
            break;
          case bir::BinaryOpcode::Xor:
            current_value = static_cast<std::int32_t>(current_value ^ immediate);
            break;
          case bir::BinaryOpcode::Sub:
            current_value = static_cast<std::int32_t>(current_value - immediate);
            break;
          case bir::BinaryOpcode::Shl:
            current_value =
                static_cast<std::int32_t>(static_cast<std::uint32_t>(current_value) << immediate);
            break;
          case bir::BinaryOpcode::LShr:
            current_value = static_cast<std::int32_t>(
                static_cast<std::uint32_t>(current_value) >> immediate);
            break;
          case bir::BinaryOpcode::AShr:
            current_value = static_cast<std::int32_t>(current_value >> immediate);
            break;
          default:
            supported = false;
            break;
        }
        if (!supported) {
          break;
        }
      }
      if (supported) {
        home.kind = PreparedValueHomeKind::RematerializableImmediate;
        home.immediate_i32 = current_value;
        return home;
      }
    }
  }
  if (value.type == bir::TypeKind::Ptr &&
      local_pointer_load_results.find(value.value_name) == local_pointer_load_results.end()) {
    if (const auto carrier_it = pointer_carriers.find(value.value_name);
        carrier_it != pointer_carriers.end() &&
        has_semantic_pointer_carrier_authority(carrier_it->second) &&
        (carrier_it->second.base_value_name != value.value_name ||
         carrier_it->second.byte_delta != 0)) {
      home.kind = PreparedValueHomeKind::PointerBasePlusOffset;
      home.pointer_base_value_name = carrier_it->second.base_value_name;
      home.pointer_base_symbol_name = carrier_it->second.base_symbol_name;
      home.pointer_byte_delta = carrier_it->second.byte_delta;
      if (value.assigned_register.has_value()) {
        home.register_name = value.assigned_register->register_name;
      }
      if (value.assigned_stack_slot.has_value()) {
        home.slot_id = value.assigned_stack_slot->slot_id;
        home.offset_bytes = value.assigned_stack_slot->offset_bytes;
        home.size_bytes = value.assigned_stack_slot->size_bytes;
        home.align_bytes = value.assigned_stack_slot->align_bytes;
      }
      return home;
    }
  }
  if (value.assigned_register.has_value()) {
    home.kind = PreparedValueHomeKind::Register;
    home.register_name = value.assigned_register->register_name;
    return home;
  }
  if (value.assigned_stack_slot.has_value()) {
    home.kind = PreparedValueHomeKind::StackSlot;
    home.slot_id = value.assigned_stack_slot->slot_id;
    home.offset_bytes = value.assigned_stack_slot->offset_bytes;
    home.size_bytes = value.assigned_stack_slot->size_bytes;
    home.align_bytes = value.assigned_stack_slot->align_bytes;
    return home;
  }
  return home;
}

std::vector<PreparedValueHome> build_prepared_value_homes(
    PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Function* function,
    const PreparedStackLayout* stack_layout,
    const PreparedAddressingFunction* function_addressing,
    const PreparedRegallocFunction& regalloc_function) {
  const auto pointer_carriers =
      function == nullptr ? PreparedPointerCarrierMap{}
                          : build_pointer_carrier_map(names, module, *function, function_addressing);
  const auto computed_values = make_prepared_computed_value_lookup(names, function);
  const auto local_pointer_load_results =
      collect_local_pointer_load_results(
          names, function, function_addressing, collect_cfg_cycle_block_labels(function));
  std::vector<PreparedValueHome> value_homes;
  value_homes.reserve(regalloc_function.values.size());
  for (const auto& value : regalloc_function.values) {
    value_homes.push_back(
        classify_prepared_value_home(names,
                                     target_profile,
                                     function,
                                     stack_layout,
                                     function_addressing,
                                     pointer_carriers,
                                     computed_values,
                                     local_pointer_load_results,
                                     value));
  }
  return value_homes;
}

}  // namespace c4c::backend::prepare::regalloc_detail
