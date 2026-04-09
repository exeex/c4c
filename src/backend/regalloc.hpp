#pragma once

#include "liveness.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace c4c::backend {

struct PhysReg {
  std::uint8_t index = 0;

  [[nodiscard]] constexpr bool operator==(const PhysReg& other) const {
    return index == other.index;
  }

  [[nodiscard]] constexpr bool operator!=(const PhysReg& other) const {
    return !(*this == other);
  }

  [[nodiscard]] constexpr bool operator<(const PhysReg& other) const {
    return index < other.index;
  }
};

struct RegAllocConfig {
  std::vector<PhysReg> available_regs;
  std::vector<PhysReg> caller_saved_regs;
  bool allow_inline_asm_regalloc = false;
};

struct RegAllocResult {
  std::unordered_map<std::string, PhysReg> assignments;
  std::vector<PhysReg> used_regs;
  std::optional<LivenessResult> liveness;
};

RegAllocResult allocate_registers(const LivenessInput& input,
                                  const RegAllocConfig& config);
RegAllocResult allocate_registers(const c4c::codegen::lir::LirFunction& function,
                                  const RegAllocConfig& config);

}  // namespace c4c::backend
