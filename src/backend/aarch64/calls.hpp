#pragma once

#include "../../codegen/lir/ir.hpp"

#include <iosfwd>

namespace c4c::backend::aarch64 {

bool render_call_instruction(std::ostream& out,
                             const c4c::codegen::lir::LirInst& inst);

}  // namespace c4c::backend::aarch64
