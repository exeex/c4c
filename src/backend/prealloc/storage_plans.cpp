#include "storage_plans.hpp"
#include "target_register_profile.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace c4c::backend::prepare {

namespace {

[[nodiscard]] PreparedRegisterBank register_bank_from_class(PreparedRegisterClass reg_class) {
  switch (reg_class) {
    case PreparedRegisterClass::General:
      return PreparedRegisterBank::Gpr;
    case PreparedRegisterClass::Float:
      return PreparedRegisterBank::Fpr;
    case PreparedRegisterClass::Vector:
      return PreparedRegisterBank::Vreg;
    case PreparedRegisterClass::AggregateAddress:
      return PreparedRegisterBank::AggregateAddress;
    case PreparedRegisterClass::None:
      return PreparedRegisterBank::None;
  }
  return PreparedRegisterBank::None;
}

[[nodiscard]] PreparedRegisterBank register_bank_from_type(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return PreparedRegisterBank::Gpr;
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
      return PreparedRegisterBank::Fpr;
    case bir::TypeKind::I128:
    case bir::TypeKind::F128:
      return PreparedRegisterBank::Vreg;
    case bir::TypeKind::Void:
      return PreparedRegisterBank::None;
  }
  return PreparedRegisterBank::None;
}

[[nodiscard]] std::optional<PreparedSpillSlotPlacement> make_spill_slot_placement(
    std::optional<PreparedFrameSlotId> slot_id,
    std::optional<std::size_t> offset_bytes) {
  if (!slot_id.has_value() || !offset_bytes.has_value()) {
    return std::nullopt;
  }
  return PreparedSpillSlotPlacement{
      .slot_id = *slot_id,
      .offset_bytes = *offset_bytes,
  };
}

[[nodiscard]] std::optional<PreparedRegisterPlacement> find_register_pool_placement(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::size_t contiguous_width,
    const std::vector<std::string>& occupied_register_names,
    bool caller_pool) {
  if (occupied_register_names.empty()) {
    return std::nullopt;
  }
  const auto spans = caller_pool ? caller_saved_register_spans(target_profile, reg_class, contiguous_width)
                                 : callee_saved_register_spans(target_profile, reg_class, contiguous_width);
  for (const auto& span : spans) {
    if (span.occupied_register_names == occupied_register_names) {
      return span.placement;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<PreparedRegisterPlacement> find_register_placement(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::size_t contiguous_width,
    const std::vector<std::string>& occupied_register_names) {
  if (const auto callee_saved =
          find_register_pool_placement(target_profile, reg_class, contiguous_width, occupied_register_names, false);
      callee_saved.has_value()) {
    return callee_saved;
  }
  return find_register_pool_placement(target_profile, reg_class, contiguous_width, occupied_register_names, true);
}

[[nodiscard]] std::optional<PreparedRegisterPlacement> assignment_register_placement(
    const c4c::TargetProfile& target_profile,
    const PreparedPhysicalRegisterAssignment& assignment) {
  if (assignment.placement.has_value()) {
    return assignment.placement;
  }
  return find_register_placement(target_profile,
                                 assignment.reg_class,
                                 assignment.contiguous_width,
                                 assignment.occupied_register_names);
}

[[nodiscard]] const PreparedRegallocFunction* find_regalloc_function(
    const PreparedRegalloc& regalloc,
    FunctionNameId function_name) {
  for (const auto& function : regalloc.functions) {
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] const PreparedRegallocValue* find_regalloc_value_by_name(
    const PreparedRegallocFunction& function,
    ValueNameId value_name) {
  for (const auto& value : function.values) {
    if (value.value_name == value_name) {
      return &value;
    }
  }
  return nullptr;
}

[[nodiscard]] const PreparedRegallocValue* find_regalloc_value_by_id(
    const PreparedRegallocFunction& function,
    PreparedValueId value_id) {
  for (const auto& value : function.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
}

struct RegallocValueIndexes {
  std::unordered_map<PreparedValueId, const PreparedRegallocValue*> by_id;
  std::unordered_map<ValueNameId, const PreparedRegallocValue*> by_name;
};

[[nodiscard]] RegallocValueIndexes make_regalloc_value_indexes(
    const PreparedRegallocFunction* function) {
  RegallocValueIndexes indexes;
  if (function == nullptr) {
    return indexes;
  }
  indexes.by_id.reserve(function->values.size());
  indexes.by_name.reserve(function->values.size());
  for (const auto& value : function->values) {
    indexes.by_id.emplace(value.value_id, &value);
    if (value.value_name != c4c::kInvalidValueName) {
      indexes.by_name.emplace(value.value_name, &value);
    }
  }
  return indexes;
}

[[nodiscard]] PreparedStorageEncodingKind storage_encoding_from_home(
    const PreparedValueHome& home) {
  switch (home.kind) {
    case PreparedValueHomeKind::Register:
      return PreparedStorageEncodingKind::Register;
    case PreparedValueHomeKind::StackSlot:
      return PreparedStorageEncodingKind::FrameSlot;
    case PreparedValueHomeKind::RematerializableImmediate:
      return PreparedStorageEncodingKind::Immediate;
    case PreparedValueHomeKind::PointerBasePlusOffset:
      return PreparedStorageEncodingKind::ComputedAddress;
    case PreparedValueHomeKind::None:
      return PreparedStorageEncodingKind::None;
  }
  return PreparedStorageEncodingKind::None;
}

[[nodiscard]] PreparedStoragePlanValue build_storage_plan_value(
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocValue* regalloc_value,
    const PreparedValueHome& home,
    bir::TypeKind type) {
  const bool home_is_assigned_register =
      regalloc_value != nullptr && regalloc_value->assigned_register.has_value() &&
      home.register_name ==
          std::optional<std::string>{regalloc_value->assigned_register->register_name};

  std::size_t contiguous_width = 1;
  PreparedRegisterBank bank = register_bank_from_type(type);
  std::vector<std::string> occupied_register_names;
  std::optional<PreparedRegisterPlacement> register_placement;
  if (home.register_name.has_value()) {
    occupied_register_names.push_back(*home.register_name);
  }
  if (regalloc_value != nullptr) {
    if (!home.register_name.has_value() || home_is_assigned_register) {
      contiguous_width = std::max<std::size_t>(regalloc_value->register_group_width, 1);
      bank = register_bank_from_class(regalloc_value->register_class);
    }
    if (home_is_assigned_register) {
      contiguous_width = regalloc_value->assigned_register->contiguous_width;
      occupied_register_names = regalloc_value->assigned_register->occupied_register_names;
      register_placement =
          assignment_register_placement(target_profile, *regalloc_value->assigned_register);
    } else if (home.kind != PreparedValueHomeKind::Register) {
      contiguous_width = std::max<std::size_t>(regalloc_value->register_group_width, 1);
      bank = register_bank_from_class(regalloc_value->register_class);
    }
  }
  return PreparedStoragePlanValue{
      .value_id = home.value_id,
      .value_name = home.value_name,
      .encoding = storage_encoding_from_home(home),
      .bank = bank,
      .contiguous_width = contiguous_width,
      .register_name = home.register_name,
      .occupied_register_names = std::move(occupied_register_names),
      .slot_id = home.slot_id,
      .stack_offset_bytes = home.offset_bytes,
      .immediate_i32 = home.immediate_i32,
      .immediate_f128 = home.immediate_f128,
      .symbol_name = std::nullopt,
      .register_placement = register_placement,
      .spill_slot_placement = make_spill_slot_placement(home.slot_id, home.offset_bytes),
  };
}

}  // namespace

void populate_storage_plans(PreparedBirModule& prepared) {
  prepared.storage_plans.functions.clear();

  for (const auto& function_locations : prepared.value_locations.functions) {
    PreparedStoragePlanFunction function_plan{
        .function_name = function_locations.function_name,
        .values = {},
    };

    const auto* regalloc_function = find_regalloc_function(prepared.regalloc, function_locations.function_name);
    const auto regalloc_value_indexes = make_regalloc_value_indexes(regalloc_function);
    function_plan.values.reserve(function_locations.value_homes.size());
    for (const auto& home : function_locations.value_homes) {
      bir::TypeKind type = bir::TypeKind::Void;
      const auto by_id_it = regalloc_value_indexes.by_id.find(home.value_id);
      const auto* typed_regalloc_value =
          by_id_it == regalloc_value_indexes.by_id.end() ? nullptr : by_id_it->second;
      if (typed_regalloc_value != nullptr) {
        type = typed_regalloc_value->type;
      }
      const auto by_name_it = regalloc_value_indexes.by_name.find(home.value_name);
      const auto* placed_regalloc_value =
          by_name_it == regalloc_value_indexes.by_name.end() ? nullptr : by_name_it->second;
      function_plan.values.push_back(
          build_storage_plan_value(prepared.target_profile, placed_regalloc_value, home, type));
    }

    if (!function_plan.values.empty()) {
      prepared.storage_plans.functions.push_back(std::move(function_plan));
    }
  }
}

}  // namespace c4c::backend::prepare
