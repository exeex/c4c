#pragma once

#include "regalloc.hpp"

#include <vector>

namespace c4c::backend {

struct RegAllocIntegrationResult {
  std::unordered_map<std::string, PhysReg> reg_assignments;
  std::vector<PhysReg> used_callee_saved;
  std::optional<LivenessResult> cached_liveness;
};

std::vector<PhysReg> merge_used_regs(const std::vector<PhysReg>& used_regs,
                                     const std::vector<PhysReg>& asm_clobbered);

RegAllocIntegrationResult run_regalloc_and_merge_clobbers(
    const LivenessInput& input,
    const RegAllocConfig& config,
    const std::vector<PhysReg>& asm_clobbered);

}  // namespace c4c::backend
