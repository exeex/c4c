#pragma once

#include <string>
#include <string_view>

#include "ir.hpp"

namespace c4c::codegen::llvm_backend {

using Module = c4c::hir::Module;

/// Codegen path selection.
enum class CodegenPath {
  Legacy,   // HirEmitter (current default)
  Lir,      // hir_to_lir + lir_printer (stub, under construction)
  Compare,  // run both, diff output
};

std::string emit_module_native(const Module& mod,
                               std::string_view target_triple,
                               CodegenPath path = CodegenPath::Legacy);

}  // namespace c4c::codegen::llvm_backend
