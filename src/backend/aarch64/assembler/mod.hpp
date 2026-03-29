#pragma once

#include "parser.hpp"

namespace c4c::backend::aarch64::assembler {

// Marker type for the staged AArch64 assembler contract surface.
struct ContractSurface final {};

// Current staged assembler entry: raw GNU-style assembly text in, placeholder
// object-emission hook out. The implementation is still a stub.
std::string assemble(const std::string& asm_text, const std::string& output_path);

}  // namespace c4c::backend::aarch64::assembler
