#pragma once

#include "../../codegen/lir/ir.hpp"

#include <ostream>

namespace c4c::backend::aarch64 {

bool render_branch_instruction(std::ostream& out,
                               const c4c::codegen::lir::LirInst& inst);
void render_terminator(std::ostream& out,
                       const c4c::codegen::lir::LirTerminator& terminator);

}  // namespace c4c::backend::aarch64
