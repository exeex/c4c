#pragma once

#include "../assembler/mod.hpp"
#include "../../../codegen/lir/ir.hpp"

#include <string>

namespace c4c::backend::x86 {

std::string emit_module(const c4c::codegen::lir::LirModule& module);
assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path);

}  // namespace c4c::backend::x86
