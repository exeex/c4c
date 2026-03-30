#pragma once

#include "../assembler/mod.hpp"
#include "../../ir.hpp"
#include "../../../codegen/lir/ir.hpp"

#include <string>

namespace c4c::backend::x86 {

std::string emit_module(const c4c::backend::BackendModule& module,
                        const c4c::codegen::lir::LirModule* legacy_fallback = nullptr);
std::string emit_module(const c4c::codegen::lir::LirModule& module);
assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path);

}  // namespace c4c::backend::x86
