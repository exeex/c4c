#pragma once

// ── LIR Printer (LLVM flavor) ───────────────────────────────────────────────
//
// Consumes a LirModule and produces LLVM IR text.
// Rendering is stateless: all semantic decisions must be made before printing.

#include "ir.hpp"

#include <string>

namespace c4c::codegen::lir {

/// Print a LirModule as LLVM IR text.
std::string print_llvm(const LirModule& mod);

}  // namespace c4c::codegen::lir
