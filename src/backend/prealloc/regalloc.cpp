#include "prealloc.hpp"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

namespace c4c::backend::prepare {

namespace {

[[nodiscard]] PreparedRegisterClass classify_register_class(const PreparedLivenessValue& value) {
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
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
      return PreparedRegisterClass::None;
  }
  return PreparedRegisterClass::None;
}

[[nodiscard]] bool intervals_overlap(const PreparedLiveInterval& lhs,
                                     const PreparedLiveInterval& rhs) {
  return std::max(lhs.start_point, rhs.start_point) <= std::min(lhs.end_point, rhs.end_point);
}

[[nodiscard]] std::size_t value_priority(const PreparedLivenessValue& value) {
  std::size_t priority = value.use_points.size();
  if (value.live_interval.has_value() && value.live_interval->end_point >= value.live_interval->start_point) {
    priority += (value.live_interval->end_point - value.live_interval->start_point) + 1U;
  }
  if (value.crosses_call) {
    priority += 2U;
  }
  if (value.requires_home_slot) {
    priority += 1U;
  }
  return priority;
}

}  // namespace

void BirPreAlloc::run_regalloc() {
  prepared_.completed_phases.push_back("regalloc");
  prepared_.regalloc.functions.clear();
  prepared_.regalloc.functions.reserve(prepared_.liveness.functions.size());

  for (const auto& liveness_function : prepared_.liveness.functions) {
    PreparedRegallocFunction regalloc_function{
        .function_name = liveness_function.function_name,
        .values = {},
        .constraints = {},
        .interference = {},
        .move_resolution = {},
        .spill_reload_ops = {},
    };
    regalloc_function.values.reserve(liveness_function.values.size());
    regalloc_function.constraints.reserve(liveness_function.values.size());

    for (const auto& liveness_value : liveness_function.values) {
      const PreparedRegisterClass register_class = classify_register_class(liveness_value);
      const bool eligible_for_register_seed =
          register_class != PreparedRegisterClass::None &&
          liveness_value.value_kind != PreparedValueKind::StackObject;
      const std::size_t priority = value_priority(liveness_value);

      regalloc_function.values.push_back(PreparedRegallocValue{
          .value_id = liveness_value.value_id,
          .stack_object_id = liveness_value.stack_object_id,
          .function_name = liveness_value.function_name,
          .value_name = liveness_value.value_name,
          .type = liveness_value.type,
          .value_kind = liveness_value.value_kind,
          .register_class = register_class,
          .allocation_status = PreparedAllocationStatus::Unallocated,
          .spillable = eligible_for_register_seed && !liveness_value.requires_home_slot,
          .requires_home_slot = liveness_value.requires_home_slot,
          .crosses_call = liveness_value.crosses_call,
          .priority = priority,
          .spill_weight = static_cast<double>(priority),
          .live_interval = liveness_value.live_interval,
          .assigned_register = std::nullopt,
          .assigned_stack_slot = std::nullopt,
      });

      if (!eligible_for_register_seed) {
        continue;
      }

      regalloc_function.constraints.push_back(PreparedAllocationConstraint{
          .value_id = liveness_value.value_id,
          .register_class = register_class,
          .requires_register = true,
          .requires_home_slot = liveness_value.requires_home_slot,
          .cannot_cross_call = liveness_value.crosses_call &&
                               register_class != PreparedRegisterClass::Float,
          .fixed_register_name = std::nullopt,
          .preferred_register_names = {},
          .forbidden_register_names = {},
      });
    }

    for (std::size_t lhs_index = 0; lhs_index < regalloc_function.values.size(); ++lhs_index) {
      const auto& lhs = regalloc_function.values[lhs_index];
      if (!lhs.live_interval.has_value() || lhs.register_class == PreparedRegisterClass::None) {
        continue;
      }
      for (std::size_t rhs_index = lhs_index + 1U; rhs_index < regalloc_function.values.size(); ++rhs_index) {
        const auto& rhs = regalloc_function.values[rhs_index];
        if (!rhs.live_interval.has_value() || rhs.register_class == PreparedRegisterClass::None) {
          continue;
        }
        if (!intervals_overlap(*lhs.live_interval, *rhs.live_interval)) {
          continue;
        }
        regalloc_function.interference.push_back(PreparedInterferenceEdge{
            .lhs_value_id = lhs.value_id,
            .rhs_value_id = rhs.value_id,
            .reason = "overlapping_live_intervals",
        });
      }
    }

    prepared_.notes.push_back(PrepareNote{
        .phase = "regalloc",
        .message = "regalloc seeded function '" + regalloc_function.function_name + "' with " +
                   std::to_string(regalloc_function.constraints.size()) +
                   " allocation constraint(s) and " +
                   std::to_string(regalloc_function.interference.size()) +
                   " interference edge(s) from active liveness",
    });
    prepared_.regalloc.functions.push_back(std::move(regalloc_function));
  }
}

}  // namespace c4c::backend::prepare
