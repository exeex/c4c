#include "assignment.hpp"

namespace c4c::backend::prepare::regalloc_detail {

bool assignments_overlap(const ActiveRegisterAssignment& active_assignment,
                         const PreparedRegisterCandidateSpan& candidate) {
  for (const auto& active_register : active_assignment.occupied_register_names) {
    for (const auto& candidate_register : candidate.occupied_register_names) {
      if (active_register == candidate_register) {
        return true;
      }
    }
  }
  return false;
}

std::optional<PreparedRegisterCandidateSpan> choose_register_span(
    const std::vector<ActiveRegisterAssignment>& active,
    const std::vector<PreparedRegisterCandidateSpan>& candidate_spans) {
  for (const auto& candidate : candidate_spans) {
    bool overlap = false;
    for (const auto& assignment : active) {
      if (assignments_overlap(assignment, candidate)) {
        overlap = true;
        break;
      }
    }
    if (!overlap) {
      return candidate;
    }
  }
  return std::nullopt;
}

void expire_completed_assignments(std::vector<ActiveRegisterAssignment>& active,
                                  std::size_t start_point,
                                  bool preserve_call_boundary_pressure) {
  active.erase(std::remove_if(active.begin(),
                              active.end(),
                              [start_point, preserve_call_boundary_pressure](
                                  const ActiveRegisterAssignment& assignment) {
                                if (assignment.end_point < start_point) {
                                  return true;
                                }
                                return !preserve_call_boundary_pressure &&
                                       assignment.end_point == start_point;
                              }),
               active.end());
}

bool has_lower_allocation_rank(const PreparedRegallocValue& lhs,
                               const PreparedRegallocValue& rhs) {
  if (lhs.spill_weight != rhs.spill_weight) {
    return lhs.spill_weight < rhs.spill_weight;
  }
  if (lhs.priority != rhs.priority) {
    return lhs.priority < rhs.priority;
  }
  if (lhs.live_interval.has_value() != rhs.live_interval.has_value()) {
    return !lhs.live_interval.has_value();
  }
  if (lhs.live_interval.has_value() && rhs.live_interval.has_value() &&
      lhs.live_interval->start_point != rhs.live_interval->start_point) {
    return lhs.live_interval->start_point > rhs.live_interval->start_point;
  }
  return lhs.value_id > rhs.value_id;
}

}  // namespace c4c::backend::prepare::regalloc_detail
