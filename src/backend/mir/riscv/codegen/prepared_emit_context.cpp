#include "prepared_emit_context.hpp"

#include "../../../prealloc/addressing.hpp"

namespace c4c::backend::riscv::codegen {
namespace {

std::optional<c4c::backend::prepare::PreparedValueId> prepared_value_id_for(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  if (lookups == nullptr || value.kind != c4c::backend::bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto value_name = names.value_names.find(value.name);
  if (value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto value_id_it = lookups->value_homes.value_ids.find(value_name);
  if (value_id_it == lookups->value_homes.value_ids.end()) {
    return std::nullopt;
  }
  return value_id_it->second;
}

}  // namespace

const c4c::backend::prepare::PreparedValueHome* prepared_value_home_for(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  const auto value_id = prepared_value_id_for(names, lookups, value);
  if (!value_id.has_value()) {
    return nullptr;
  }
  const auto home_it = lookups->value_homes.homes_by_id.find(*value_id);
  if (home_it == lookups->value_homes.homes_by_id.end()) {
    return nullptr;
  }
  return home_it->second;
}

std::optional<std::string> prepared_register_for_value(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  const auto value_id = prepared_value_id_for(names, lookups, value);
  if (!value_id.has_value()) {
    return std::nullopt;
  }
  const auto home_it = lookups->value_homes.homes_by_id.find(*value_id);
  if (home_it == lookups->value_homes.homes_by_id.end() || home_it->second == nullptr) {
    return std::nullopt;
  }
  const auto& home = *home_it->second;
  if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value() || home.register_name->empty()) {
    return std::nullopt;
  }
  return home.register_name;
}

std::optional<std::string> prepared_pointer_register_for_value(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  const auto value_id = prepared_value_id_for(names, lookups, value);
  if (!value_id.has_value()) {
    return std::nullopt;
  }
  const auto home_it = lookups->value_homes.homes_by_id.find(*value_id);
  if (home_it == lookups->value_homes.homes_by_id.end() || home_it->second == nullptr) {
    return std::nullopt;
  }
  const auto& home = *home_it->second;
  if ((home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register &&
       home.kind != c4c::backend::prepare::PreparedValueHomeKind::PointerBasePlusOffset) ||
      !home.register_name.has_value() ||
      home.register_name->empty()) {
    return std::nullopt;
  }
  return home.register_name;
}

std::optional<std::string> prepared_register_for_value_name_id(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::ValueNameId value_name) {
  if (lookups == nullptr || value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto value_id_it = lookups->value_homes.value_ids.find(value_name);
  if (value_id_it == lookups->value_homes.value_ids.end()) {
    return std::nullopt;
  }
  const auto home_it = lookups->value_homes.homes_by_id.find(value_id_it->second);
  if (home_it == lookups->value_homes.homes_by_id.end() || home_it->second == nullptr) {
    return std::nullopt;
  }
  const auto& home = *home_it->second;
  if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value() || home.register_name->empty()) {
    return std::nullopt;
  }
  return home.register_name;
}

bool has_frame_slot_address_materialization_at(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index) {
  namespace prepare = c4c::backend::prepare;

  if (lookups == nullptr) {
    return false;
  }
  const auto* materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups->address_materializations,
          block_label);
  if (materializations == nullptr) {
    return false;
  }
  for (const auto* materialization : *materializations) {
    if (materialization != nullptr &&
        materialization->inst_index == instruction_index &&
        materialization->kind == prepare::PreparedAddressMaterializationKind::FrameSlot &&
        materialization->address_space == c4c::backend::bir::AddressSpace::Default &&
        materialization->frame_slot_id.has_value()) {
      return true;
    }
  }
  return false;
}

}  // namespace c4c::backend::riscv::codegen
