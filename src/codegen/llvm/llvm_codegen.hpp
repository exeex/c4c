#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "../target_profile.hpp"
#include "hir_ir.hpp"

namespace c4c::codegen::llvm_backend {

using Module = c4c::hir::Module;

/// Codegen path selection.
enum class CodegenPath {
  Llvm,     // HirEmitter (current default)
  Lir,      // hir_to_lir + lir_printer (under construction)
  Obj,      // hir_to_lir + backend native object bytes
  Compare,  // run both and compare output
};

struct NativeObjectResult {
  std::vector<std::uint8_t> bytes;
  std::string diagnostic;

  [[nodiscard]] bool ok() const {
    return diagnostic.empty() && !bytes.empty();
  }
};

std::string emit_module_native(const Module& mod,
                               const c4c::TargetProfile& target_profile,
                               CodegenPath path = CodegenPath::Llvm,
                               bool emit_semantic_bir = false);

NativeObjectResult emit_module_native_object(
    const Module& mod,
    const c4c::TargetProfile& target_profile);

}  // namespace c4c::codegen::llvm_backend
