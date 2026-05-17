#pragma once

#include "../regalloc.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::prepare {

namespace regalloc_detail {

struct ActiveRegisterAssignment {
  std::size_t value_index = 0;
  std::size_t end_point = 0;
  std::string register_name;
  std::vector<std::string> occupied_register_names;
};

[[nodiscard]] bool assignments_overlap(const ActiveRegisterAssignment& active_assignment,
                                       const PreparedRegisterCandidateSpan& candidate);

[[nodiscard]] std::optional<PreparedRegisterCandidateSpan> choose_register_span(
    const std::vector<ActiveRegisterAssignment>& active,
    const std::vector<PreparedRegisterCandidateSpan>& candidate_spans);

[[nodiscard]] bool has_lower_allocation_rank(const PreparedRegallocValue& lhs,
                                             const PreparedRegallocValue& rhs);

template <typename CanEvict>
[[nodiscard]] std::optional<std::size_t> choose_eviction_candidate(
    const PreparedRegallocFunction& function,
    const std::vector<ActiveRegisterAssignment>& active,
    const std::vector<PreparedRegisterCandidateSpan>& candidate_spans,
    const PreparedRegallocValue& value,
    CanEvict can_evict) {
  std::optional<std::size_t> weakest_active_index;
  for (std::size_t active_index = 0; active_index < active.size(); ++active_index) {
    const auto& assignment = active[active_index];
    const bool overlaps_any_candidate =
        std::any_of(candidate_spans.begin(),
                    candidate_spans.end(),
                    [&](const PreparedRegisterCandidateSpan& candidate) {
                      return assignments_overlap(assignment, candidate);
                    });
    if (!overlaps_any_candidate || !can_evict(assignment)) {
      continue;
    }

    const auto& active_value = function.values[assignment.value_index];
    if (!has_lower_allocation_rank(active_value, value)) {
      continue;
    }
    if (!weakest_active_index.has_value() ||
        has_lower_allocation_rank(active_value,
                                  function.values[active[*weakest_active_index].value_index])) {
      weakest_active_index = active_index;
    }
  }
  return weakest_active_index;
}

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
