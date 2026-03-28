#pragma once

#include "../../codegen/lir/ir.hpp"

#include <ostream>

namespace c4c::backend::aarch64 {

void render_entry_allocas(std::ostream& out,
                          const c4c::codegen::lir::LirFunction& function);
void render_function_epilogue(std::ostream& out);

}  // namespace c4c::backend::aarch64
