#include "intervals.hpp"

#include <algorithm>
#include <limits>

namespace c4c::backend::prepare::regalloc_detail {

bool intervals_overlap(const PreparedLiveInterval& lhs,
                       const PreparedLiveInterval& rhs) {
  return std::max(lhs.start_point, rhs.start_point) <= std::min(lhs.end_point, rhs.end_point);
}

std::size_t value_priority(const PreparedLivenessValue& value) {
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

namespace {

[[nodiscard]] std::size_t loop_depth_weight(std::size_t loop_depth) {
  switch (loop_depth) {
    case 0:
      return 1U;
    case 1:
      return 10U;
    case 2:
      return 100U;
    case 3:
      return 1000U;
    default:
      return 10000U;
  }
}

}  // namespace

std::optional<ProgramPointLocation> locate_program_point(
    const PreparedLivenessFunction& function,
    std::size_t point) {
  for (const auto& block : function.blocks) {
    if (point < block.start_point || point > block.end_point) {
      continue;
    }
    return ProgramPointLocation{
        .block_index = block.block_index,
        .instruction_index = point - block.start_point,
    };
  }
  return std::nullopt;
}

std::size_t weighted_use_score(const PreparedLivenessFunction& function,
                               const PreparedLivenessValue& value) {
  std::size_t weighted_uses = 0;
  for (const std::size_t use_point : value.use_points) {
    std::size_t weight = 1U;
    if (const auto location = locate_program_point(function, use_point); location.has_value() &&
        location->block_index < function.block_loop_depth.size()) {
      weight = loop_depth_weight(function.block_loop_depth[location->block_index]);
    }
    weighted_uses += weight;
  }
  return weighted_uses;
}

std::size_t interval_start_sort_key(const PreparedRegallocValue& value) {
  if (!value.live_interval.has_value()) {
    return std::numeric_limits<std::size_t>::max();
  }
  return value.live_interval->start_point;
}

}  // namespace c4c::backend::prepare::regalloc_detail
