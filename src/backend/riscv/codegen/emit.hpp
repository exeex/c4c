#pragma once

#include "../assembler/mod.hpp"

#include <optional>
#include <string>

namespace c4c::codegen::lir {
struct LirModule;
}

namespace c4c::backend::riscv {

assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path);

}  // namespace c4c::backend::riscv

namespace c4c::backend::riscv::codegen {

std::string emit_prepared_lir_module(const c4c::codegen::lir::LirModule& module);
std::string peephole_optimize(std::string asm_text);

}  // namespace c4c::backend::riscv::codegen
