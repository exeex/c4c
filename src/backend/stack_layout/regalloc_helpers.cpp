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

}  // namespace c4c::backend::stack_layout
