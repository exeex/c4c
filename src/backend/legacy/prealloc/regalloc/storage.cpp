#include "storage.hpp"

#include <algorithm>

namespace c4c::backend::prepare {

namespace regalloc_detail {

PreparedMoveStorageKind assigned_storage_kind(const PreparedRegallocValue& value) {
  if (value.assigned_register.has_value()) {
    return PreparedMoveStorageKind::Register;
  }
  if (value.assigned_stack_slot.has_value()) {
    return PreparedMoveStorageKind::StackSlot;
  }
  return PreparedMoveStorageKind::None;
}

bool assigned_storage_matches(const PreparedRegallocValue& lhs,
                              const PreparedRegallocValue& rhs) {
  const PreparedMoveStorageKind lhs_kind = assigned_storage_kind(lhs);
  const PreparedMoveStorageKind rhs_kind = assigned_storage_kind(rhs);
  if (lhs_kind != rhs_kind) {
    return false;
  }
  switch (lhs_kind) {
    case PreparedMoveStorageKind::None:
      return true;
    case PreparedMoveStorageKind::Register:
      return lhs.assigned_register->occupied_register_names ==
             rhs.assigned_register->occupied_register_names;
    case PreparedMoveStorageKind::StackSlot:
      return lhs.assigned_stack_slot->slot_id == rhs.assigned_stack_slot->slot_id;
  }
  return false;
}

const PreparedRegallocValue* find_regalloc_value(const PreparedRegallocFunction& function,
                                                 const PreparedNameTables& names,
                                                 std::string_view value_name) {
  const ValueNameId value_name_id = names.value_names.find(value_name);
  if (value_name_id == kInvalidValueName) {
    return nullptr;
  }
  const auto it = std::find_if(function.values.begin(),
                               function.values.end(),
                               [value_name_id](const PreparedRegallocValue& value) {
                                 return value.value_name == value_name_id;
                               });
  if (it == function.values.end()) {
    return nullptr;
  }
  return &*it;
}

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
