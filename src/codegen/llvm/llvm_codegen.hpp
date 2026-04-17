#pragma once

#include <string>
#include <string_view>

#include "../target_profile.hpp"
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
                               const c4c::TargetProfile& target_profile,
                               CodegenPath path = CodegenPath::Llvm,
                               bool emit_semantic_bir = false);

}  // namespace c4c::codegen::llvm_backend
