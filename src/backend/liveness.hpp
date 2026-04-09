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

struct BackendCfgSignatureParam {
  std::string type;
  std::string operand;
  bool is_varargs = false;
};

struct BackendCfgCallResult {
  std::string value_name;
  std::string type_str;
};

struct BackendCfgFunction {
  std::vector<BackendCfgPoint> entry_insts;
  std::vector<BackendCfgSignatureParam> signature_params;
  std::optional<std::string> return_type;
  bool is_variadic = false;
  std::vector<BackendCfgCallResult> call_results;
  std::vector<BackendCfgBlock> blocks;
  std::vector<BackendCfgPhiIncomingUse> phi_incoming_uses;
};

struct BackendCfgLivenessPoint {
  std::vector<std::string> used_names;
  std::optional<std::string> result_name;
  bool is_call = false;
};

struct BackendCfgLivenessBlock {
  std::string label;
  std::vector<BackendCfgLivenessPoint> insts;
  std::vector<std::string> terminator_used_names;
  std::vector<std::string> successor_labels;
};

struct BackendCfgLivenessFunction {
  std::vector<BackendCfgLivenessPoint> entry_insts;
  std::vector<BackendCfgLivenessBlock> blocks;
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
BackendCfgLivenessFunction lower_backend_cfg_to_liveness_function(const BackendCfgFunction& function);
LivenessInput lower_backend_cfg_to_liveness_input(const BackendCfgLivenessFunction& function);
LivenessInput lower_backend_cfg_to_liveness_input(const BackendCfgFunction& function);
// Compatibility-only raw-LIR entrypoint. Prefer backend CFG/MIR-derived inputs
// in production paths.
[[deprecated("compatibility-only; prefer backend CFG/MIR inputs")]]
LivenessInput lower_lir_to_liveness_input(const c4c::codegen::lir::LirFunction& function);
LivenessResult compute_live_intervals(const LivenessInput& input);

}  // namespace c4c::backend
