#include "regalloc_helpers.hpp"

#include <algorithm>

namespace c4c::backend::stack_layout {

std::vector<PhysReg> filter_available_regs(const std::vector<PhysReg>& callee_saved,
                                           const std::vector<PhysReg>& asm_clobbered) {
  std::vector<PhysReg> filtered;
  filtered.reserve(callee_saved.size());

  for (const auto& reg : callee_saved) {
    const bool is_clobbered =
        std::any_of(asm_clobbered.begin(), asm_clobbered.end(),
                    [&](const PhysReg& clobbered) {
                      return clobbered.index == reg.index;
                    });
    if (!is_clobbered) {
      filtered.push_back(reg);
    }
  }

  return filtered;
}

const PhysReg* find_assigned_reg(const RegAllocIntegrationResult& regalloc,
                                 const std::string& value_name) {
  const auto it = regalloc.reg_assignments.find(value_name);
  if (it == regalloc.reg_assignments.end()) {
    return nullptr;
  }
  return &it->second;
}

bool uses_callee_saved_reg(const RegAllocIntegrationResult& regalloc, PhysReg reg) {
  return std::any_of(regalloc.used_callee_saved.begin(),
                     regalloc.used_callee_saved.end(),
                     [&](const PhysReg& used) { return used.index == reg.index; });
}

const LivenessResult* find_cached_liveness(const RegAllocIntegrationResult& regalloc) {
  return regalloc.cached_liveness ? &*regalloc.cached_liveness : nullptr;
}

}  // namespace c4c::backend::stack_layout
