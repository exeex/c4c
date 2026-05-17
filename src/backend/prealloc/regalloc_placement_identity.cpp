#include "regalloc_placement_identity.hpp"
#include "target_register_profile.hpp"

#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::prepare {

namespace {

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

}  // namespace

void populate_regalloc_placement_identity(PreparedBirModule& prepared) {
  for (auto& function : prepared.regalloc.functions) {
    for (auto& value : function.values) {
      if (value.assigned_register.has_value()) {
        value.assigned_register->placement =
            assignment_register_placement(prepared.target_profile, *value.assigned_register);
      }
      if (value.spill_register_authority.has_value()) {
        value.spill_register_authority->placement =
            assignment_register_placement(prepared.target_profile, *value.spill_register_authority);
      }
      if (value.assigned_stack_slot.has_value()) {
        value.assigned_stack_slot->placement = PreparedSpillSlotPlacement{
            .slot_id = value.assigned_stack_slot->slot_id,
            .offset_bytes = value.assigned_stack_slot->offset_bytes,
        };
      }
    }

    for (auto& op : function.spill_reload_ops) {
      const auto* value = find_regalloc_value_by_id(function, op.value_id);
      if (value != nullptr) {
        const auto& published_register = value->assigned_register.has_value()
                                             ? value->assigned_register
                                             : value->spill_register_authority;
        if (published_register.has_value()) {
          op.register_placement =
              assignment_register_placement(prepared.target_profile, *published_register);
        }
      }
      op.spill_slot_placement = make_spill_slot_placement(op.slot_id, op.stack_offset_bytes);
    }
  }
}

}  // namespace c4c::backend::prepare
