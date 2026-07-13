#include "classification.hpp"

#include "../module.hpp"

#include <algorithm>

namespace c4c::backend::prepare::regalloc_detail {

PreparedRegisterClass classify_register_class(const PreparedLivenessValue& value) {
  switch (value.type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return PreparedRegisterClass::General;
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return PreparedRegisterClass::Float;
    case bir::TypeKind::Vrm1:
    case bir::TypeKind::Vrm2:
    case bir::TypeKind::Vrm4:
    case bir::TypeKind::Vrm8:
      return PreparedRegisterClass::Vector;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
      return PreparedRegisterClass::None;
  }
  return PreparedRegisterClass::None;
}

PreparedRegisterClass resolve_register_class(const PreparedBirModule& prepared,
                                             const PreparedLivenessValue& value) {
  // Step 5 fence: overrides are prepared-route annotations keyed by interned
  // function/value IDs; they select target register classes, not value identity.
  if (const auto* override =
          find_prepared_register_group_override(prepared, value.function_name, value.value_name);
      override != nullptr && override->register_class != PreparedRegisterClass::None) {
    return override->register_class;
  }
  return classify_register_class(value);
}

std::size_t resolve_register_group_width(const PreparedBirModule& prepared,
                                         const PreparedLivenessValue& value) {
  // Step 5 fence: contiguous width follows the same prepared-route override as
  // register class and is not a raw spelling lookup.
  if (const auto* override =
          find_prepared_register_group_override(prepared, value.function_name, value.value_name);
      override != nullptr) {
    return std::max<std::size_t>(override->contiguous_width, 1);
  }
  if (const std::size_t width = bir::vrm_register_group_width(value.type); width != 0) {
    return width;
  }
  return 1;
}

PreparedRegisterBank register_bank_from_class(PreparedRegisterClass reg_class) {
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

std::vector<std::string> materialize_register_names(
    const std::vector<std::string_view>& register_names) {
  std::vector<std::string> materialized;
  materialized.reserve(register_names.size());
  for (const std::string_view register_name : register_names) {
    materialized.emplace_back(register_name);
  }
  return materialized;
}

std::vector<PreparedRegisterPlacement> materialize_register_placements(
    const std::vector<PreparedRegisterCandidateSpan>& spans) {
  std::vector<PreparedRegisterPlacement> placements;
  placements.reserve(spans.size());
  for (const auto& span : spans) {
    if (span.placement.has_value()) {
      placements.push_back(*span.placement);
    }
  }
  return placements;
}

std::size_t published_register_group_width(const PreparedRegallocValue& value) {
  if (value.assigned_register.has_value()) {
    return std::max<std::size_t>(value.assigned_register->contiguous_width, 1);
  }
  if (value.spill_register_authority.has_value()) {
    return std::max<std::size_t>(value.spill_register_authority->contiguous_width, 1);
  }
  return std::max<std::size_t>(value.register_group_width, 1);
}

std::optional<PreparedRegisterPlacement> assigned_register_placement(
    const PreparedRegallocValue& value) {
  if (!value.assigned_register.has_value()) {
    return std::nullopt;
  }
  return value.assigned_register->placement;
}

}  // namespace c4c::backend::prepare::regalloc_detail
