#pragma once

// ── LIR Printer (LLVM flavor) ───────────────────────────────────────────────
//
// Consumes a LirModule and produces LLVM IR text.
// Stage 0: skeleton interface only.
// Stage 2+: will replace HirEmitter's direct string emission.

#include "ir.hpp"

#include <string>

namespace c4c::codegen::lir {

/// Print a LirModule as LLVM IR text.
/// Stage 0: returns an empty string.
/// Stage 2+: will produce the complete LLVM IR output.
std::string print_llvm(const LirModule& mod);

}  // namespace c4c::codegen::lir
