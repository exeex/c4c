#pragma once

#include "bir.hpp"
#include "../codegen/lir/ir.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend {

struct BackendCfgPoint {
  std::vector<std::string> used_names;
  std::optional<std::string> result_name;
  bool is_call = false;
  struct PointerAccess {
    std::string value_name;
    enum class Kind {
      Read,
      Store,
    } kind = Kind::Read;
  };
  std::vector<PointerAccess> pointer_accesses;
  std::vector<std::string> escaped_names;
  std::optional<std::pair<std::string, std::string>> derived_pointer_root;
};

struct BackendCfgBlock {
  std::string label;
  std::vector<BackendCfgPoint> insts;
  std::vector<std::string> terminator_used_names;
  std::vector<std::string> successor_labels;
};

struct BackendCfgPhiIncomingUse {
  std::string predecessor_label;
  std::string value_name;
};

struct BackendCfgFunction {
  std::vector<BackendCfgPoint> entry_insts;
  std::vector<BackendCfgBlock> blocks;
  std::vector<BackendCfgPhiIncomingUse> phi_incoming_uses;
};

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

BackendCfgFunction lower_bir_to_backend_cfg(const bir::Function& function);
BackendCfgFunction lower_lir_to_backend_cfg(const c4c::codegen::lir::LirFunction& function);
LivenessInput lower_backend_cfg_to_liveness_input(const BackendCfgFunction& function);
LivenessInput lower_lir_to_liveness_input(const c4c::codegen::lir::LirFunction& function);
LivenessResult compute_live_intervals(const LivenessInput& input);

}  // namespace c4c::backend
