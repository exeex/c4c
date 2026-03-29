#pragma once

#include "../generation.hpp"
#include "../regalloc.hpp"

#include <vector>

namespace c4c::backend::stack_layout {

std::vector<PhysReg> filter_available_regs(const std::vector<PhysReg>& callee_saved,
                                           const std::vector<PhysReg>& asm_clobbered);

const PhysReg* find_assigned_reg(const RegAllocIntegrationResult& regalloc,
                                 const std::string& value_name);

bool uses_callee_saved_reg(const RegAllocIntegrationResult& regalloc, PhysReg reg);

const LivenessResult* find_cached_liveness(const RegAllocIntegrationResult& regalloc);

}  // namespace c4c::backend::stack_layout
