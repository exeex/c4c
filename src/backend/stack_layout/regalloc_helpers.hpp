#pragma once

#include "../regalloc.hpp"

#include <vector>

namespace c4c::backend::stack_layout {

std::vector<PhysReg> filter_available_regs(const std::vector<PhysReg>& callee_saved,
                                           const std::vector<PhysReg>& asm_clobbered);

}  // namespace c4c::backend::stack_layout
