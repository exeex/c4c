#include "regalloc.hpp"

#include <algorithm>

namespace c4c::backend {

RegAllocResult allocate_registers(const c4c::codegen::lir::LirFunction& function,
                                  const RegAllocConfig& config) {
  if (config.available_regs.empty() && config.caller_saved_regs.empty()) {
    return RegAllocResult{};
  }

  RegAllocResult result;
  result.liveness = compute_live_intervals(function);

  std::vector<PhysReg> pool = config.available_regs;
  pool.insert(pool.end(), config.caller_saved_regs.begin(),
              config.caller_saved_regs.end());

  std::size_t next_reg = 0;
  for (const auto& interval : result.liveness->intervals) {
    if (next_reg >= pool.size()) {
      break;
    }
    result.assignments.emplace(interval.value_name, pool[next_reg]);
    result.used_regs.push_back(pool[next_reg]);
    ++next_reg;
  }

  std::sort(result.used_regs.begin(), result.used_regs.end());
  result.used_regs.erase(
      std::unique(result.used_regs.begin(), result.used_regs.end(),
                  [](const PhysReg& lhs, const PhysReg& rhs) {
                    return lhs.index == rhs.index;
                  }),
      result.used_regs.end());

  return result;
}

}  // namespace c4c::backend
