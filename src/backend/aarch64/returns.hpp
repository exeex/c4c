#pragma once

#include "../../codegen/lir/ir.hpp"

#include <ostream>

namespace c4c::backend::aarch64 {

void render_return(std::ostream& out, const c4c::codegen::lir::LirRet& ret);

}  // namespace c4c::backend::aarch64
