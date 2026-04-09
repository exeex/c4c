#include "regalloc.hpp"

#include <algorithm>
#include <unordered_set>

namespace {

using c4c::backend::LiveInterval;
using c4c::backend::PhysReg;

bool spans_any_call(const LiveInterval& interval,
                    const std::vector<std::uint32_t>& call_points) {
  const auto first_possible =
      std::lower_bound(call_points.begin(), call_points.end(), interval.start);
  return first_possible != call_points.end() && *first_possible <= interval.end;
}

std::vector<const LiveInterval*> collect_candidates(
    const c4c::backend::LivenessResult& liveness,
    const c4c::backend::RegAllocResult& result,
    bool require_call_spanning) {
  std::vector<const LiveInterval*> candidates;
  for (const auto& interval : liveness.intervals) {
    if (interval.end <= interval.start) {
      continue;
    }
    if (result.assignments.find(interval.value_name) != result.assignments.end()) {
      continue;
    }
    if (spans_any_call(interval, liveness.call_points) != require_call_spanning) {
      continue;
    }
    candidates.push_back(&interval);
  }
  return candidates;
}

std::optional<std::size_t> find_best_callee_reg(
    const std::vector<std::uint32_t>& reg_free_until,
    std::uint32_t interval_start,
    const std::vector<PhysReg>& available_regs,
    const std::unordered_set<std::uint8_t>& used_regs) {
  std::optional<std::size_t> best_used;
  std::uint32_t best_used_free = UINT32_MAX;
  std::optional<std::size_t> best_new;
  std::uint32_t best_new_free = UINT32_MAX;

  for (std::size_t i = 0; i < reg_free_until.size(); ++i) {
    if (reg_free_until[i] > interval_start) {
      continue;
    }
    const auto reg_index = available_regs[i].index;
    if (used_regs.find(reg_index) != used_regs.end()) {
      if (!best_used.has_value() || reg_free_until[i] < best_used_free) {
        best_used = i;
        best_used_free = reg_free_until[i];
      }
      continue;
    }
    if (!best_new.has_value() || reg_free_until[i] < best_new_free) {
      best_new = i;
      best_new_free = reg_free_until[i];
    }
  }

  return best_used.has_value() ? best_used : best_new;
}

std::optional<std::size_t> find_first_free_reg(
    const std::vector<std::uint32_t>& reg_free_until,
    std::uint32_t interval_start) {
  for (std::size_t i = 0; i < reg_free_until.size(); ++i) {
    if (reg_free_until[i] <= interval_start) {
      return i;
    }
  }
  return std::nullopt;
}

}  // namespace

namespace c4c::backend {

RegAllocResult allocate_registers(const LivenessInput& input,
                                  const RegAllocConfig& config) {
  if (config.available_regs.empty() && config.caller_saved_regs.empty()) {
    return RegAllocResult{};
  }

  RegAllocResult result;
  result.liveness = compute_live_intervals(input);
  auto& liveness = *result.liveness;

  std::vector<std::uint32_t> callee_free_until(config.available_regs.size(), 0);
  std::vector<std::uint32_t> caller_free_until(config.caller_saved_regs.size(), 0);
  std::unordered_set<std::uint8_t> used_callee_regs;

  for (const LiveInterval* interval : collect_candidates(liveness, result, true)) {
    const auto reg_index = find_best_callee_reg(
        callee_free_until, interval->start, config.available_regs, used_callee_regs);
    if (!reg_index.has_value()) {
      continue;
    }
    callee_free_until[*reg_index] = interval->end + 1;
    result.assignments.emplace(interval->value_name, config.available_regs[*reg_index]);
    used_callee_regs.insert(config.available_regs[*reg_index].index);
  }

  for (const LiveInterval* interval : collect_candidates(liveness, result, false)) {
    const auto reg_index = find_first_free_reg(caller_free_until, interval->start);
    if (!reg_index.has_value()) {
      continue;
    }
    caller_free_until[*reg_index] = interval->end + 1;
    result.assignments.emplace(interval->value_name,
                               config.caller_saved_regs[*reg_index]);
  }

  for (const LiveInterval* interval : collect_candidates(liveness, result, false)) {
    const auto reg_index = find_best_callee_reg(
        callee_free_until, interval->start, config.available_regs, used_callee_regs);
    if (!reg_index.has_value()) {
      continue;
    }
    callee_free_until[*reg_index] = interval->end + 1;
    result.assignments.emplace(interval->value_name, config.available_regs[*reg_index]);
    used_callee_regs.insert(config.available_regs[*reg_index].index);
  }

  result.used_regs.reserve(used_callee_regs.size());
  for (const auto reg_index : used_callee_regs) {
    result.used_regs.push_back(PhysReg{reg_index});
  }
  std::sort(result.used_regs.begin(), result.used_regs.end());

  return result;
}

}  // namespace c4c::backend
