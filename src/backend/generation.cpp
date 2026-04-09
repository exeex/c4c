#include "generation.hpp"

#include <algorithm>

namespace c4c::backend {

std::vector<PhysReg> merge_used_regs(const std::vector<PhysReg>& used_regs,
                                     const std::vector<PhysReg>& asm_clobbered) {
  std::vector<PhysReg> merged = used_regs;
  merged.insert(merged.end(), asm_clobbered.begin(), asm_clobbered.end());
  std::sort(merged.begin(), merged.end());
  merged.erase(std::unique(merged.begin(), merged.end(),
                           [](const PhysReg& lhs, const PhysReg& rhs) {
                             return lhs.index == rhs.index;
                           }),
               merged.end());
  return merged;
}

RegAllocIntegrationResult run_regalloc_and_merge_clobbers(
    const LivenessInput& input,
    const RegAllocConfig& config,
    const std::vector<PhysReg>& asm_clobbered) {
  RegAllocResult alloc_result = allocate_registers(input, config);
  return RegAllocIntegrationResult{
      std::move(alloc_result.assignments),
      merge_used_regs(alloc_result.used_regs, asm_clobbered),
      std::move(alloc_result.liveness),
  };
}

}  // namespace c4c::backend
