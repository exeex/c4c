#pragma once

#include "../codegen/lir/ir.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend {

struct LivenessPoint {
  std::vector<std::string> used_names;
  std::optional<std::string> result_name;
  bool is_call = false;
};

struct LivenessBlockInput {
  std::string label;
  std::vector<LivenessPoint> insts;
  std::vector<std::string> terminator_used_names;
  std::vector<std::string> successor_labels;
};

struct LivenessPhiIncomingUse {
  std::string predecessor_label;
  std::string value_name;
};

struct LivenessInput {
  std::vector<LivenessPoint> entry_insts;
  std::vector<LivenessBlockInput> blocks;
  std::vector<LivenessPhiIncomingUse> phi_incoming_uses;
};

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

LivenessInput lower_lir_to_liveness_input(const c4c::codegen::lir::LirFunction& function);
LivenessResult compute_live_intervals(const LivenessInput& input);

}  // namespace c4c::backend
