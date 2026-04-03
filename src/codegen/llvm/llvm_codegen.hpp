#pragma once

#include <string>
#include <string_view>

#include "hir_ir.hpp"

namespace c4c::codegen::llvm_backend {

using Module = c4c::hir::Module;

/// Codegen path selection.
enum class CodegenPath {
  Llvm,     // HirEmitter (current default)
  Lir,      // hir_to_lir + lir_printer (under construction)
  Compare,  // run both and compare output
};

std::string emit_module_native(const Module& mod,
                               std::string_view target_triple,
                               CodegenPath path = CodegenPath::Llvm);

}  // namespace c4c::codegen::llvm_backend
