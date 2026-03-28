#pragma once

#include "../../codegen/lir/ir.hpp"

#include <ostream>

namespace c4c::backend::aarch64 {

void validate_module(const c4c::codegen::lir::LirModule& module);
void validate_function(const c4c::codegen::lir::LirFunction& function);
void render_module_header(std::ostream& out,
                          const c4c::codegen::lir::LirModule& module);
void render_function_prologue(std::ostream& out,
                              const c4c::codegen::lir::LirFunction& function);

}  // namespace c4c::backend::aarch64
