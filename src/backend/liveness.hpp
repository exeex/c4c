#pragma once

#include "../codegen/lir/ir.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend {

struct LiveInterval {
  std::uint32_t start = 0;
  std::uint32_t end = 0;
  std::string value_name;
};

struct LivenessResult {
  std::vector<LiveInterval> intervals;
  std::vector<std::uint32_t> call_points;
  std::vector<std::uint32_t> block_loop_depth;

  [[nodiscard]] const LiveInterval* find_interval(std::string_view value_name) const;
};

LivenessResult compute_live_intervals(const c4c::codegen::lir::LirFunction& function);

}  // namespace c4c::backend
