#pragma once

#include "../../codegen/lir/ir.hpp"

#include <ostream>

namespace c4c::backend::aarch64 {

void render_module_string_pool(std::ostream& out,
                               const c4c::codegen::lir::LirModule& module);

void render_module_globals(std::ostream& out,
                           const c4c::codegen::lir::LirModule& module);

void render_module_extern_decls(std::ostream& out,
                                const c4c::codegen::lir::LirModule& module);

}  // namespace c4c::backend::aarch64
