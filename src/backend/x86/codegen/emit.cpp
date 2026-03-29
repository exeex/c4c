#include "emit.hpp"

#include "../../../codegen/lir/lir_printer.hpp"

// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/x86/codegen/emit.rs
// Stage the x86 backend behind an explicit seam while the real codegen path is
// still being brought up.

std::string c4c::backend::x86::emit_module(
    const c4c::codegen::lir::LirModule& module) {
  return c4c::codegen::lir::print_llvm(module);
}
